#include <include/api.h>

void New_empty() {
    EcsWorld *world = ecs_init();

    EcsEntity e = ecs_new(world, 0);
    test_assert(e != 0);
    test_assert(ecs_empty(world, e));

    ecs_fini(world);
}

void New_component() {
    EcsWorld *world = ecs_init();

    ECS_COMPONENT(world, Position);

    EcsEntity e = ecs_new(world, Position);
    test_assert(e != 0);
    test_assert(ecs_has(world, e, Position));

    ecs_fini(world);
}

void New_type() {
    EcsWorld *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_TYPE(world, Type, Position);

    EcsEntity e = ecs_new(world, Type);
    test_assert(e != 0);
    test_assert(ecs_has(world, e, Position));
    
    ecs_fini(world);
}

void New_type_of_2() {
    EcsWorld *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);
    ECS_TYPE(world, Type, Position, Velocity);

    EcsEntity e = ecs_new(world, Type);
    test_assert(e != 0);
    test_assert(ecs_has(world, e, Position));
    
    ecs_fini(world);
}

void New_type_w_type() {
    EcsWorld *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_TYPE(world, Type_1, Position);
    ECS_TYPE(world, Type_2, Type_1);

    EcsEntity e = ecs_new(world, Type_2);
    test_assert(e != 0);
    test_assert(ecs_has(world, e, Position));
    
    ecs_fini(world);
}

void New_type_w_2_types() {
    EcsWorld *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

    ECS_TYPE(world, Type_1, Position);
    ECS_TYPE(world, Type_2, Velocity);
    ECS_TYPE(world, Type_3, Type_1, Type_2);

    EcsEntity e = ecs_new(world, Type_3);
    test_assert(e != 0);
    test_assert(ecs_has(world, e, Position));
    test_assert(ecs_has(world, e, Velocity));
    
    ecs_fini(world);
}

void New_type_mixed() {
    EcsWorld *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

    ECS_TYPE(world, Type_1, Position);
    ECS_TYPE(world, Type_2, Type_1, Velocity);

    EcsEntity e = ecs_new(world, Type_2);
    test_assert(e != 0);
    test_assert(ecs_has(world, e, Position));
    test_assert(ecs_has(world, e, Velocity));
    
    ecs_fini(world);
}

void New_tag() {
    EcsWorld *world = ecs_init();

    ECS_TAG(world, Tag);

    EcsEntity e = ecs_new(world, Tag);
    test_assert(e != 0);
    test_assert(ecs_has(world, e, Tag));
    
    ecs_fini(world);
}

void New_type_w_tag() {
    EcsWorld *world = ecs_init();

    ECS_TAG(world, Tag);

    ECS_TYPE(world, Type, Tag);

    EcsEntity e = ecs_new(world, Type);
    test_assert(e != 0);
    test_assert(ecs_has(world, e, Tag));
    
    ecs_fini(world);
}

void New_type_w_2_tags() {
    EcsWorld *world = ecs_init();

    ECS_TAG(world, Tag_1);
    ECS_TAG(world, Tag_2);

    ECS_TYPE(world, Type, Tag_1, Tag_2);

    EcsEntity e = ecs_new(world, Type);
    test_assert(e != 0);
    test_assert(ecs_has(world, e, Tag_1));
    test_assert(ecs_has(world, e, Tag_1));
    
    ecs_fini(world);
}

void New_type_w_tag_mixed() {
    EcsWorld *world = ecs_init();

    ECS_TAG(world, Tag);
    ECS_COMPONENT(world, Position);

    ECS_TYPE(world, Type, Position, Tag);

    EcsEntity e = ecs_new(world, Type);
    test_assert(e != 0);
    test_assert(ecs_has(world, e, Tag));
    test_assert(ecs_has(world, e, Position));
    
    ecs_fini(world);
}

