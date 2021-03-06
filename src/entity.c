#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include "include/private/flecs.h"

static
void copy_column(
    EcsTableColumn *new_column,
    uint32_t new_index,
    EcsTableColumn *old_column,
    uint32_t old_index)
{
    uint32_t size = new_column->size;

    if (size) {
        EcsArrayParams param = {.element_size = new_column->size};
        void *dst = ecs_array_get(new_column->data, &param, new_index);
        void *src = ecs_array_get(old_column->data, &param, old_index);
            
        ecs_assert(dst != NULL, ECS_INTERNAL_ERROR, NULL);
        ecs_assert(src != NULL, ECS_INTERNAL_ERROR, NULL);

        memcpy(dst, src, param.element_size);
    }
}

static
void copy_row(
    EcsArray *new_type,
    EcsTableColumn *new_columns,
    uint32_t new_index,
    EcsArray *old_type,
    EcsTableColumn *old_columns,
    uint32_t old_index)
{
    uint16_t i_new, new_component_count = ecs_array_count(new_type);
    uint16_t i_old = 0, old_component_count = ecs_array_count(old_type);
    EcsEntity *new_components = ecs_array_buffer(new_type);
    EcsEntity *old_components = ecs_array_buffer(old_type);

    ecs_assert(old_columns->data != NULL, ECS_INTERNAL_ERROR, NULL);
    ecs_assert(new_columns->data != NULL, ECS_INTERNAL_ERROR, NULL);

    for (i_new = 0; i_new < new_component_count; ) {
        if (i_old == old_component_count) {
            break;
        }

        EcsEntity new_component = new_components[i_new];
        EcsEntity old_component = old_components[i_old];

        if (new_component == old_component) {
            copy_column(&new_columns[i_new + 1], new_index, &old_columns[i_old + 1], old_index);
            i_new ++;
            i_old ++;
        } else if (new_component < old_component) {
            i_new ++;
        } else if (new_component > old_component) {
            i_old ++;
        }
    }
}

static
void* get_row_ptr(
    EcsWorld *world,
    EcsStage *stage,
    EcsArray *type,
    EcsTableColumn *columns,
    uint32_t index,
    EcsEntity component)
{
    uint32_t column_index = ecs_type_index_of(type, component);
    if (column_index == -1) {
        return NULL;
    }

    EcsTableColumn *column = &columns[column_index + 1];
    EcsArrayParams param = {.element_size = column->size};

    if (param.element_size) {
        ecs_assert(column->data != NULL, ECS_INTERNAL_ERROR, NULL);
        return ecs_array_get(column->data, &param, index);
    } else {
        return NULL;
    }
}

static
void* get_ptr(
    EcsWorld *world,
    EcsEntity entity,
    EcsEntity component,
    bool staged_only,
    bool search_prefab,
    EcsEntityInfo *info)
{
    uint64_t row_64;
    EcsType type_id = 0, staged_id = 0;
    void *ptr = NULL;
    EcsStage *stage = ecs_get_stage(&world);

    if (world->in_progress) {
        row_64 = ecs_map_get64(stage->entity_index, entity);
        if (row_64) {
            EcsRow row = ecs_to_row(row_64);
            staged_id = row.type_id;
            EcsTableColumn *columns = ecs_map_get(stage->data_stage, staged_id);
            EcsTable *table = ecs_world_get_table(world, stage, staged_id);
            info->entity = entity;
            info->type_id = row.type_id;
            info->index = row.index;
            info->table = table;
            info->columns = columns;
            ptr = get_row_ptr(
                world, stage, table->type, columns, row.index, component);  
        }

        if (!ptr) {
            /* If component is in to remove type, it has been removed while in
             * progress. Return NULL if so. */
            EcsType to_remove = ecs_map_get64(stage->remove_merge, entity);
            if (to_remove) {
                if (ecs_type_contains_component(
                    world, stage, to_remove, component, false)) 
                {
                    return NULL;
                }
            }
        } 
    }

    if (ptr) return ptr;

    EcsEntity prefab = 0;

    if (!world->in_progress || !staged_only) {
        row_64 = ecs_map_get64(world->main_stage.entity_index, entity);
        if (row_64) {
            EcsRow row = ecs_to_row(row_64);
            type_id = row.type_id;
            EcsTable *table = ecs_world_get_table(world, stage, type_id);
            info->entity = entity;
            info->type_id = row.type_id;
            info->index = row.index;
            info->table = table;
            info->columns = table->columns;
            ptr = get_row_ptr(
                world, stage, table->type, table->columns, row.index, component);
        }

        if (ptr) return ptr;

        if (type_id && search_prefab) {
            prefab = ecs_map_get64(world->prefab_index, type_id);
        }
    }

    if (!prefab && staged_id && search_prefab) {
        prefab = ecs_map_get64(world->prefab_index, staged_id);
    }

    if (prefab) {
        if (component != EEcsId && component != EEcsPrefab) {
            return get_ptr(world, prefab, component, staged_only, true, info);
        } else {
            return NULL;
        }
    } else {
        return NULL;
    }
}

