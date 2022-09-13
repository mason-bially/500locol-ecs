# DSECS: Dead Simple Entity-Component-System framework.

## Background

### Parable

Once upon a time computing was simple. The world was stable, the clocks always ran at the same speed, things took the same amount of time to do regardless of what you were doing.

Then we got 

TODO, reference https://scholar.harvard.edu/files/mickens/files/theslowwinter.pdf

### Why? (And History)

The original impetus of the ECS design was a performance one.

Discuss performance costs of cache prefetch

### Philosophy

One dichotomy that is useful to understand the idea of an ECS with is that of Arrays of Structs (AoS) versus Structs of Arrays (SoA). Which are exactly what they sound like.

```c++
struct Door {
    Material material;
    uint wallIndex;
    double width;
};

std::vector<Door> doors; // an array of structs
```

An array of structs is the common method of organizing data. This is generally because of objects, where an object is a large struct of varying size, and a list of pointers to objects being the array. The programming languages in common use provide many convince features for objects, which makes them an attractive choice. However as we saw earlier, they are not an optimal way to store data.

```c++
struct Doors {
    std::vector<Material> materials;
    std::vector<uint> wallIndexes;
    std::vector<double> widths;
};

Doors doors; // a struct of arrays
```

A struct of arrays provides better data locality. We can now iterate every value in a field more efferently, and using less memory.

It should be obvious that these examples are functionally equivalent, even if their representations are often quite different. Some programming languages are working to add features that make swapping between the two simpler. TODO expand on the idea of semantic complexity and equivalency and programming languages.

Astute readers may have noted a complication with our claim of equivalency: while *these* code examples may be equivalent, the use of objects (by way of a pointer) in an array is not so easily transformed into a SoA style. That is the point of an ECS runtime, specifically the **Components** aspect of it.

An important related corollary to the AoS vs. SoA dichotomy is the Iteration of Dispatches (IoD) versus Dispatches of Iteration (DoL). Which are bit more esoteric and involved.

```c++
void SomeObject::update() { // virtual
    // the contents of this would differ by object type
    this->physComp.updatePhysics();
    this->inputComp.updateInput();
    this->renderComp.draw();
}

void World::update() {
    for (auto obj : this->contents) {
        obj->update(); // a dispatch
    }
}
```

Here we see an Iteration of Dispatches in the form of a common object oriented game update loop. Every object has different code to run, so we iterate over them and dispatch to the function specific to that kind of object. This is a problem for our instruction cache and branch predictor, because the instructions being ran each time will probably be different. The dispatch is inside the loop, and so it is done many times.

```c++
void PhysicsWorld::update() { // virtual
    // the details of this would depend on each implementation
    for (auto phys_obj : this->contents) {
        phys_obj->updatePhysics();
    }
}

void World::update() {
    this->physWorld->update() // a dispatch
    this->inputWorld->update() // a dispatch
    this->render->draw() // a dispatch
}
```

Here we see a Dispatches of Iteration of what should be notable as roughly equivalent logic. Except now, each part of the world is running a single set of instructions. The loop is done inside of the dispatch, preventing it from being done many times.

Most readers will likely note at least one of the problems of calling these equivalent: that attaching a bunch of features together is now very complicated, that creating a new kind of behavior is now more complicated, that the memory management involved becomes much more complex in doing so. That is the point of an ECS runtime, specifically the **Systems** aspect of it.

## The Trinity

Now lets actually implement an ECS, we'll start by implementing the basic trinity of concepts, and then build up from there.

### Entities

Entities are our equivalent of objects. Or more specifically, they are similar to *pointers* to objects. However, unlike pointers, they are opaque, making them a form of resource handle. From the view of an ECS as an in memory database, they would be unique row ids.

```c++
using Entity = uint64_t;
```

Well that was easy.

The choice of a 64 bit integer in this case is because that is the width of a register in most common architectures. We could have also used `size_t` or `uintptr_t` here, each for different reasons.

Of course it's not that simple, we also need a way to allocate new ones.

```c++
class World {
        Entity _nextEntity = 1;
    public:
        Entity newEntity() { return _nextEntity++; }
};
```

Well, that was also pretty easy!

Note that we skip 0 simply so that we can use it as a "no entity" in the future, which we should probably formalize.

```c++
constexpr Entity NoEntity;
```

Incidentally, due to our use of C++, this works already:

```c++
Entity e = findEntity("nothing"); // assuming this method existed
if (e) {

}
```

### Components

Components are groups of data we wish an entity to have. In object oriented programming this is similar to the pattern of composition. For our purposes any plain old data object will be a valid component, as well as anything that acts like a value type (implements the rule of three/five/zero).

Some user code of components might look like this:

```c++
struct Position {
    float x, y; // could also be a vector value type, ala `glm::vec2`
};

struct Velocity {
    float x, y;
};

struct Acceleration {
    float x, y;
};
```

Note that this would be user code and not library code. What we want for each user defined component is something along the following lines.

```c++
#include <unordered_map>

class EgWorld {
    public:
        std::unordered_map<Entity, Position> positions;
        std::unordered_map<Entity, Velocity> velocities;
        std::unordered_map<Entity, Acceleration> accelerations;
}
```

A struct of arrays, and we are using `std::unordered_map` in the place of an array this time. We'll come back to why this is the right data structure, and optimizing it further later, but for now we are merely concerned with making this work.

The problem here is that we don't know what component types (the `Position`) the user will want. We want users to be able to create arbitrary components using their own `struct` definitions. To do this we will need to manage the storage of components at runtime, dynamically creating the structure containing the arrays. And because it involves user defined input types, we'll also need to use templates.

