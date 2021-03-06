#include <include/api.h>

static
void Iter(EcsRows *rows) {
    Position *p = ecs_column(rows, Position, 1);
    Velocity *v = ecs_column_test(rows, Velocity, 2);
    Mass *m = ecs_column_test(rows, Mass, 3);

    int *param = rows->param;

    ProbeSystem(rows);

    int i;
    for (i = rows->begin; i < rows->end; i ++) {
        p[i].x = 10;
        p[i].y = 20;

        if (param) {
            p[i].x += *param;
            p[i].y += *param;
        }

        if (v) {
            v[i].x = 30;
            v[i].y = 40;
        }

        if (m) {
            m[i] = 50;
        }
    }
}

void Run_run() {
    EcsWorld *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);
    ECS_COMPONENT(world, Mass);
    ECS_COMPONENT(world, Rotation);

    ECS_ENTITY(world, e_1, Position, Velocity);
    ECS_ENTITY(world, e_2, Position, Velocity);
    ECS_ENTITY(world, e_3, Position, Velocity);
    ECS_ENTITY(world, e_4, Position, Velocity, Mass);
    ECS_ENTITY(world, e_5, Position, Velocity, Mass);
    ECS_ENTITY(world, e_6, Position, Velocity, Rotation);
    ECS_ENTITY(world, e_7, Position);

    ECS_SYSTEM(world, Iter, EcsManual, Position, Velocity);

    SysTestData ctx = {0};
    ecs_set_context(world, &ctx);

    /* Ensure system is not run by ecs_progress */
    ecs_progress(world, 1);
    test_int(ctx.invoked, 0);

    test_int( ecs_run(world, Iter, 1.0, NULL), 0);

    test_int(ctx.count, 6);
    test_int(ctx.invoked, 3);
    test_int(ctx.system, Iter);
    test_int(ctx.column_count, 2);
    test_null(ctx.param);

    test_int(ctx.e[0], e_1);
    test_int(ctx.e[1], e_2);
    test_int(ctx.e[2], e_3);
    test_int(ctx.e[3], e_4);
    test_int(ctx.e[4], e_5);
    test_int(ctx.e[5], e_6);

    int i;
    for (i = 0; i < ctx.invoked; i ++) {
        test_int(ctx.c[i][0], EPosition);
        test_int(ctx.s[i][0], 0);
        test_int(ctx.c[i][1], EVelocity);
        test_int(ctx.s[i][1], 0);
    }

    for (i = 0; i < ctx.count; i ++) {
        Position *p = ecs_get_ptr(world, ctx.e[i], Position);
        test_int(p->x, 10);
        test_int(p->y, 20);
        Velocity *v = ecs_get_ptr(world, ctx.e[i], Velocity);
        test_int(v->x, 30);
        test_int(v->y, 40);        
    }

    ecs_fini(world);
}

void Run_run_w_param() {
    EcsWorld *world = ecs_init();

    ECS_COMPONENT(world, Position);

    ECS_ENTITY(world, e_1, Position);

    ECS_SYSTEM(world, Iter, EcsManual, Position);

    SysTestData ctx = {0};
    ecs_set_context(world, &ctx);

    /* Ensure system is not run by ecs_progress */
    ecs_progress(world, 1);
    test_int(ctx.invoked, 0);

    int param = 5;
    test_int( ecs_run(world, Iter, 1.0, &param), 0);

    test_int(ctx.count, 1);
    test_int(ctx.invoked, 1);
    test_int(ctx.system, Iter);
    test_int(ctx.column_count, 1);
    test_ptr(ctx.param, &param);

    test_int(ctx.e[0], e_1);
    test_int(ctx.c[0][0], EPosition);
    test_int(ctx.s[0][0], 0);

    Position *p = ecs_get_ptr(world, e_1, Position);
    test_int(p->x, 15);
    test_int(p->y, 25);

    ecs_fini(world);
}

void Run_run_w_offset() {
    EcsWorld *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);
    ECS_COMPONENT(world, Mass);
    ECS_COMPONENT(world, Rotation);

    ECS_ENTITY(world, e_1, Position, Velocity);
    ECS_ENTITY(world, e_2, Position, Velocity);
    ECS_ENTITY(world, e_3, Position, Velocity);
    ECS_ENTITY(world, e_4, Position, Velocity, Mass);
    ECS_ENTITY(world, e_5, Position, Velocity, Mass);
    ECS_ENTITY(world, e_6, Position, Velocity, Rotation);
    ECS_ENTITY(world, e_7, Position);

    ECS_SYSTEM(world, Iter, EcsManual, Position, Velocity);

    SysTestData ctx = {0};
    ecs_set_context(world, &ctx);

    /* Ensure system is not run by ecs_progress */
    ecs_progress(world, 1);
    test_int(ctx.invoked, 0);

    test_int( ecs_run_w_filter(world, Iter, 1.0, 2, 0, 0, NULL), 0);

    test_int(ctx.count, 4);
    test_int(ctx.invoked, 3);
    test_int(ctx.system, Iter);
    test_int(ctx.column_count, 2);
    test_null(ctx.param);

    test_int(ctx.e[0], e_3);
    test_int(ctx.e[1], e_4);
    test_int(ctx.e[2], e_5);
    test_int(ctx.e[3], e_6);

    int i;
    for (i = 0; i < ctx.invoked; i ++) {
        test_int(ctx.c[i][0], EPosition);
        test_int(ctx.s[i][0], 0);
        test_int(ctx.c[i][1], EVelocity);
        test_int(ctx.s[i][1], 0);
    }

    for (i = 0; i < ctx.count; i ++) {
        Position *p = ecs_get_ptr(world, ctx.e[i], Position);
        test_int(p->x, 10);
        test_int(p->y, 20);
        Velocity *v = ecs_get_ptr(world, ctx.e[i], Velocity);
        test_int(v->x, 30);
        test_int(v->y, 40);        
    }

    ecs_fini(world);
}