/** Copy default values from base (and base of base) prefabs */
static
void copy_from_prefab(
    EcsWorld *world,
    EcsStage *stage,
    EcsTable *table,
    EcsEntity entity,
    uint32_t offset,
    uint32_t limit,
    EcsType type_id,
    EcsType to_add)
{
    EcsEntity prefab;
    EcsType entity_type = type_id;

    if (world->in_progress) {
        uint64_t row64 = ecs_map_get64(stage->entity_index, entity);
        if (row64) {
            EcsRow row = ecs_to_row(row64);
            entity_type = row.type_id;
        }
    }

    while ((prefab = ecs_map_get64(world->prefab_index, entity_type))) {
        /* Prefabs are only resolved from the main stage. Prefabs created while
         * iterating cannot be resolved in the same iteration. */
        EcsRow row = ecs_to_row(ecs_map_get64(world->main_stage.entity_index, prefab));

        EcsTable *prefab_table = ecs_world_get_table(
            world, stage, row.type_id);

        EcsArray *add_type = ecs_type_get(world, stage, to_add);
        EcsEntity *add_handles = ecs_array_buffer(add_type);
        uint32_t i, add_count = ecs_array_count(add_type);

        for (i = 0; i < add_count; i ++) {
            EcsEntity component = add_handles[i];
            void *prefab_ptr = get_row_ptr(
                world, stage, prefab_table->type, prefab_table->columns,
                row.index, component);

            if (prefab_ptr) {
                EcsTableColumn *columns;
                if (world->in_progress) {
                    columns = ecs_map_get(stage->data_stage, type_id);
                } else {
                    columns = table->columns;
                }

                uint32_t size = columns->size;

                if (size) {
                    void *ptr = get_row_ptr(
                        world, stage, table->type, columns, offset, component);
                    if (ptr) {
                        int i;
                        for (i = 0; i < limit; i ++) {
                            memcpy(ptr, prefab_ptr, size);
                            ptr = ECS_OFFSET(ptr, size);
                        }
                    }
                }
            }
        }

        /* Recursively search through prefabs */
        entity_type = row.type_id;
    }
}

static
bool notify_pre_merge(
    EcsWorld *world,
    EcsTable *table,
    EcsTableColumn *table_columns,
    uint32_t offset,
    uint32_t limit,
    EcsType to_init,
    EcsMap *systems)
{
    if (world->is_merging) {
        return false;
    }

    bool in_progress = world->in_progress;
    world->in_progress = true;

    bool result = ecs_notify(
        world, systems, to_init, table, table_columns, offset, limit);

    world->in_progress = in_progress;
    if (result && !in_progress) {
        ecs_merge(world);

    }

    return result;
}

static
bool notify_post_merge(
    EcsWorld *world,
    EcsTable *table,
    EcsTableColumn *table_columns,
    uint32_t offset,
    uint32_t limit,
    EcsType to_deinit)
{
    if (world->in_progress) {
        return false;
    }

    /* This will trigger asserts when operations are invoked during a merge that
     * are not valid while merging. */
    bool is_merging = world->is_merging;
    world->is_merging = true;

    bool result = ecs_notify(
        world, world->type_sys_remove_index, to_deinit, table, 
        table_columns, offset, limit);

    world->is_merging = is_merging;

    return result;
}

