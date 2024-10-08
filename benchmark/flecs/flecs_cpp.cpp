#include <benchmark/benchmark.h>

#include "flecs.h"
#include "../bench.hpp"

template<BenchmarkSettings bs>
static void flecs_cpp_A(benchmark::State& state) {

    TimeDelta delta = {1.0F / 60.0F};
    std::unordered_set<flecs::id_t> set;
    set.reserve(BMEntities * 1024);
    std::vector<flecs::id_t> out;
    out.reserve(BMEntities / 2);

    bench_or_once<bs, BenchmarkSettings::Init>(state,
    [&] {
        flecs::world world;
        world.system<PositionComponent, VelocityComponent>("updatePosition")
            .kind(flecs::OnUpdate)
            .iter([](flecs::iter it, PositionComponent *p, VelocityComponent *v) {
                for (int i : it) {
                    updatePosition(p[i], v[i], it.delta_time());
                }
            });
        world.system<PositionComponent, VelocityComponent, DataComponent>("updateComponents")
            .kind(flecs::OnUpdate)
            .iter([](flecs::iter it, PositionComponent *p, VelocityComponent *v, DataComponent *d) {
                for (int i : it) {
                    updateComponents(p[i], v[i], d[i]);
                }
            });
        world.system<DataComponent>("updateData")
            .kind(flecs::OnUpdate)
            .iter([](flecs::iter it, DataComponent *d) {
                for (int i : it) {
                    updateData(d[i], it.delta_time());
                }
            });

        bench_or_once<bs, BenchmarkSettings::Expand|BenchmarkSettings::Churn>(state,
        [&] {
            for (size_t i = 0; i < BMEntities; ++i) {
                auto e = world.entity();
                e.add<PositionComponent>();
                if ((i & 3) == 0)
                    e.add<VelocityComponent>();
                if ((i & 8) == 0)
                    e.add<DataComponent>();

                if constexpr (bs.MainType == BenchmarkSettings::Churn)
                    set.emplace(e);
            }

            if constexpr (bs.MainType == BenchmarkSettings::Churn) {
                std::sample(set.begin(), set.end(),
                    std::back_inserter(out), BMEntities / 2,
                    m_eng
                );
                for (auto e : out) {
                    (flecs::entity{world, e}).destruct();
                    set.erase(e);
                }
                out.clear();
            }

            bench_or_once<bs, BenchmarkSettings::Update>(state,
            [&] { world.progress(delta); });
        });
    });
}

BENCHMARK(flecs_cpp_A<BsUpdate>);
BENCHMARK(flecs_cpp_A<BsInit>);
BENCHMARK(flecs_cpp_A<BsExpand>);//->(BMChurnIter);
BENCHMARK(flecs_cpp_A<BsChurn>);//->(BMChurnIter);