void Run_run_w_offset_skip_1_archetype() {
    EcsWorld *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);
    ECS_COMPONENT(world, Mass);
    ECS_COMPONENT(world, Rotation);

    ECS_ENTITY(world, e_1, Position, Velocity);
    ECS_ENTITY(world, e_2, Position, Velocity);
    ECS_ENTITY(world, e_3, Position, Velocity);
    ECS_ENTITY(world, e_4, Position, Velocity, Mass);
    ECS_ENTITY(world, e_5, Position, Velocity, Mass);
    ECS_ENTITY(world, e_6, Position, Velocity, Rotation);
    ECS_ENTITY(world, e_7, Position);

    ECS_SYSTEM(world, Iter, EcsManual, Position, Velocity);

    SysTestData ctx = {0};
    ecs_set_context(world, &ctx);

    /* Ensure system is not run by ecs_progress */
    ecs_progress(world, 1);
    test_int(ctx.invoked, 0);

    test_int( ecs_run_w_filter(world, Iter, 1.0, 3, 0, 0, NULL), 0);

    test_int(ctx.count, 3);
    test_int(ctx.invoked, 2);
    test_int(ctx.system, Iter);
    test_int(ctx.column_count, 2);
    test_null(ctx.param);

    test_int(ctx.e[0], e_4);
    test_int(ctx.e[1], e_5);
    test_int(ctx.e[2], e_6);

    int i;
    for (i = 0; i < ctx.invoked; i ++) {
        test_int(ctx.c[i][0], EPosition);
        test_int(ctx.s[i][0], 0);
        test_int(ctx.c[i][1], EVelocity);
        test_int(ctx.s[i][1], 0);
    }

    for (i = 0; i < ctx.count; i ++) {
        Position *p = ecs_get_ptr(world, ctx.e[i], Position);
        test_int(p->x, 10);
        test_int(p->y, 20);
        Velocity *v = ecs_get_ptr(world, ctx.e[i], Velocity);
        test_int(v->x, 30);
        test_int(v->y, 40);        
    }

    ecs_fini(world);
}

void Run_run_w_offset_skip_1_archetype_plus_one() {
    EcsWorld *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);
    ECS_COMPONENT(world, Mass);
    ECS_COMPONENT(world, Rotation);

    ECS_ENTITY(world, e_1, Position, Velocity);
    ECS_ENTITY(world, e_2, Position, Velocity);
    ECS_ENTITY(world, e_3, Position, Velocity);
    ECS_ENTITY(world, e_4, Position, Velocity, Mass);
    ECS_ENTITY(world, e_5, Position, Velocity, Mass);
    ECS_ENTITY(world, e_6, Position, Velocity, Rotation);
    ECS_ENTITY(world, e_7, Position);

    ECS_SYSTEM(world, Iter, EcsManual, Position, Velocity);

    SysTestData ctx = {0};
    ecs_set_context(world, &ctx);

    /* Ensure system is not run by ecs_progress */
    ecs_progress(world, 1);
    test_int(ctx.invoked, 0);

    test_int( ecs_run_w_filter(world, Iter, 1.0, 4, 0, 0, NULL), 0);

    test_int(ctx.count, 2);
    test_int(ctx.invoked, 2);
    test_int(ctx.system, Iter);
    test_int(ctx.column_count, 2);
    test_null(ctx.param);

    test_int(ctx.e[0], e_5);
    test_int(ctx.e[1], e_6);

    int i;
    for (i = 0; i < ctx.invoked; i ++) {
        test_int(ctx.c[i][0], EPosition);
        test_int(ctx.s[i][0], 0);
        test_int(ctx.c[i][1], EVelocity);
        test_int(ctx.s[i][1], 0);
    }

    for (i = 0; i < ctx.count; i ++) {
        Position *p = ecs_get_ptr(world, ctx.e[i], Position);
        test_int(p->x, 10);
        test_int(p->y, 20);
        Velocity *v = ecs_get_ptr(world, ctx.e[i], Velocity);
        test_int(v->x, 30);
        test_int(v->y, 40);        
    }

    ecs_fini(world);
}