/** Commit an entity with a specified type to memory */
static
uint32_t commit_w_type(
    EcsWorld *world,
    EcsStage *stage,
    EcsEntityInfo *info,
    EcsType type_id,
    EcsType to_add,
    EcsType to_remove)
{
    EcsTable *old_table, *new_table = NULL;
    EcsTableColumn *new_columns, *old_columns;
    EcsMap *entity_index;
    EcsType old_type_id = 0;
    uint32_t new_index = -1, old_index = -1;
    bool in_progress = world->in_progress;
    EcsEntity entity = info->entity;
    EcsArray *old_type = NULL;

    entity_index = stage->entity_index;

    /* Always update remove_merge stage when in progress. It is possible (and
     * likely) that when a component is removed, it hasn't been added in the
     * same iteration. As a result, the staged entity index does not know about
     * the component to remove, and thus the staged type_id of the entity when
     * merged with the to_remove type id results in the same type id. This would
     * ordinarily cause the commit action to be skipped, but when in progress,
     * we need to keep track of the remove so that it is removed from the main
     * stage once the merge takes place. */
    if (in_progress) {
        /* Update remove type. Add to_remove, and subtract to_add. */
        EcsType remove_type = ecs_map_get64(stage->remove_merge, entity);

        remove_type = ecs_type_merge(
            world, stage, remove_type, to_remove, to_add);
        ecs_map_set64(stage->remove_merge, entity, remove_type);
    }

    if ((old_table = info->table)) {
        old_type_id = info->type_id;
        if (old_type_id == type_id) {
            return info->index;
        }

        old_index = info->index;
        if (in_progress) {
            old_columns = ecs_map_get(stage->data_stage, old_type_id);
        } else {
            old_columns = old_table->columns;
        }

        old_type = old_table->type;
    }

    if (type_id) {
        EcsArray *old_table_db = stage->tables;
        new_table = ecs_world_get_table(world, stage, type_id);

        if (old_table && old_table_db != stage->tables) {
            old_table = ecs_world_get_table(world, stage, info->type_id);
        }

        if (in_progress) {
            EcsTableColumn *columns = ecs_map_get(stage->data_stage, type_id);
            new_columns = columns;

            if (!new_columns) {
                EcsArray *type = ecs_type_get(world, stage, type_id);
                ecs_assert(type != NULL, ECS_INTERNAL_ERROR, NULL);
                new_columns = ecs_table_get_columns(world, stage, type);
            }

            new_index = ecs_table_insert(world, new_table, new_columns, entity);
            assert(new_index != -1);

            if (new_columns != columns) {
                ecs_map_set(stage->data_stage, type_id, new_columns);
            }
        } else {
            new_index = ecs_table_insert(
                world, new_table, new_table->columns, entity);

            new_columns = new_table->columns;
        }
    }

    if (old_type_id && type_id) {
        copy_row(new_table->type, new_columns, new_index, 
            old_type, old_columns, old_index);
    }

    if (type_id) {
        EcsRow new_row = (EcsRow){.type_id = type_id, .index = new_index};
        ecs_map_set64(entity_index, entity, ecs_from_row(new_row));
    } else {
        if (in_progress) {
            /* The entity must be kept in the stage index because otherwise the
             * merge doesn't know that it needs to merge data for the entity */
            ecs_map_set64(entity_index, entity, 0);
        } else {
            ecs_map_remove(entity_index, entity);
        }
    }

    if (!in_progress) {
        bool merged = false;

        /* Invoke the OnRemove callbacks when there are components to remove,
         * but only when not in progress. If we are currently in progress, the
         * OnRemove handlers will be invoked during the merge at the end of the
         * frame, to ensure that no data is cleaned up while we are still
         * iterating. 
         * 
         * Note that this action is performanced *after* the entity index has
         * been updated. An OnRemove action can itself contain operations that
         * update the entity index, so we need to make sure the index is up to
         * date before invoking the callback.
         */
        if (to_remove && old_index != -1) {
            merged = notify_post_merge(
                world, old_table, old_columns, old_index, 1, to_remove);
        }

        /* After the cleanup code has been invoked we can finally remove the
         * entity from the old table. If an OnRemove handler was invoked, it did
         * already trigger a merge. Since the entity index was already updated
         * to reflect the removed component, the merge will already have removed
         * the entity from the table. In that case, we won't have to do it
         * again. Additionally, if the entity was not stored in any table before
         * this commit, we also don't need to perform the delete. */
        if (!merged && old_type_id) {
            ecs_table_delete(world, old_table, old_index);
        }
    }

    /* After the entity has been created in the new table and the stage is
     * consistent again with the entity index, execute the OnAdd actions. */
    if (type_id) {
        if (to_add) {
            notify_pre_merge (
                world, new_table, new_columns, new_index, 1, to_add, 
                world->type_sys_add_index);

            copy_from_prefab(
                world, stage, new_table, entity, new_index, 1, type_id, to_add);
        }     
    }

    /* This is a crude way to signal to the world that we need to redetermine
     * the schedule for the worker threads */
    world->valid_schedule = false;

    return new_index;
}