```c++
struct ComponentManagerBase {
    virtual ~ComponentManagerBase() = default;
};

template<typename TComp>
struct ComponentManager : ComponentManagerBase {
    std::unordered_map<Entity, TComp> values; // the actual array

    virtual ~ComponentManager() = default;
};
```

These managers represent one array field in the dynamic structure.

The design of the "manager" here uses inheritance and virtual functions to manage the actual components in a polymorphic way. Currently the only thing we need the `ComponentManagerBase` for is being able to properly destroy all the component data when the world is deleted. We can do this because the *user* will always know the types of the data they want, and so they can simply access the concrete type. This also leaves the door open for user customized component managers.

Now we can make the actual dynamic structure of arrays.

```c++
#include <memory>

class World {
        std::unordered_map<size_t, std::shared_ptr<ComponentManagerBase>> _components; // the dynamic structure
    public:
        template<typename TComp>
        auto requireComponent() -> std::shared_ptr<ComponentManager<TComp>> {
            auto key = typeid(TComp).hash_code();
            auto it = _components.find(key);
            if (it != _components.end())
                // this static cast is safe because we index by typeid
                return std::static_pointer_cast<ComponentManager<TComp>>(it->second);
            auto res = std::make_shared<ComponentManager<TComp>>();
            _components[key] = res;
            return res;
        }
};
```

We use a single function to both make and retrieve our component managers. This way users can be confident in using any type as components.

(TODO: footnote) One note is that our use of `typeid` will require run time type information. It is the opinion of the authors that built in run time type information is not a notable burden if used appropriately, especially for smaller teams. It's also useful as a standard way to communicate ideas, hence our use of it here. Any reflection system that can provide a unique ID for a type will suffice for the purposes of this book, feel free to use a custom one.

This function is a relatively standard pattern of find and return, or make, insert, and return; the iterator is used to prevent duplicate lookups. We use the hashcode of the type as the key, and we use a shared pointer for safety and simplicity. Note that we return the exact type of the component manager, even if we have to downcast it, this is so the *user* can use the type specific `ComponentManager` interface.

```c++
World world;

auto pos = world.requireComponent<Position>();
auto vel = world.requireComponent<Velocity>();
```

The user can now access the component managers, every `world.requireComponent<Position>()` will be the same manager and data, so long as the world object is the same. We now have a very straight forward ECS interface.

```c++
Entity e0 = world.newEntity();
pos->values[e0] = { 0.0, 3.0 };
Entity e1 = world.newEntity();
pos->values[e1] = { 0.0, 3.0 };
vel->values[e1] = { 1.0, 0.0 };
Entity e2 = world.newEntity();
vel->values[e2] = { 1.0, 0.0 };
```

And with a quick helper method...

```c++
#include <ranges>

class World {
    public:
        auto allEntities() { return std::ranges::iota_view(1u, _nextEntity); }
};
```

... we can even start displaying some information.

```c++
for (auto e : world.allEntities()) {
    std::cout << std::format("{:03}:   p<{:^8}>   v<{:^8}>", e,
        (pos->values.contains(e)) ? std::format("{}, {}", pos->values[e].x, pos->values[e].y) : "_, _",
        (vel->values.contains(e)) ? std::format("{}, {}", vel->values[e].x, vel->values[e].y) : "_, _"
    ) << std::endl;
}
```

The above display method is notably inefficient. While the `.contains()` method is useful, it's also very inefficient as a check for the existence of a component if the point is to then *use* that component. Especially if we then look up the component more than once, this is why we re-used the iterator in `requireComponent()`. However, since this is user code, this is actually a library problem (since we don't provide a more convenient method), so we'll work on fixing this up later.

### Systems

Systems are the chunks of logic we want our ECS to run. Specifically the logic that runs across the entire world. Logic that happens in response to triggers, like input or network events, are generally done outside of the world, using the API and an external entity variable to "randomly access" the relevant components. And while plenty of ECS libraries to provide mechanisms for this, it is not the strength of the paradigm, and we will not be adding them.

Some user code of systems might look like this:

```c++
class VelocitySystem : SystemBase {
        std::shared_ptr<ComponentManager<Velocity>> _vel;
        std::shared_ptr<ComponentManager<Position>> _pos;
    public:
        VelocitySystem(World* w)
            : _vel(w->requireComponent<Velocity>()), _pos(w->requireComponent<Position>()) { }

        virtual void update() override { // the dispatched method
            for (auto& [e, v] : _vel->values) { // the iteration
                if (_pos->values.contains(e)) {
                    auto p& = _pos->values[e];
                    p.x += v.x;
                    p.y += v.y;
                }
            }
        }
}
```

There are some issues with this user code, and we should be looking to simplify it with our library if we can. The constructor and variable definitions are especially egregious examples of boilerplate. But we also see a repeated lookup of our component values again. 

Anyway, what we want for the library is to then have something like this.

```c++
class EgWorld {
        VelocitySystem _velocitySystem;
        AccelerationSystem _accelerationSystem;
    public:
        void update() {
            _velocitySystem.update();
            _accelerationSystem.update();
        }
}
```

A dispatches of iteration. Again, like with components, we want to make this a dynamic dispatch list. We already have a way to do that, so we can start by simplifying this and defining our base class.

```c++
struct SystemBase {
    virtual ~SystemManagerBase() = default;

    virtual void update(class World* w) = 0;
};

class World {
        std::vector<std::shared_ptr<SystemBase>> _systems;
    public:
        void update() {
            for (auto sys : _systems) {
                sys->update(this);
            }
        }
}
```