void Run_run_w_offset_skip_2_archetypes() {
    EcsWorld *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);
    ECS_COMPONENT(world, Mass);
    ECS_COMPONENT(world, Rotation);

    ECS_ENTITY(world, e_1, Position, Velocity);
    ECS_ENTITY(world, e_2, Position, Velocity);
    ECS_ENTITY(world, e_3, Position, Velocity);
    ECS_ENTITY(world, e_4, Position, Velocity, Mass);
    ECS_ENTITY(world, e_5, Position, Velocity, Mass);
    ECS_ENTITY(world, e_6, Position, Velocity, Rotation);
    ECS_ENTITY(world, e_7, Position);

    ECS_SYSTEM(world, Iter, EcsManual, Position, Velocity);

    SysTestData ctx = {0};
    ecs_set_context(world, &ctx);

    /* Ensure system is not run by ecs_progress */
    ecs_progress(world, 1);
    test_int(ctx.invoked, 0);

    test_int( ecs_run_w_filter(world, Iter, 1.0, 5, 0, 0, NULL), 0);

    test_int(ctx.count, 1);
    test_int(ctx.invoked, 1);
    test_int(ctx.system, Iter);
    test_int(ctx.column_count, 2);
    test_null(ctx.param);

    test_int(ctx.e[0], e_6);

    test_int(ctx.c[0][0], EPosition);
    test_int(ctx.s[0][0], 0);
    test_int(ctx.c[0][1], EVelocity);
    test_int(ctx.s[0][1], 0);

    Position *p = ecs_get_ptr(world, e_6, Position);
    test_int(p->x, 10);
    test_int(p->y, 20);
    Velocity *v = ecs_get_ptr(world, e_6, Velocity);
    test_int(v->x, 30);
    test_int(v->y, 40);        

    ecs_fini(world);
}

void Run_run_w_limit_skip_1_archetype() {
    EcsWorld *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);
    ECS_COMPONENT(world, Mass);
    ECS_COMPONENT(world, Rotation);

    ECS_ENTITY(world, e_1, Position, Velocity);
    ECS_ENTITY(world, e_2, Position, Velocity);
    ECS_ENTITY(world, e_3, Position, Velocity);
    ECS_ENTITY(world, e_4, Position, Velocity, Mass);
    ECS_ENTITY(world, e_5, Position, Velocity, Mass);
    ECS_ENTITY(world, e_6, Position, Velocity, Rotation);
    ECS_ENTITY(world, e_7, Position);

    ECS_SYSTEM(world, Iter, EcsManual, Position, Velocity);

    SysTestData ctx = {0};
    ecs_set_context(world, &ctx);

    /* Ensure system is not run by ecs_progress */
    ecs_progress(world, 1);
    test_int(ctx.invoked, 0);

    test_int( ecs_run_w_filter(world, Iter, 1.0, 0, 5, 0, NULL), 0);

    test_int(ctx.count, 5);
    test_int(ctx.invoked, 2);
    test_int(ctx.system, Iter);
    test_int(ctx.column_count, 2);
    test_null(ctx.param);

    test_int(ctx.e[0], e_1);
    test_int(ctx.e[1], e_2);
    test_int(ctx.e[2], e_3);
    test_int(ctx.e[3], e_4);
    test_int(ctx.e[4], e_5);

    int i;
    for (i = 0; i < ctx.invoked; i ++) {
        test_int(ctx.c[i][0], EPosition);
        test_int(ctx.s[i][0], 0);
        test_int(ctx.c[i][1], EVelocity);
        test_int(ctx.s[i][1], 0);
    }

    for (i = 0; i < ctx.count; i ++) {
        Position *p = ecs_get_ptr(world, ctx.e[i], Position);
        test_int(p->x, 10);
        test_int(p->y, 20);
        Velocity *v = ecs_get_ptr(world, ctx.e[i], Velocity);
        test_int(v->x, 30);
        test_int(v->y, 40);        
    }

    ecs_fini(world);
}

