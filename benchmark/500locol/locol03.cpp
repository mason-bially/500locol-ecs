#include "locol.hpp"
#include "03_trinity/dsecs_03.hpp"

template<BenchmarkSettings bs>
static void locol03_A(benchmark::State& state) {
    using namespace dsecs03;

    TimeDelta delta = {1.0F / 60.0F};
    //std::unordered_set<uint64_t> set;
    //set.reserve(BMEntities * 1024);
    //std::vector<uint64_t> out;
    //out.reserve(BMEntities / 2);

    bench_or_once<bs, BenchmarkSettings::Init>(state,
    [&] {
        World world;
        auto pos = world.template requireComponent<PositionComponent>();
        auto vel = world.template requireComponent<VelocityComponent>();
        auto dat = world.template requireComponent<DataComponent>();

        world.makeSystem([=,&delta](World* w) {
            for (auto& [e, v] : vel->values) {
                if (pos->values.contains(e)) {
                    auto& p = pos->values[e];

                    updatePosition(p, v, delta);
                }
            }
        });

        world.makeSystem([=](World* w) {
            for (auto& [e, d] : dat->values) {
                if (pos->values.contains(e) && vel->values.contains(e)) {
                    auto& p = pos->values[e];
                    auto& v = vel->values[e];
                    
                    updateComponents(p, v, d);
                }
            }
        });

        world.makeSystem([=,&delta](World* w) {
            for (auto& [e, d] : dat->values) {
                updateData(d, delta);
            }
        });

        bench_or_once<bs, BenchmarkSettings::Expand>(state,
        [&] {
            for (size_t i = 0; i < BMEntities; ++i) {
                auto e = world.newEntity();
                pos->values[e] = { };
                if ((i & 3) == 0)
                    vel->values[e] = { };
                if ((i & 8) == 0)
                    dat->values[e] = { };
            }

            bench_or_once<bs, BenchmarkSettings::Update>(state,
            [&] { world.update(); });
        });
    });
}

BENCHMARK(locol03_A<BsUpdate>);
BENCHMARK(locol03_A<BsInit>);
BENCHMARK(locol03_A<BsExpand>);//->(BMChurnIter);
