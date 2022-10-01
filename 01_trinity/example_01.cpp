
#include <iostream>
#ifdef _MSC_VER
    #include <format>
#else // compatability support:
    #define FMT_HEADER_ONLY
    #include <fmt/format.h>
    namespace std { using namespace fmt; }
#endif
#include "dsecs_01.hpp"

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

    world.makeSystem([=](World* w) {
        for (auto& [e, a] : acl->values) {
            if (vel->values.contains(e)) {
                auto& v = vel->values[e];
                v.x += a.x;
                v.y += a.y;
            }
        }
    });

    world.makeSystem([=](World* w) {
        for (auto& [e, v] : vel->values) {
            if (pos->values.contains(e)) {
                auto& p = pos->values[e];
                p.x += v.x;
                p.y += v.y;
            }
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
