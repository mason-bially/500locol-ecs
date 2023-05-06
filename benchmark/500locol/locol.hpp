#include <benchmark/benchmark.h>

#include "../bench.hpp"

template<BenchmarkSettings bs, typename World>
inline void locol_bm_A(benchmark::State& state) {
    TimeDelta delta = {1.0F / 60.0F};
    std::unordered_set<uint64_t> set;
    set.reserve(BMEntities * 1024);
    std::vector<uint64_t> out;
    out.reserve(BMEntities / 2);

    bench_or_once<bs, BenchmarkSettings::Init>(state,
    [&] {
        World world;
        auto pos = world.template requireComponent<PositionComponent>();
        auto vel = world.template requireComponent<VelocityComponent>();
        auto dat = world.template requireComponent<DataComponent>();

        world.makeSystem("updatePosition", [=,&delta](World* w) {
            for (auto& [e, v] : vel->values) {
                pos->with(e, [=](auto& p) {
                    updatePosition(p, v, delta);
                });
            }
        });

        world.makeSystem("updateComponents", [=](World* w) {
            for (auto& [e, d] : dat->values) {
                if (pos->values.contains(e) && vel->values.contains(e)) {
                    auto& p = pos->values[e];
                    auto& v = vel->values[e];
                    
                    updateComponents(p, v, d);
                }
            }
        });

        world.makeSystem("updateData", [=,&delta](World* w) {
            for (auto& [e, d] : dat->values) {
                updateData(d, delta);
            }
        });

        bench_or_once<bs, BenchmarkSettings::Expand|BenchmarkSettings::Churn>(state,
        [&] {
            for (size_t i = 0; i < BMEntities; ++i) {
                auto e = world.newEntity();
                pos->values[e] = { };
                if ((i & 3) == 0)
                    vel->values[e] = { };
                if ((i & 8) == 0)
                    dat->values[e] = { };

                if constexpr (bs.MainType == BenchmarkSettings::Churn)
                    set.emplace(e);
            }

            if constexpr (bs.MainType == BenchmarkSettings::Churn) {
                std::sample(set.begin(), set.end(),
                    std::back_inserter(out), BMEntities / 2,
                    m_eng
                );
                for (auto e : out) {
                    world.kill(e);
                    set.erase(e);
                }
                out.clear();
            }

            bench_or_once<bs, BenchmarkSettings::Update>(state,
            [&] { world.update(); });
        });
    });
}
