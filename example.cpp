
#include <iostream>
#include <format>
#include "dsecs.hpp"

struct Position
{
    float x, y, z;
};

struct Velocity
{
    float x, y, z;
};


int main()
{
    using namespace dsecs;

    World world;

    auto pos = world.requireComponent<Position>();
    auto vel = world.requireComponent<Velocity>();

    world.makeSystem("velocity", [=](World* w, auto c){
        for (auto& [e, v] : vel->values) {
            pos->doif(e, [=](auto& p) { p.x += v.x; });
        }
    });

    Entity e0 = world.newEntity();
    pos->values[e0] = { 0.0, 3.0, -1.0 };
    Entity e1 = world.newEntity();
    pos->values[e1] = { 0.0, 3.0, -1.0 };
    vel->values[e1] = { 1.0, 0.0, 0.0 };
    Entity e2 = world.newEntity();
    vel->values[e2] = { 1.0, 0.0, 0.0 };

    world.update();
    world.update();
    world.update();
    world.update();

    for (auto e : world.allEntities()) {
        std::cout << std::format("{:03}:   p<{:^12}>   v<{:^12}>", e,
            (pos->values.contains(e)) ? std::format("{}, {}, {}", pos->values[e].x, pos->values[e].y, pos->values[e].z) : "_, _, _",
            (vel->values.contains(e)) ? std::format("{}, {}, {}", vel->values[e].x, vel->values[e].y, vel->values[e].z) : "_, _, _"
        ) << std::endl;
    }

    return 0;
}
