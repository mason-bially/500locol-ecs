
#include <iostream>
#include "compat/format"
#include "dsecs_0e.hpp"

struct Position {
    float x, y;
};

auto& operator<<(std::ostream& os, Position p) {
    return os << std::format("p<{:^+4}, {:^+4}>", p.x, p.y);
}

struct Velocity {
    float x, y;
};

auto& operator<<(std::ostream& os, Velocity v) {
    return os << std::format("v<{:^+4}, {:^+4}>", v.x, v.y);
}

struct Acceleration {
    float x, y;
};

auto& operator<<(std::ostream& os, Acceleration a) {
    return os << std::format("a<{:^+4}, {:^+4}>", a.x, a.y);
}

// The current health status of a unit, the status is all the important parts of health as calculated from other systems.
// - maxium describes the current maximum value of health the unit can have as calculated from other sources.
// - current_pct describes the curren percentage of the maximum, note that health scaling falls out of this design (even if fixed damage amounts must be divided in).
// - delta describes the current change per second the unit is undergoing from accumulated status effects and base health regen and buffs and the like.
struct HealthStatus {
    double current_pct;
    uint32_t maximum;

    double delta;
};

auto& operator<<(std::ostream& os, HealthStatus h) {
    return os << std::format("health: {:.0F} / {:<7}", h.maximum * h.current_pct, h.maximum);
}

int main()
{
    using namespace dsecs0e;

    World world;

    auto pos = world.requireComponent<Position>();
    auto vel = world.requireComponent<Velocity>();
    auto acl = world.requireComponent<Acceleration>();

    auto health = world.requireComponent<HealthStatus>();

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

    world.makeSystem("health-tick", [=](World* w) {
        for (auto& [e, h] : health->values) {
            double health_point_pct = 1.0 / h.maximum;  // to prevent dividing multiple times
            h.current_pct += h.delta * health_point_pct;
            if (h.current_pct <= 0.5 * health_point_pct) // less than 0.5 health points, this is effectively our epsilon.
                w->kill(e); // our iterator is now invalidated, the h (and e!) above are now invalid because they were taken by reference.
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
    health->values[foo] = { 1.0, 500, -100 };

    auto printAll = [&] {
        std::cout << std::format("===== WORLD STATE =====", foo) << std::endl;
        for (auto e : world.allEntities()) {
            std::cout << std::format("{:03}:   p<{:^12}>   v<{:^12}>", e,
                (pos->values.contains(e)) ? std::format("{:^+4}, {:^+4}", pos->values[e].x, pos->values[e].y) : " _ ,   _",
                (vel->values.contains(e)) ? std::format("{:^+4}, {:^+4}", vel->values[e].x, vel->values[e].y) : " _ ,   _"
            ) << std::endl;
        }
    };

    auto printOne = [&](Entity e) {
        std::cout << std::format("===== DIAGNOSE {:03} =====", e) << std::endl;
        for (auto c : world.allComponents()) {
            if (c->has(e))
                std::cout << std::format("{:20} || {}", c->name, c->str(e)) << std::endl;
        }
    };

    printOne(foo);
    acl->values.erase(foo);
    printOne(foo);

    printAll();

    world.update();
    world.update();
    world.update();
    world.update();

    printOne(foo);
    printAll();

    world.findSystem("acceleration")->enable = false;
    world.update();
    world.update();
    world.update();
    world.update();

    printOne(foo);
    printAll();

    return 0;
}
