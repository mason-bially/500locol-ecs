
#include <iostream>
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

    World w;

    auto p = w.requireComponent<Position>();
    auto v = w.requireComponent<Velocity>();

    w.makeSystem("velocity", [=](World* w, auto c){
        for (auto& [e, v] : v->components) {
            p->doif(e, [=](auto& p) { p.x += v.x; });
        }
    });

    Entity e0 = w.newEntity();
    p->components[e0] = { 0.0, 3.0, -1.0 };
    Entity e1 = w.newEntity();
    p->components[e1] = { 0.0, 3.0, -1.0 };
    v->components[e1] = { 1.0, 0.0, 0.0 };
    Entity e2 = w.newEntity();
    v->components[e2] = { 1.0, 0.0, 0.0 };

    w.update();
    w.update();
    w.update();
    w.update();

    for (auto e : w.allEntities()) {
        std::cout << e << ": ";
        p->doif(e, [](auto p) { std::cout << p.x; });
        v->doif(e, [](auto v) { std::cout << " has V!"; });
        std::cout << std::endl;
    }

    return 0;
}