void Run_run_w_limit_skip_1_archetype_minus_one() {
    EcsWorld *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);
    ECS_COMPONENT(world, Mass);
    ECS_COMPONENT(world, Rotation);

    ECS_ENTITY(world, e_1, Position, Velocity);
    ECS_ENTITY(world, e_2, Position, Velocity);
    ECS_ENTITY(world, e_3, Position, Velocity);
    ECS_ENTITY(world, e_4, Position, Velocity, Mass);
    ECS_ENTITY(world, e_5, Position, Velocity, Mass);
    ECS_ENTITY(world, e_6, Position, Velocity, Rotation);
    ECS_ENTITY(world, e_7, Position);

    ECS_SYSTEM(world, Iter, EcsManual, Position, Velocity);

    SysTestData ctx = {0};
    ecs_set_context(world, &ctx);

    /* Ensure system is not run by ecs_progress */
    ecs_progress(world, 1);
    test_int(ctx.invoked, 0);

    test_int( ecs_run_w_filter(world, Iter, 1.0, 0, 4, 0, NULL), 0);

    test_int(ctx.count, 4);
    test_int(ctx.invoked, 2);
    test_int(ctx.system, Iter);
    test_int(ctx.column_count, 2);
    test_null(ctx.param);

    test_int(ctx.e[0], e_1);
    test_int(ctx.e[1], e_2);
    test_int(ctx.e[2], e_3);
    test_int(ctx.e[3], e_4);

    int i;
    for (i = 0; i < ctx.invoked; i ++) {
        test_int(ctx.c[i][0], EPosition);
        test_int(ctx.s[i][0], 0);
        test_int(ctx.c[i][1], EVelocity);
        test_int(ctx.s[i][1], 0);
    }

    for (i = 0; i < ctx.count; i ++) {
        Position *p = ecs_get_ptr(world, ctx.e[i], Position);
        test_int(p->x, 10);
        test_int(p->y, 20);
        Velocity *v = ecs_get_ptr(world, ctx.e[i], Velocity);
        test_int(v->x, 30);
        test_int(v->y, 40);        
    }

    ecs_fini(world);
}

void Run_run_w_limit_skip_2_archetypes() {
    EcsWorld *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);
    ECS_COMPONENT(world, Mass);
    ECS_COMPONENT(world, Rotation);

    ECS_ENTITY(world, e_1, Position, Velocity);
    ECS_ENTITY(world, e_2, Position, Velocity);
    ECS_ENTITY(world, e_3, Position, Velocity);
    ECS_ENTITY(world, e_4, Position, Velocity, Mass);
    ECS_ENTITY(world, e_5, Position, Velocity, Mass);
    ECS_ENTITY(world, e_6, Position, Velocity, Rotation);
    ECS_ENTITY(world, e_7, Position);

    ECS_SYSTEM(world, Iter, EcsManual, Position, Velocity);

    SysTestData ctx = {0};
    ecs_set_context(world, &ctx);

    /* Ensure system is not run by ecs_progress */
    ecs_progress(world, 1);
    test_int(ctx.invoked, 0);

    test_int( ecs_run_w_filter(world, Iter, 1.0, 0, 3, 0, NULL), 0);

    test_int(ctx.count, 3);
    test_int(ctx.invoked, 1);
    test_int(ctx.system, Iter);
    test_int(ctx.column_count, 2);
    test_null(ctx.param);

    test_int(ctx.e[0], e_1);
    test_int(ctx.e[1], e_2);
    test_int(ctx.e[2], e_3);

    int i;
    for (i = 0; i < ctx.invoked; i ++) {
        test_int(ctx.c[i][0], EPosition);
        test_int(ctx.s[i][0], 0);
        test_int(ctx.c[i][1], EVelocity);
        test_int(ctx.s[i][1], 0);
    }

    for (i = 0; i < ctx.count; i ++) {
        Position *p = ecs_get_ptr(world, ctx.e[i], Position);
        test_int(p->x, 10);
        test_int(p->y, 20);
        Velocity *v = ecs_get_ptr(world, ctx.e[i], Velocity);
        test_int(v->x, 30);
        test_int(v->y, 40);        
    }

    ecs_fini(world);
}

void Run_run_w_offset_1_limit_max() {
    EcsWorld *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);
    ECS_COMPONENT(world, Mass);
    ECS_COMPONENT(world, Rotation);

    ECS_ENTITY(world, e_1, Position, Velocity);
    ECS_ENTITY(world, e_2, Position, Velocity);
    ECS_ENTITY(world, e_3, Position, Velocity);
    ECS_ENTITY(world, e_4, Position, Velocity, Mass);
    ECS_ENTITY(world, e_5, Position, Velocity, Mass);
    ECS_ENTITY(world, e_6, Position, Velocity, Rotation);
    ECS_ENTITY(world, e_7, Position);

    ECS_SYSTEM(world, Iter, EcsManual, Position, Velocity);

    SysTestData ctx = {0};
    ecs_set_context(world, &ctx);

    /* Ensure system is not run by ecs_progress */
    ecs_progress(world, 1);
    test_int(ctx.invoked, 0);

    test_int( ecs_run_w_filter(world, Iter, 1.0, 1, 5, 0, NULL), 0);

    test_int(ctx.count, 5);
    test_int(ctx.invoked, 3);
    test_int(ctx.system, Iter);
    test_int(ctx.column_count, 2);
    test_null(ctx.param);

    test_int(ctx.e[0], e_2);
    test_int(ctx.e[1], e_3);
    test_int(ctx.e[2], e_4);
    test_int(ctx.e[3], e_5);
    test_int(ctx.e[4], e_6);

    int i;
    for (i = 0; i < ctx.invoked; i ++) {
        test_int(ctx.c[i][0], EPosition);
        test_int(ctx.s[i][0], 0);
        test_int(ctx.c[i][1], EVelocity);
        test_int(ctx.s[i][1], 0);
    }

    for (i = 0; i < ctx.count; i ++) {
        Position *p = ecs_get_ptr(world, ctx.e[i], Position);
        test_int(p->x, 10);
        test_int(p->y, 20);
        Velocity *v = ecs_get_ptr(world, ctx.e[i], Velocity);
        test_int(v->x, 30);
        test_int(v->y, 40);        
    }

    ecs_fini(world);
}

