#include <include/api.h>

void ProbeSystem(
    EcsRows *rows) 
{
    SysTestData *ctx = ecs_get_context(rows->world);
    if (!ctx) {
        return;
    }

    ctx->system = rows->system;
    ctx->offset = rows->begin;
    ctx->column_count = rows->column_count;
    ctx->param = rows->param;

    int i;
    for (i = 0; i < ctx->column_count; i ++) {
        ctx->c[ctx->invoked][i] = rows->components[i];
        ctx->s[ctx->invoked][i] = ecs_column_source(rows, i + 1);

        /* Make sure ecs_column functions work */
        EcsType t = ecs_column_type(rows, i + 1);
        test_assert(t != 0);

        EcsEntity e = ecs_column_component(rows, i + 1);
        test_assert(e != 0);
    }

    EcsEntity *e = ecs_column_test(rows, EcsEntity, 0);
    if (e) {
        test_assert(e != NULL);
        test_assert(rows->entities != NULL);
        test_assert(rows->entities == e);
        for (i = rows->begin; i < rows->end; i ++) {
            ctx->e[i - rows->begin + ctx->count] = e[i];

            /* Make sure ecs_field works for all columns */
            int c;
            for (c = 0; c < ctx->column_count; c ++) {
                _ecs_field(rows, i, c, false);
            }
        }
        ctx->count += rows->count;
    }

    ctx->invoked ++;
}
