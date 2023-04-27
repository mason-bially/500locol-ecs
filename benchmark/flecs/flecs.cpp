#include <benchmark/benchmark.h>

#include <flecs.h>
#include "../bench.hpp"

void flecs_updatePosition(ecs_iter_t *it) {
    PositionComponent *p = ecs_field(it, PositionComponent, 1);
    VelocityComponent *v = ecs_field(it, VelocityComponent, 2);

    for (int i = 0; i < it->count; i++) {
        updatePosition(p[i], v[i], it->delta_time);
    }
}
void flecs_updateComponents(ecs_iter_t *it) {
    PositionComponent *p = ecs_field(it, PositionComponent, 1);
    VelocityComponent *v = ecs_field(it, VelocityComponent, 2);
    DataComponent *d = ecs_field(it, DataComponent, 3);

    for (int i = 0; i < it->count; i++) {
        updateComponents(p[i], v[i], d[i]);
    }
}
void flecs_updateData(ecs_iter_t *it) {
    DataComponent *d = ecs_field(it, DataComponent, 1);

    for (int i = 0; i < it->count; i++) {
        updateData(d[i], it->delta_time);
    }
}

static void flecs_bm(benchmark::State& state) {
    ecs_world_t *world = ecs_init();

    ECS_COMPONENT(world, PositionComponent);
    ECS_COMPONENT(world, VelocityComponent);
    ECS_COMPONENT(world, DataComponent);

    TimeDelta delta = {1.0F / 60.0F};

    ECS_SYSTEM(world, flecs_updatePosition, EcsOnUpdate, PositionComponent, VelocityComponent);
    ECS_SYSTEM(world, flecs_updateComponents, EcsOnUpdate, PositionComponent, VelocityComponent, DataComponent);
    ECS_SYSTEM(world, flecs_updateData, EcsOnUpdate, DataComponent);

    for (size_t i = 0; i < BMEntities; ++i) {
        ecs_entity_t e = ecs_new_id(world);

        ecs_add(world, e, PositionComponent);
        ecs_add(world, e, VelocityComponent);
        if ((i & 8) == 0)
            ecs_add(world, e, DataComponent);
    }

    for (auto _ : state)
        ecs_progress(world, delta);
        
    ecs_fini(world);
}

BENCHMARK(flecs_bm);