void Run_run_w_offset_1_limit_minus_1() {
    EcsWorld *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);
    ECS_COMPONENT(world, Mass);
    ECS_COMPONENT(world, Rotation);

    ECS_ENTITY(world, e_1, Position, Velocity);
    ECS_ENTITY(world, e_2, Position, Velocity);
    ECS_ENTITY(world, e_3, Position, Velocity);
    ECS_ENTITY(world, e_4, Position, Velocity, Mass);
    ECS_ENTITY(world, e_5, Position, Velocity, Mass);
    ECS_ENTITY(world, e_6, Position, Velocity, Rotation);
    ECS_ENTITY(world, e_7, Position);

    ECS_SYSTEM(world, Iter, EcsManual, Position, Velocity);

    SysTestData ctx = {0};
    ecs_set_context(world, &ctx);

    /* Ensure system is not run by ecs_progress */
    ecs_progress(world, 1);
    test_int(ctx.invoked, 0);

    test_int( ecs_run_w_filter(world, Iter, 1.0, 1, 4, 0, NULL), 0);

    test_int(ctx.count, 4);
    test_int(ctx.invoked, 2);
    test_int(ctx.system, Iter);
    test_int(ctx.column_count, 2);
    test_null(ctx.param);

    test_int(ctx.e[0], e_2);
    test_int(ctx.e[1], e_3);
    test_int(ctx.e[2], e_4);
    test_int(ctx.e[3], e_5);

    int i;
    for (i = 0; i < ctx.invoked; i ++) {
        test_int(ctx.c[i][0], EPosition);
        test_int(ctx.s[i][0], 0);
        test_int(ctx.c[i][1], EVelocity);
        test_int(ctx.s[i][1], 0);
    }

    for (i = 0; i < ctx.count; i ++) {
        Position *p = ecs_get_ptr(world, ctx.e[i], Position);
        test_int(p->x, 10);
        test_int(p->y, 20);
        Velocity *v = ecs_get_ptr(world, ctx.e[i], Velocity);
        test_int(v->x, 30);
        test_int(v->y, 40);        
    }

    ecs_fini(world);
}

void Run_run_w_offset_2_type_limit_max() {
    EcsWorld *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);
    ECS_COMPONENT(world, Mass);
    ECS_COMPONENT(world, Rotation);

    ECS_ENTITY(world, e_1, Position, Velocity);
    ECS_ENTITY(world, e_2, Position, Velocity);
    ECS_ENTITY(world, e_3, Position, Velocity);
    ECS_ENTITY(world, e_4, Position, Velocity, Mass);
    ECS_ENTITY(world, e_5, Position, Velocity, Mass);
    ECS_ENTITY(world, e_6, Position, Velocity, Rotation);
    ECS_ENTITY(world, e_7, Position);

    ECS_SYSTEM(world, Iter, EcsManual, Position, Velocity);

    SysTestData ctx = {0};
    ecs_set_context(world, &ctx);

    /* Ensure system is not run by ecs_progress */
    ecs_progress(world, 1);
    test_int(ctx.invoked, 0);

    test_int( ecs_run_w_filter(world, Iter, 1.0, 3, 3, 0, NULL), 0);

    test_int(ctx.count, 3);
    test_int(ctx.invoked, 2);
    test_int(ctx.system, Iter);
    test_int(ctx.column_count, 2);
    test_null(ctx.param);

    test_int(ctx.e[0], e_4);
    test_int(ctx.e[1], e_5);
    test_int(ctx.e[2], e_6);

    int i;
    for (i = 0; i < ctx.invoked; i ++) {
        test_int(ctx.c[i][0], EPosition);
        test_int(ctx.s[i][0], 0);
        test_int(ctx.c[i][1], EVelocity);
        test_int(ctx.s[i][1], 0);
    }

    for (i = 0; i < ctx.count; i ++) {
        Position *p = ecs_get_ptr(world, ctx.e[i], Position);
        test_int(p->x, 10);
        test_int(p->y, 20);
        Velocity *v = ecs_get_ptr(world, ctx.e[i], Velocity);
        test_int(v->x, 30);
        test_int(v->y, 40);        
    }

    ecs_fini(world);
}