/* -- Private functions -- */

bool ecs_notify(
    EcsWorld *world,
    EcsMap *index,
    EcsType type_id,
    EcsTable *table,
    EcsTableColumn *table_columns,
    int32_t offset,
    int32_t limit)
{
    EcsArray *systems = ecs_map_get(index, type_id);
    bool notified = false;

    if (systems) {
        EcsEntity *buffer = ecs_array_buffer(systems);
        uint32_t i, count = ecs_array_count(systems);

        for (i = 0; i < count; i ++) {
            notified |= ecs_notify_row_system(
                world, buffer[i], table->type, table_columns, offset, limit);
        }
    } 

    return notified;
}

void ecs_merge_entity(
    EcsWorld *world,
    EcsStage *stage,
    EcsEntity entity,
    EcsRow *staged_row)
{
    EcsRow old_row = {0};
    EcsTable *old_table = NULL;
    uint64_t old_row_64 = ecs_map_get64(world->main_stage.entity_index, entity);
    if (old_row_64) {
        old_row = ecs_to_row(old_row_64);
        old_table = ecs_world_get_table(world, stage, old_row.type_id);
    }

    EcsType to_remove = ecs_map_get64(stage->remove_merge, entity);
    EcsType staged_id = staged_row->type_id;
    EcsType type_id = ecs_type_merge(
        world, stage, old_row.type_id, staged_row->type_id, to_remove);

    EcsEntityInfo info = {
        .entity = entity,
        .table = old_table,
        .type_id = old_row.type_id,
        .index = old_row.index
    };

    uint32_t new_index = commit_w_type(
        world, &world->main_stage, &info, type_id, 0, to_remove);

    if (type_id && staged_id) {
        EcsTable *new_table = ecs_world_get_table(world, stage, type_id);
        assert(new_table != NULL);

        EcsTable *staged_table = ecs_world_get_table(world, stage, staged_id);
        EcsTableColumn *staged_columns = ecs_map_get(
            stage->data_stage, staged_row->type_id);

        copy_row( new_table->type, new_table->columns, new_index,
                  staged_table->type, staged_columns, staged_row->index); 
    }
}

/* -- Public functions -- */

