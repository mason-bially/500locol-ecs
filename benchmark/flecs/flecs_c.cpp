#include <benchmark/benchmark.h>

#include "flecs.h"
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

template<BenchmarkSettings bs>
static void flecs_c_A(benchmark::State& state) {

    TimeDelta delta = {1.0F / 60.0F};
    std::vector<ecs_entity_t> vec;
    vec.reserve(BMEntities * 1024);
    std::vector<ecs_entity_t> out;
    vec.reserve(BMEntities / 2);

    bench_or_once<bs, BenchmarkSettings::Init>(state,
    [&] {
        ecs_world_t *world = ecs_init();

        ECS_COMPONENT(world, PositionComponent);
        ECS_COMPONENT(world, VelocityComponent);
        ECS_COMPONENT(world, DataComponent);

        ECS_SYSTEM(world, flecs_updatePosition, EcsOnUpdate, PositionComponent, VelocityComponent);
        ECS_SYSTEM(world, flecs_updateComponents, EcsOnUpdate, PositionComponent, VelocityComponent, DataComponent);
        ECS_SYSTEM(world, flecs_updateData, EcsOnUpdate, DataComponent);

        bench_or_once<bs, BenchmarkSettings::Expand|BenchmarkSettings::Churn>(state,
        [&] {
            for (size_t i = 0; i < BMEntities; ++i) {
                ecs_entity_t e = ecs_new_id(world);

                ecs_add(world, e, PositionComponent);
                if ((i & 3) == 0)
                    ecs_add(world, e, VelocityComponent);
                if ((i & 8) == 0)
                    ecs_add(world, e, DataComponent);

                if constexpr (bs.MainType == BenchmarkSettings::Churn)
                    vec.push_back(e);
            }

            if constexpr (bs.MainType == BenchmarkSettings::Churn) {
                std::sample(vec.begin(), vec.end(),
                    std::back_inserter(out), BMEntities / 2,
                    m_eng
                );
                for (auto e : out)
                    ecs_delete(world, e);
                out.clear();
            }

            bench_or_once<bs, BenchmarkSettings::Update>(state,
            [&] { ecs_progress(world, delta); });
        });
        
        ecs_fini(world);
    });
}

BENCHMARK(flecs_c_A<BsUpdate>);
BENCHMARK(flecs_c_A<BsInit>);
BENCHMARK(flecs_c_A<BsExpand>);
BENCHMARK(flecs_c_A<BsChurn>);