void Run_run_w_offset_2_type_limit_minus_1() {
    EcsWorld *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);
    ECS_COMPONENT(world, Mass);
    ECS_COMPONENT(world, Rotation);

    ECS_ENTITY(world, e_1, Position, Velocity);
    ECS_ENTITY(world, e_2, Position, Velocity);
    ECS_ENTITY(world, e_3, Position, Velocity);
    ECS_ENTITY(world, e_4, Position, Velocity, Mass);
    ECS_ENTITY(world, e_5, Position, Velocity, Mass);
    ECS_ENTITY(world, e_6, Position, Velocity, Rotation);
    ECS_ENTITY(world, e_7, Position);

    ECS_SYSTEM(world, Iter, EcsManual, Position, Velocity);

    SysTestData ctx = {0};
    ecs_set_context(world, &ctx);

    /* Ensure system is not run by ecs_progress */
    ecs_progress(world, 1);
    test_int(ctx.invoked, 0);

    test_int( ecs_run_w_filter(world, Iter, 1.0, 3, 2, 0, NULL), 0);

    test_int(ctx.count, 2);
    test_int(ctx.invoked, 1);
    test_int(ctx.system, Iter);
    test_int(ctx.column_count, 2);
    test_null(ctx.param);

    test_int(ctx.e[0], e_4);
    test_int(ctx.e[1], e_5);

    int i;
    for (i = 0; i < ctx.invoked; i ++) {
        test_int(ctx.c[i][0], EPosition);
        test_int(ctx.s[i][0], 0);
        test_int(ctx.c[i][1], EVelocity);
        test_int(ctx.s[i][1], 0);
    }

    for (i = 0; i < ctx.count; i ++) {
        Position *p = ecs_get_ptr(world, ctx.e[i], Position);
        test_int(p->x, 10);
        test_int(p->y, 20);
        Velocity *v = ecs_get_ptr(world, ctx.e[i], Velocity);
        test_int(v->x, 30);
        test_int(v->y, 40);        
    }

    ecs_fini(world);
}

void Run_run_w_limit_1_all_offsets() {
    EcsWorld *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);
    ECS_COMPONENT(world, Mass);
    ECS_COMPONENT(world, Rotation);

    ECS_ENTITY(world, e_1, Position, Velocity);
    ECS_ENTITY(world, e_2, Position, Velocity);
    ECS_ENTITY(world, e_3, Position, Velocity);
    ECS_ENTITY(world, e_4, Position, Velocity, Mass);
    ECS_ENTITY(world, e_5, Position, Velocity, Mass);
    ECS_ENTITY(world, e_6, Position, Velocity, Rotation);
    ECS_ENTITY(world, e_7, Position);

    ECS_SYSTEM(world, Iter, EcsManual, Position, Velocity);

    SysTestData ctx = {0};
    ecs_set_context(world, &ctx);

    /* Ensure system is not run by ecs_progress */
    ecs_progress(world, 1);
    test_int(ctx.invoked, 0);

    /* Process entities out of order so we can validate whether it is correct */
    test_int( ecs_run_w_filter(world, Iter, 1.0, 0, 1, 0, NULL), 0);
    test_int( ecs_run_w_filter(world, Iter, 1.0, 2, 1, 0, NULL), 0);
    test_int( ecs_run_w_filter(world, Iter, 1.0, 1, 1, 0, NULL), 0);
    test_int( ecs_run_w_filter(world, Iter, 1.0, 3, 1, 0, NULL), 0);
    test_int( ecs_run_w_filter(world, Iter, 1.0, 5, 1, 0, NULL), 0);
    test_int( ecs_run_w_filter(world, Iter, 1.0, 4, 1, 0, NULL), 0);

    test_int(ctx.count, 6);
    test_int(ctx.invoked, 6);
    test_int(ctx.system, Iter);
    test_int(ctx.column_count, 2);
    test_null(ctx.param);

    test_int(ctx.e[0], e_1);
    test_int(ctx.e[1], e_3);
    test_int(ctx.e[2], e_2);
    test_int(ctx.e[3], e_4);
    test_int(ctx.e[4], e_6);
    test_int(ctx.e[5], e_5);

    int i;
    for (i = 0; i < ctx.invoked; i ++) {
        test_int(ctx.c[i][0], EPosition);
        test_int(ctx.s[i][0], 0);
        test_int(ctx.c[i][1], EVelocity);
        test_int(ctx.s[i][1], 0);
    }

    for (i = 0; i < ctx.count; i ++) {
        Position *p = ecs_get_ptr(world, ctx.e[i], Position);
        test_int(p->x, 10);
        test_int(p->y, 20);
        Velocity *v = ecs_get_ptr(world, ctx.e[i], Velocity);
        test_int(v->x, 30);
        test_int(v->y, 40);        
    }

    ecs_fini(world);
}