EcsEntity ecs_clone(
    EcsWorld *world,
    EcsEntity entity,
    bool copy_value)
{
    ecs_assert(world != NULL, ECS_INVALID_PARAMETERS, NULL);

    EcsStage *stage = ecs_get_stage(&world);

    ecs_assert(!world->is_merging, ECS_INVALID_WHILE_MERGING, NULL);

    EcsEntity result = ++ world->last_handle;
    if (entity) {
        int64_t row64 = ecs_map_get64(world->main_stage.entity_index, entity);
        if (row64) {
            EcsRow row = ecs_to_row(row64);
            EcsType type_id = row.type_id;
            EcsEntityInfo info = {
                .entity = result,
                .type_id = 0,
                .index = row.index
            };

            commit_w_type(world, stage, &info, type_id, type_id, 0);

            if (copy_value) {
                EcsTable *from_table = ecs_world_get_table(world, stage, type_id);
                EcsTableColumn *to_columns = NULL, *from_columns = from_table->columns;
                EcsTable *to_table = ecs_world_get_table(world, stage, type_id);
                EcsRow to_row = {0};

                if (world->in_progress) {
                    to_columns = ecs_map_get(stage->data_stage, type_id);
                } else {
                    to_columns = to_table->columns;
                }

                to_row = ecs_to_row(ecs_map_get64(stage->entity_index, result));

                if (!to_table)
                    to_table = from_table;

                if (!to_columns)
                    to_columns = from_columns;

                if (!to_row.index)
                    to_row = ecs_to_row(ecs_map_get64(
                            world->main_stage.entity_index, result));

                copy_row(to_table->type, to_columns, to_row.index,
                    from_table->type, from_columns, row.index);

                /* A clone with value is equivalent to a set */
                ecs_notify(
                    world, world->type_sys_set_index, from_table->type_id, 
                    to_table, to_columns, to_row.index, 1);
            }
        }
    }

    return result;
}

EcsEntity _ecs_new(
    EcsWorld *world,
    EcsType type)
{
    ecs_assert(world != NULL, ECS_INVALID_PARAMETERS, NULL);

    EcsStage *stage = ecs_get_stage(&world);

    ecs_assert(!world->is_merging, ECS_INVALID_WHILE_MERGING, NULL);

    EcsEntity entity = ++ world->last_handle;
    if (type) {
        EcsEntityInfo info = {
            .entity = entity
        };

        commit_w_type(world, stage, &info, type, type, 0);
    }

    return entity;
}

EcsEntity _ecs_new_child(
    EcsWorld *world,
    EcsEntity parent,
    const char *name,
    EcsType type)
{
    ecs_assert(world != NULL, ECS_INVALID_PARAMETERS, NULL);

    EcsType TFullType = type;
    
    if (parent) {
        if (!ecs_has(world, parent, EcsContainer)) {
            ecs_add(world, parent, EcsContainer);
        }
        EcsType TParentType = ecs_type_from_entity(world, parent);
        TFullType = ecs_merge_type(world, FullType, ParentType, 0);
    }

    if (name) {
        TFullType = ecs_merge_type(world, FullType, EcsId, 0);
    }

    EcsEntity result = ecs_new(world, FullType);

    if (name) {
        ecs_set(world, result, EcsId, {name});
    }

    return result;
}

EcsEntity _ecs_new_w_count(
    EcsWorld *world,
    EcsType type,
    uint32_t count,
    EcsEntity *handles_out)
{
    ecs_assert(world != NULL, ECS_INVALID_PARAMETERS, NULL);

    EcsStage *stage = ecs_get_stage(&world);
    EcsEntity result = world->last_handle + 1;
    world->last_handle += count;
    
    ecs_assert(!world->is_merging, ECS_INVALID_WHILE_MERGING, NULL);

    if (type) {
        EcsTable *table = ecs_world_get_table(world, stage, type);
        uint32_t row = ecs_table_grow(world, table, table->columns, count, result);

        EcsMap *entity_index = stage->entity_index;

        uint32_t cur_index_count = ecs_map_count(entity_index);
        ecs_map_set_size(entity_index, cur_index_count + count);

        int i, cur_row = row;
        for (i = result; i < (result + count); i ++) {
            /* We need to commit each entity individually in order to populate
             * the entity index */

            EcsRow new_row = (EcsRow){.type_id = type, .index = cur_row};
            ecs_map_set64(entity_index, i, ecs_from_row(new_row));

            cur_row ++;
        }

        /* Now we can notify matching OnAdd row systems in bulk */
        notify_pre_merge(
            world, table, table->columns, row, count, 
            type, world->type_sys_add_index);
        
        /* Check if there are prefabs */
        copy_from_prefab(world, stage, table, result, row, count, type, type);
    } 
    
    if (handles_out) {
        int i;
        for (i = 0; i < count; i ++) {
            handles_out[i] = result + i;
        }
    }

    return result;
}

