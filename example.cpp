
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

    world.makeSystem("acceleration", [=](World* w) {
        for (auto& [e, a] : acl->values) {
            vel->with(e, [=](auto& v) {
                v.x += a.x;
                v.y += a.y;
            });
        }
    });

    world.makeSystem("velocity", [=](World* w) {
        for (auto& [e, v] : vel->values) {
            pos->with(e, [=](auto& p) {
                p.x += v.x;
                p.y += v.y;
            });
        }
    });

    Entity e0 = world.newEntity();
    pos->values[e0] = { 0.0, 3.0 };
    Entity e1 = world.newEntity();
    pos->values[e1] = { 0.0, 3.0 };
    vel->values[e1] = { 1.0, 0.0 };
    Entity e2 = world.newEntity();
    pos->values[e2] = { 0.0, 3.0 };
    vel->values[e2] = { 1.0, 0.0 };
    acl->values[e2] = { 0.0, 0.5 };
    Entity e3 = world.newEntity();
    vel->values[e3] = { 1.0, 0.0 };
    acl->values[e3] = { 0.0, 0.5 };

    Entity foo = world.requireEntity("foo");
    acl->values[foo] = { 1.0, 2.0 };
    pos->values[foo] = { 3.0, 4.0 };

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

    std::cout << std::format("==== DIAGNOSE {:03} =====", foo) << std::endl;
    for (auto c : world.allComponents()) {
        if (c->has(foo))
            std::cout << std::format("{:20}||{}", c->name, c->print(foo)) << std::endl;
    }

    return 0;
}
