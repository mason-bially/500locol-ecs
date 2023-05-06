#include <benchmark/benchmark.h>

#include <entt/entity/registry.hpp>
#include "../bench.hpp"

template<BenchmarkSettings bs>
static void entt_A(benchmark::State& state) {

    TimeDelta delta = {1.0F / 60.0F};
    std::unordered_set<entt::entity> set;
    set.reserve(BMEntities * 1024);
    std::vector<entt::entity> out;
    out.reserve(BMEntities / 2);

    bench_or_once<bs, BenchmarkSettings::Init>(state,
    [&] {
        entt::registry registry;

        auto enttUpdatePosition = [&]() {
            auto view = registry.view<PositionComponent, VelocityComponent>();
            for (auto e : view) {
                updatePosition(view.get<PositionComponent>(e), view.get<VelocityComponent>(e), delta);
            }
        };
        auto enttUpdateComponents = [&]() {
            auto view = registry.view<PositionComponent, VelocityComponent, DataComponent>();
            for (auto e : view) {
                updateComponents(view.get<PositionComponent>(e), view.get<VelocityComponent>(e), view.get<DataComponent>(e));
            }
        };
        auto enttUpdateData = [&]() {
            auto view = registry.view<DataComponent>();
            for (auto e : view) {
                updateData(view.get<DataComponent>(e), delta);
            }
        };

        bench_or_once<bs, BenchmarkSettings::Expand|BenchmarkSettings::Churn>(state,
        [&] {
            for (size_t i = 0; i < BMEntities; ++i) {
                auto e = registry.create();
                registry.emplace<PositionComponent>(e);
                if ((i & 3) == 0)
                    registry.emplace<VelocityComponent>(e);
                if ((i & 8) == 0)
                    registry.emplace<DataComponent>(e);

                if constexpr (bs.MainType == BenchmarkSettings::Churn)
                    set.emplace(e);
            }

            if constexpr (bs.MainType == BenchmarkSettings::Churn) {
                std::sample(set.begin(), set.end(),
                    std::back_inserter(out), BMEntities / 2,
                    m_eng
                );
                for (auto e : out)
                {
                    registry.destroy(e);
                    set.erase(e);
                }
                out.clear();
            }

            bench_or_once<bs, BenchmarkSettings::Update>(state,
            [&] {
                enttUpdatePosition();
                enttUpdateComponents();
                enttUpdateData();
            });
        });
    });
}

BENCHMARK(entt_A<BsUpdate>);
BENCHMARK(entt_A<BsInit>);
BENCHMARK(entt_A<BsExpand>)->Iterations(BMChurnIter);
BENCHMARK(entt_A<BsChurn>)->Iterations(BMChurnIter);