void ecs_delete(
    EcsWorld *world,
    EcsEntity entity)
{
    ecs_assert(world != NULL, ECS_INVALID_PARAMETERS, NULL);
    ecs_assert(entity != 0, ECS_INVALID_PARAMETERS, NULL);

    EcsStage *stage = ecs_get_stage(&world);
    bool in_progress = world->in_progress;

    if (!in_progress) {
        uint64_t row64;
        if (ecs_map_has(world->main_stage.entity_index, entity, &row64)) {
            EcsRow row = ecs_to_row(row64);
            EcsEntityInfo info = {
                .entity = entity,
                .type_id = row.type_id,
                .index = row.index,
                .table = ecs_world_get_table(world, stage, row.type_id)
            };

            commit_w_type(world, stage, &info, 0, 0, row.type_id);

            ecs_map_remove(world->main_stage.entity_index, entity);
        }
    } else {
        /* Mark components of the entity in the main stage as removed. This will
         * ensure that subsequent calls to ecs_has, ecs_get and ecs_empty will
         * behave consistently with the delete. */
        uint64_t row64 = ecs_map_get64(world->main_stage.entity_index, entity);
        if (row64) {
            EcsRow row = ecs_to_row(row64);
            ecs_map_set64(stage->remove_merge, entity, row.type_id);
        }

        /* Remove the entity from the staged index. Any added components while
         * in progress will be discarded as a result. */
        ecs_map_set64(stage->entity_index, entity, 0);
    }
}

EcsResult _ecs_add(
    EcsWorld *world,
    EcsEntity entity,
    EcsType type)
{
    ecs_assert(world != NULL, ECS_INVALID_PARAMETERS, NULL);
    EcsStage *stage = ecs_get_stage(&world);
    ecs_assert(!world->is_merging, ECS_INVALID_WHILE_MERGING, NULL);
    
    EcsMap *entity_index = stage->entity_index;
    EcsType dst_type = 0;
    EcsEntityInfo info = {.entity = entity};

    uint64_t row_64 = ecs_map_get64(entity_index, entity);
    if (row_64) {
        EcsRow row = ecs_to_row(row_64);
        info.table = ecs_world_get_table(world, stage, row.type_id);
        info.columns = info.table->columns;
        info.index = row.index;
        info.type_id = row.type_id;
        EcsArray *to_add = ecs_type_get(world, stage, type);
        dst_type = ecs_type_merge_arr(world, stage, info.table->type, to_add, NULL);
    } else {
        dst_type = type;
    }

    return commit_w_type(world, stage, &info, dst_type, type, 0);
}

EcsResult _ecs_remove(
    EcsWorld *world,
    EcsEntity entity,
    EcsType type)
{
    ecs_assert(world != NULL, ECS_INVALID_PARAMETERS, NULL);
    EcsStage *stage = ecs_get_stage(&world);
    ecs_assert(!world->is_merging, ECS_INVALID_WHILE_MERGING, NULL);

    EcsMap *entity_index = stage->entity_index;
    EcsType dst_type = 0;
    EcsEntityInfo info = {.entity = entity};

    uint64_t row_64 = ecs_map_get64(entity_index, entity);
    if (row_64) {
        EcsRow row = ecs_to_row(row_64);
        info.table = ecs_world_get_table(world, stage, row.type_id);
        info.columns = info.table->columns;
        info.index = row.index;
        info.type_id = row.type_id;
        EcsArray *to_remove = ecs_type_get(world, stage, type);
        dst_type = ecs_type_merge_arr(world, stage, info.table->type, NULL, to_remove);
    } else if (!world->in_progress) {
        return EcsOk;
    }

    return commit_w_type(world, stage, &info, dst_type, 0, type);
}

EcsResult ecs_adopt(
    EcsWorld *world,
    EcsEntity parent,
    EcsEntity child)
{    
    ecs_assert(world != NULL, ECS_INVALID_PARAMETERS, NULL);
    ecs_assert(!world->is_merging, ECS_INVALID_WHILE_MERGING, NULL);

    if (!ecs_has(world, parent, EcsContainer)) {
        ecs_add(world, parent, EcsContainer);
    }

    EcsType TParentType = ecs_type_from_entity(world, parent);

    return ecs_add(world, child, ParentType);
}