void Run_run_w_offset_out_of_bounds() {
    EcsWorld *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);
    ECS_COMPONENT(world, Mass);
    ECS_COMPONENT(world, Rotation);

    ECS_ENTITY(world, e_1, Position, Velocity);
    ECS_ENTITY(world, e_2, Position, Velocity);
    ECS_ENTITY(world, e_3, Position, Velocity);
    ECS_ENTITY(world, e_4, Position, Velocity, Mass);
    ECS_ENTITY(world, e_5, Position, Velocity, Mass);
    ECS_ENTITY(world, e_6, Position, Velocity, Rotation);
    ECS_ENTITY(world, e_7, Position);

    ECS_SYSTEM(world, Iter, EcsManual, Position, Velocity);

    SysTestData ctx = {0};
    ecs_set_context(world, &ctx);

    /* Ensure system is not run by ecs_progress */
    ecs_progress(world, 1);
    test_int(ctx.invoked, 0);

    test_int( ecs_run_w_filter(world, Iter, 1.0, 6, 1, 0, NULL), 0);

    test_int(ctx.count, 0);
    test_int(ctx.invoked, 0);

    ecs_fini(world);
}

void Run_run_w_limit_out_of_bounds() {
    EcsWorld *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);
    ECS_COMPONENT(world, Mass);
    ECS_COMPONENT(world, Rotation);

    ECS_ENTITY(world, e_1, Position, Velocity);
    ECS_ENTITY(world, e_2, Position, Velocity);
    ECS_ENTITY(world, e_3, Position, Velocity);
    ECS_ENTITY(world, e_4, Position, Velocity, Mass);
    ECS_ENTITY(world, e_5, Position, Velocity, Mass);
    ECS_ENTITY(world, e_6, Position, Velocity, Rotation);
    ECS_ENTITY(world, e_7, Position);

    ECS_SYSTEM(world, Iter, EcsManual, Position, Velocity);

    SysTestData ctx = {0};
    ecs_set_context(world, &ctx);

    /* Ensure system is not run by ecs_progress */
    ecs_progress(world, 1);
    test_int(ctx.invoked, 0);

    test_int( ecs_run_w_filter(world, Iter, 1.0, 5, 2, 0, NULL), 0);

    test_int(ctx.count, 1);
    test_int(ctx.invoked, 1);
    test_int(ctx.system, Iter);
    test_int(ctx.column_count, 2);
    test_null(ctx.param);

    test_int(ctx.e[0], e_6);

    int i;
    for (i = 0; i < ctx.invoked; i ++) {
        test_int(ctx.c[i][0], EPosition);
        test_int(ctx.s[i][0], 0);
        test_int(ctx.c[i][1], EVelocity);
        test_int(ctx.s[i][1], 0);
    }

    for (i = 0; i < ctx.count; i ++) {
        Position *p = ecs_get_ptr(world, ctx.e[i], Position);
        test_int(p->x, 10);
        test_int(p->y, 20);
        Velocity *v = ecs_get_ptr(world, ctx.e[i], Velocity);
        test_int(v->x, 30);
        test_int(v->y, 40);        
    }

    ecs_fini(world);
}

void Run_run_w_component_filter() {
    EcsWorld *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);
    ECS_COMPONENT(world, Mass);
    ECS_COMPONENT(world, Rotation);

    ECS_ENTITY(world, e_1, Position, Velocity);
    ECS_ENTITY(world, e_2, Position, Velocity);
    ECS_ENTITY(world, e_3, Position, Velocity);
    ECS_ENTITY(world, e_4, Position, Velocity, Mass);
    ECS_ENTITY(world, e_5, Position, Velocity, Mass);
    ECS_ENTITY(world, e_6, Position, Velocity, Rotation);
    ECS_ENTITY(world, e_7, Position);

    ECS_SYSTEM(world, Iter, EcsManual, Position, Velocity);

    SysTestData ctx = {0};
    ecs_set_context(world, &ctx);

    /* Ensure system is not run by ecs_progress */
    ecs_progress(world, 1);
    test_int(ctx.invoked, 0);

    test_int( ecs_run_w_filter(world, Iter, 1.0, 0, 0, Mass, NULL), 0);

    test_int(ctx.count, 2);
    test_int(ctx.invoked, 1);
    test_int(ctx.system, Iter);
    test_int(ctx.column_count, 2);
    test_null(ctx.param);

    test_int(ctx.e[0], e_4);
    test_int(ctx.e[1], e_5);

    int i;
    for (i = 0; i < ctx.invoked; i ++) {
        test_int(ctx.c[i][0], EPosition);
        test_int(ctx.s[i][0], 0);
        test_int(ctx.c[i][1], EVelocity);
        test_int(ctx.s[i][1], 0);
    }

    for (i = 0; i < ctx.count; i ++) {
        Position *p = ecs_get_ptr(world, ctx.e[i], Position);
        test_int(p->x, 10);
        test_int(p->y, 20);
        Velocity *v = ecs_get_ptr(world, ctx.e[i], Velocity);
        test_int(v->x, 30);
        test_int(v->y, 40);        
    }

    ecs_fini(world);
}

