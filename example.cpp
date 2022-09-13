
#include <iostream>
#include <format>
#include "dsecs.hpp"

struct Position {
    float x, y;
};

struct Velocity {
    float x, y;
};

struct Acceleration {
    float x, y;
};

int main()
{
    using namespace dsecs;

    World world;

    auto pos = world.requireComponent<Position>();
    auto vel = world.requireComponent<Velocity>();
    auto acl = world.requireComponent<Acceleration>();

    world.makeSystem("acceleration", [=](World* w, auto c) {
        for (auto& [e, a] : acl->values) {
            vel->doif(e, [=](auto& v) { v.x += a.x; v.y += a.y; });
        }
    });

    world.makeSystem("velocity", [=](World* w, auto c) {
        for (auto& [e, v] : vel->values) {
            pos->doif(e, [=](auto& p) { p.x += v.x; p.y += v.y; });
        }
    });

    Entity e0 = world.newEntity();
    pos->values[e0] = { 0.0, -3.0 };
    Entity e1 = world.newEntity();
    pos->values[e1] = { 0.0, 3.0 };
    vel->values[e1] = { 1.0, 0.0 };
    Entity e2 = world.newEntity();
    vel->values[e2] = { 1.0, 0.0 };

    world.update();
    world.update();
    world.update();
    world.update();

    for (auto e : world.allEntities()) {
        std::cout << std::format("{:03}:   p<{:^12}>   v<{:^12}>", e,
            (pos->values.contains(e)) ? std::format("{:^+4}, {:^+4}", pos->values[e].x, pos->values[e].y) : " _ ,   _",
            (vel->values.contains(e)) ? std::format("{:^+4}, {:^+4}", vel->values[e].x, vel->values[e].y) : " _ ,   _"
        ) << std::endl;
    }

    return 0;
}