EcsResult ecs_orphan(
    EcsWorld *world,
    EcsEntity parent,
    EcsEntity child)
{
    ecs_assert(world != NULL, ECS_INVALID_PARAMETERS, NULL);

    EcsType TParentType = ecs_type_from_entity(world, parent);

    return ecs_remove(world, child, ParentType);    
}

void* _ecs_get_ptr(
    EcsWorld *world,
    EcsEntity entity,
    EcsType type)
{
    ecs_assert(world != NULL, ECS_INVALID_PARAMETERS, NULL);


    /* Get only accepts types that hold a single component */
    EcsEntity component = ecs_entity_from_type(world, type);

    EcsEntityInfo info;
    return get_ptr(world, entity, component, false, true, &info);
}

static
EcsEntity _ecs_set_ptr_intern(
    EcsWorld *world,
    EcsEntity entity,
    EcsType type,
    size_t size,
    void *ptr)
{
    ecs_assert(world != NULL, ECS_INVALID_PARAMETERS, NULL);
    ecs_assert(type != 0, ECS_INVALID_PARAMETERS, NULL);
    ecs_assert(ptr != NULL, ECS_INVALID_PARAMETERS, NULL);
    
    EcsEntityInfo info = {0};

    /* Set only accepts types that hold a single component */
    EcsEntity component = ecs_entity_from_type(world, type);

    /* If component hasn't been added to entity yet, add it */
    int *dst = get_ptr(world, entity, component, true, false, &info);
    if (!dst) {
        _ecs_add(world, entity, type);
        dst = get_ptr(world, entity, component, true, false, &info);
        if (!dst) {
            /* It is possible that an OnAdd system removed the component before
             * it could have been set */
            return entity;
        }
    }

#ifndef NDEBUG
    EcsEntityInfo cinfo = {0};
    EcsComponent *cdata = get_ptr(
        world, component, EEcsComponent, false, false, &cinfo);
    ecs_assert(cdata->size == size, ECS_INVALID_COMPONENT_SIZE, NULL);
#endif

    memcpy(dst, ptr, size);

    notify_pre_merge(
        world, info.table, info.columns, info.index, 1, type,
        world->type_sys_set_index);

    return entity;
}

EcsEntity _ecs_set_ptr(
    EcsWorld *world,
    EcsEntity entity,
    EcsType type,
    size_t size,
    void *ptr)
{
    ecs_assert(world != NULL, ECS_INVALID_PARAMETERS, NULL);
    ecs_assert(ptr != NULL, ECS_INVALID_PARAMETERS, NULL);

    /* If no entity is specified, create one */
    if (!entity) {
        entity = _ecs_new(world, type);
    }

    return _ecs_set_ptr_intern(world, entity, type, size, ptr);
}

EcsEntity _ecs_set_singleton_ptr(
    EcsWorld *world,
    EcsType type,
    size_t size,
    void *ptr)
{
    return _ecs_set_ptr_intern(world, 0, type, size, ptr);
}

bool _ecs_has(
    EcsWorld *world,
    EcsEntity entity,
    EcsType type)
{
    ecs_assert(world != NULL, ECS_INVALID_PARAMETERS, NULL);

    if (!type) {
        return true;
    }

    EcsStage *stage = ecs_get_stage(&world);
    EcsType entity_type = ecs_typeid(world, entity);
    return ecs_type_contains(world, stage, entity_type, type, true, true) != 0;
}

bool _ecs_has_any(
    EcsWorld *world,
    EcsEntity entity,
    EcsType type)
{
    ecs_assert(world != NULL, ECS_INVALID_PARAMETERS, NULL);

    if (!type) {
        return true;
    }

    EcsStage *stage = ecs_get_stage(&world);
    EcsType entity_type = ecs_typeid(world, entity);
    return ecs_type_contains(world, stage, entity_type, type, false, true);
}

bool ecs_contains(
    EcsWorld *world,
    EcsEntity parent,
    EcsEntity child)
{
    ecs_assert(world != NULL, ECS_INVALID_PARAMETERS, NULL);

    if (!parent || !child) {
        return false;
    }

    EcsType TParent = ecs_type_from_entity(world, parent);
    return ecs_has(world, child, Parent);
}

