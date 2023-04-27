#include <benchmark/benchmark.h>

#include "../bench.hpp"

template<typename World>
inline void locol_bm(benchmark::State& state) {
    World world;

    auto pos = world.template requireComponent<PositionComponent>();
    auto vel = world.template requireComponent<VelocityComponent>();
    auto dat = world.template requireComponent<DataComponent>();

    TimeDelta delta = {1.0F / 60.0F};

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
            if (pos->values.contains(e), vel->values.contains(e)) {
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

    for (size_t i = 0; i < BMEntities; ++i) {
        auto e = world.newEntity();
        pos->values[e] = { };
        vel->values[e] = { };
        if ((i & 8) == 0)
            dat->values[e] = { };
    }

    for (auto _ : state)
        world.update();
}
