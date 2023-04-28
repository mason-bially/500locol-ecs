#include <benchmark/benchmark.h>

#include "pico_ecs.h"
#include "../bench.hpp"

static ecs_id_t pos_man;
static ecs_id_t vel_man;
static ecs_id_t dat_man;

ecs_ret_t pico_updatePosition(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata) {
    (void)udata;

    for (int i = 0; i < entity_count; i++) {
        ecs_id_t id = entities[i];

        auto p = (PositionComponent*)ecs_get(ecs, id, pos_man);
        auto v = (VelocityComponent*)ecs_get(ecs, id, vel_man);
        
        updatePosition(*p, *v, dt);
    }

    return 0;
}
ecs_ret_t pico_updateComponents(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata) {
    (void)udata;
    (void)dt;

    for (int i = 0; i < entity_count; i++) {
        ecs_id_t id = entities[i];

        auto p = (PositionComponent*)ecs_get(ecs, id, pos_man);
        auto v = (VelocityComponent*)ecs_get(ecs, id, vel_man);
        auto d = (DataComponent*)ecs_get(ecs, id, dat_man);

        updateComponents(*p, *v, *d);
    }

    return 0;
}
ecs_ret_t pico_updateData(ecs_t* ecs, ecs_id_t* entities, int entity_count, ecs_dt_t dt, void* udata) {
    (void)udata;

    for (int i = 0; i < entity_count; i++) {
        ecs_id_t id = entities[i];

        auto d = (DataComponent*)ecs_get(ecs, id, dat_man);

        updateData(*d, dt);
    }

    return 0;
}

template<BenchmarkSettings bs>
static void picoecs_A(benchmark::State& state) {

    TimeDelta delta = {1.0F / 60.0F};
    std::vector<ecs_id_t> vec;
    vec.reserve(BMEntities * 1024);
    std::vector<ecs_id_t> out;
    vec.reserve(BMEntities / 2);

    bench_or_once<bs, BenchmarkSettings::Init>(state,
    [&] {
        auto ecs = ecs_new(BMEntities * 1024, NULL);

        pos_man = ecs_register_component(ecs, sizeof(PositionComponent));
        vel_man = ecs_register_component(ecs, sizeof(VelocityComponent));
        dat_man = ecs_register_component(ecs, sizeof(DataComponent));

        auto updatePosition = ecs_register_system(ecs, pico_updatePosition, NULL, NULL, NULL);
        ecs_require_component(ecs, updatePosition, pos_man);
        ecs_require_component(ecs, updatePosition, vel_man);

        auto updateComponents = ecs_register_system(ecs, pico_updateComponents, NULL, NULL, NULL);
        ecs_require_component(ecs, updateComponents, pos_man);
        ecs_require_component(ecs, updateComponents, vel_man);
        ecs_require_component(ecs, updateComponents, dat_man);

        auto updateData = ecs_register_system(ecs, pico_updateData, NULL, NULL, NULL);
        ecs_require_component(ecs, updateData, dat_man);

        bench_or_once<bs, BenchmarkSettings::Expand|BenchmarkSettings::Churn>(state,
        [&] {
            for (size_t i = 0; i < BMEntities; ++i) {
                ecs_id_t e = ecs_create(ecs);

                ecs_add(ecs, e, pos_man);
                if ((i & 3) == 0)
                    ecs_add(ecs, e, vel_man);
                if ((i & 8) == 0)
                    ecs_add(ecs, e, dat_man);

                if constexpr (bs.MainType == BenchmarkSettings::Churn)
                    vec.push_back(e);
            }

            if constexpr (bs.MainType == BenchmarkSettings::Churn) {
                std::sample(vec.begin(), vec.end(),
                    std::back_inserter(out), BMEntities / 2,
                    m_eng
                );
                for (auto e : out)
                    ecs_destroy(ecs, e);
                out.clear();
            }

            bench_or_once<bs, BenchmarkSettings::Update>(state,
            [&] {
                ecs_update_system(ecs, updatePosition, delta);
                ecs_update_system(ecs, updateComponents, delta);
                ecs_update_system(ecs, updateData, delta);
            });
        });
        
        ecs_free(ecs);
        ecs = nullptr;
    });
}

BENCHMARK(picoecs_A<BsUpdate>);
BENCHMARK(picoecs_A<BsInit>);
BENCHMARK(picoecs_A<BsExpand>);
BENCHMARK(picoecs_A<BsChurn>);


#define PICO_ECS_MAX_SYSTEMS 16
#define PICO_ECS_MAX_COMPONENTS 64
#define PICO_ECS_IMPLEMENTATION
#include "pico_ecs.h"