EcsEntity ecs_new_component(
    EcsWorld *world,
    const char *id,
    size_t size)
{
    ecs_assert(world != NULL, ECS_INVALID_PARAMETERS, NULL);
    assert(world->magic == ECS_WORLD_MAGIC);

    EcsEntity result = ecs_lookup(world, id);
    if (result) {
        return result;
    }

    result = _ecs_new(world, world->t_component);
    ecs_set(world, result, EcsComponent, {.size = size});
    ecs_set(world, result, EcsId, {id});

    return result;
}

const char* ecs_id(
    EcsWorld *world,
    EcsEntity entity)
{
    EcsId *id = ecs_get_ptr(world, entity, EcsId);
    if (id) {
        return *id;
    } else {
        return NULL;
    }
}

bool ecs_empty(
    EcsWorld *world,
    EcsEntity entity)
{
    ecs_assert(world != NULL, ECS_INVALID_PARAMETERS, NULL);

    uint64_t cur64 = ecs_map_get64(world->main_stage.entity_index, entity);

    if (world->in_progress) {
        EcsStage *stage = ecs_get_stage(&world);

        uint64_t to_add64 = ecs_map_get64(stage->entity_index, entity);
        uint64_t to_remove64 = ecs_map_get64(stage->remove_merge, entity);

        EcsRow cur = ecs_to_row(cur64);
        EcsRow to_add = ecs_to_row(to_add64);
        EcsRow to_remove = ecs_to_row(to_remove64);
        EcsType result = ecs_type_merge(world, stage, 
            cur.type_id, to_add.type_id, to_remove.type_id);

        return result == 0;   
    } else {
        return cur64 == 0;
    }
}

EcsEntity ecs_get_component(
    EcsWorld *world,
    EcsEntity entity,
    uint32_t index)
{
    ecs_assert(world != NULL, ECS_INVALID_PARAMETERS, NULL);

    EcsStage *stage = ecs_get_stage(&world);
    int64_t row64 = ecs_map_get64(stage->entity_index, entity);

    if (!row64) {
        return 0;
    }

    EcsRow row = ecs_to_row(row64);
    EcsArray *components = ecs_map_get(stage->type_index, row.type_id);
    EcsEntity *buffer = ecs_array_buffer(components);

    if (ecs_array_count(components) > index) {
        return buffer[index];
    } else {
        return 0;
    }
}

EcsType ecs_type_from_entity(
    EcsWorld *world,
    EcsEntity entity)
{
    ecs_assert(world != NULL, ECS_INVALID_PARAMETERS, NULL);
    EcsStage *stage = ecs_get_stage(&world);

    return ecs_type_from_handle(world, stage, entity, NULL);
}

EcsEntity ecs_entity_from_type(
    EcsWorld *world, 
    EcsType type_id)
{
    ecs_assert(world != NULL, ECS_INVALID_PARAMETERS, NULL);

    EcsArray *type = ecs_map_get(world->main_stage.type_index, type_id);
    if (!type) {
        ecs_abort(ECS_UNKNOWN_TYPE_ID, NULL);
    }

    /* If array contains n entities, it cannot be reduced to a single entity */
    if (ecs_array_count(type) != 1) {
        ecs_abort(ECS_TYPE_NOT_AN_ENTITY, NULL);
    }

    return ((EcsEntity*)ecs_array_buffer(type))[0];
}

EcsType ecs_typeid(
    EcsWorld *world,
    EcsEntity entity)
{
    ecs_assert(world != NULL, ECS_INVALID_PARAMETERS, NULL);
    EcsStage *stage = ecs_get_stage(&world);
    int64_t row64 = ecs_map_get64(stage->entity_index, entity);
    EcsRow row = ecs_to_row(row64);
    EcsType result = row.type_id;

    if (world->in_progress) {
        int64_t main_row64 = ecs_map_get64(world->main_stage.entity_index, entity);
        EcsType remove_type = ecs_map_get64(stage->remove_merge, entity);
        EcsRow main_row = ecs_to_row(main_row64);
        result = ecs_type_merge(world, stage, main_row.type_id, result, remove_type);
    }
    
    return result;
}