void Run_run_w_type_filter_of_2() {
    EcsWorld *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);
    ECS_COMPONENT(world, Mass);
    ECS_COMPONENT(world, Rotation);

    ECS_TYPE(world, Type, Mass, Rotation);

    ECS_ENTITY(world, e_1, Position, Velocity);
    ECS_ENTITY(world, e_2, Position, Velocity);
    ECS_ENTITY(world, e_3, Position, Velocity);
    ECS_ENTITY(world, e_4, Position, Velocity, Mass);
    ECS_ENTITY(world, e_5, Position, Velocity, Mass);
    ECS_ENTITY(world, e_6, Position, Velocity, Mass, Rotation);
    ECS_ENTITY(world, e_7, Position);

    ECS_SYSTEM(world, Iter, EcsManual, Position, Velocity);

    SysTestData ctx = {0};
    ecs_set_context(world, &ctx);

    /* Ensure system is not run by ecs_progress */
    ecs_progress(world, 1);
    test_int(ctx.invoked, 0);

    test_int( ecs_run_w_filter(world, Iter, 1.0, 0, 0, Type, NULL), 0);

    test_int(ctx.count, 1);
    test_int(ctx.invoked, 1);
    test_int(ctx.system, Iter);
    test_int(ctx.column_count, 2);
    test_null(ctx.param);

    test_int(ctx.e[0], e_6);

    int i;
    for (i = 0; i < ctx.invoked; i ++) {
        test_int(ctx.c[i][0], EPosition);
        test_int(ctx.s[i][0], 0);
        test_int(ctx.c[i][1], EVelocity);
        test_int(ctx.s[i][1], 0);
    }

    for (i = 0; i < ctx.count; i ++) {
        Position *p = ecs_get_ptr(world, ctx.e[i], Position);
        test_int(p->x, 10);
        test_int(p->y, 20);
        Velocity *v = ecs_get_ptr(world, ctx.e[i], Velocity);
        test_int(v->x, 30);
        test_int(v->y, 40);        
    }

    ecs_fini(world);
}

void Run_run_w_container_filter() {
    EcsWorld *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);
    ECS_COMPONENT(world, Mass);
    ECS_COMPONENT(world, Rotation);

    ECS_TYPE(world, Type, Mass, Rotation);

    ECS_ENTITY(world, e_1, Position, Velocity);
    ECS_ENTITY(world, e_2, Position, Velocity);
    ECS_ENTITY(world, e_3, Position, Velocity);
    ECS_ENTITY(world, e_4, Position, Velocity, Mass);
    ECS_ENTITY(world, e_5, Position, Velocity, Mass);
    ECS_ENTITY(world, e_6, Position, Velocity, Mass, Rotation);
    ECS_ENTITY(world, e_7, Position);

    ECS_SYSTEM(world, Iter, EcsManual, Position);

    SysTestData ctx = {0};
    ecs_set_context(world, &ctx);

    /* Create a parent entity */
    EcsEntity parent = ecs_new(world, 0);

    /* Adopt child entities */
    ecs_adopt(world, parent, e_1);
    ecs_adopt(world, parent, e_4);
    ecs_adopt(world, parent, e_6);
    ecs_adopt(world, parent, e_7);

    /* Get type from parent to use as filter */
    EcsType TParent = ecs_type_from_entity(world, parent);

    /* Ensure system is not run by ecs_progress */
    ecs_progress(world, 1);
    test_int(ctx.invoked, 0);

    test_int( ecs_run_w_filter(world, Iter, 1.0, 0, 0, Parent, NULL), 0);

    test_int(ctx.count, 4);
    test_int(ctx.invoked, 4);
    test_int(ctx.system, Iter);
    test_int(ctx.column_count, 1);
    test_null(ctx.param);

    test_int(ctx.e[0], e_6);
    test_int(ctx.e[1], e_7);
    test_int(ctx.e[2], e_1);
    test_int(ctx.e[3], e_4);

    int i;
    for (i = 0; i < ctx.invoked; i ++) {
        test_int(ctx.c[i][0], EPosition);
        test_int(ctx.s[i][0], 0);
    }

    for (i = 0; i < ctx.count; i ++) {
        Position *p = ecs_get_ptr(world, ctx.e[i], Position);
        test_int(p->x, 10);
        test_int(p->y, 20);       
    }

    ecs_fini(world);
}

void Run_run_no_match() {
    EcsWorld *world = ecs_init();

    ECS_COMPONENT(world, Position);
    ECS_COMPONENT(world, Velocity);

    ECS_ENTITY(world, e_1, Position);

    ECS_SYSTEM(world, Iter, EcsManual, Position, Velocity);

    SysTestData ctx = {0};
    ecs_set_context(world, &ctx);

    /* Ensure system is not run by ecs_progress */
    ecs_progress(world, 1);
    test_int(ctx.invoked, 0);

    test_int( ecs_run(world, Iter, 1.0, NULL), 0);

    test_int(ctx.count, 0);
    test_int(ctx.invoked, 0);

    ecs_fini(world);
}
