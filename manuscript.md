# DSECS: Dead Simple Entity-Component-System framework.

Skip to [Philosophy](#Philosophy) for the currently edited section. Beware wandering past [THE LINE](#the-line).

## Background

### Parable

Once upon a time computing was simple. The world was stable, the clocks always ran at the same speed, things took the same amount of time to do regardless of what you were doing.

Then we got 

TODO, reference https://scholar.harvard.edu/files/mickens/files/theslowwinter.pdf

### Why? (And History)

The original impetus of the ECS design was a performance one.

Discuss performance costs of cache prefetch

### Philosophy

##### Structure of Arrays

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

##### Dispatch of Iteration

An important related corollary to the AoS vs. SoA dichotomy is the Iteration of Dispatches (IoD) versus Dispatches of Iteration (DoL). Which are bit more esoteric and involved.

```c++
void SomeObject::update() { // virtual
    // the contents of this would differ by object type
    // e.g. some would have AI instead of input
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

Now lets actually implement an ECS, we'll start by implementing the Core trinity of ECS ideas—entity, component, and system—and then build up and improve from there iteratively.

### Entities

Entities are our equivalent of objects. Or more specifically, they are similar to *pointers* to objects. However, unlike pointers, they are opaque, making them actually more similar to a resource handle. From the view of ECS as an in memory database, entities would be unique row ids.

```c++
using Entity = uint64_t;
```

Well that was easy.

The choice of a 64 bit integer in this case is because that is the width of a register in most common architectures. We could have also used `size_t` or `uintptr_t` here for the same outcome—each for a different reason.

Of course it's not as simple as a type definition, we also need a way to allocate new ones.

```c++
class World {
        Entity _nextEntity = 1;
    public:
        auto newEntity() /* -> Entity */ { return _nextEntity++; }
        // Note: We will always use `auto` or `void` on the left of function definitions.
        //   Return types that need to be explicitly specified will be placed using arrow syntax on the right, as shown (though unneeded here).
};
```

Well, that was also pretty easy!

Note that we skip `0` simply so that we can use it as a "no entity" sentinel in the future (e.g. `null`), which we should probably formalize.

```c++
constexpr Entity NoEntity = 0;
```

Incidentally, due to our use of C++, this works already:

```c++
/*** EXAMPLE ***/

Entity e = findEntity("nothing"); // assuming this method existed
if (e) {
    // entity is not `NoEntity` e.g. it exists
}
```

### Components

Components are groups of data we wish an entity to have. From the view of the object oriented paradigm, this is similar to the composition pattern (which C++ supports directly through multiple inheritance).

For our purposes anything that acts like a value type (implements the rule of three/five/zero) will be usable as a component. As a consequence of C++'s design, any plain old data type (e.g classic C `struct`s) will automatically be a valid component.

Some user code of custom components might look like this:

```c++
/*** USER CODE ***/

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

Note that this would be user code and not library code. It would appear after the definition of the library, and our library cannot make assumptions of their existence.

What we then want from our ECS, for each user defined component, is something along the following lines.

```c++
/*** EXAMPLE ***/
#include <unordered_map>

class EgWorld {
    public:
        std::unordered_map<Entity, Position> positions;
        std::unordered_map<Entity, Velocity> velocities;
        std::unordered_map<Entity, Acceleration> accelerations;
}
```

A struct of arrays. We are using `std::unordered_map` in the place of an array for now—we'll come back to why this was the right data structure to choose later, when we optimize the storage aspect of our design. But for now we are merely concerned with the core minimum we need to have a working example. The critical aspect of our design here is the struct of arrays pattern, which is what allows our ECS to have strong performance gains.

The problem here is that we don't know what component types (e.g. `Position`) the user will want. We want users to be able to create arbitrary components using their own component definitions. To do this we will need to manage the storage of components at runtime, dynamically creating the structure containing the arrays. And because this will involve user defined input types, we'll have to use templates to do it.

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

These managers represent one member field (e.g. `std::unordered_map<Entity, Position> positions;`) in the dynamic structure.

The design of the "manager" here uses inheritance and virtual functions to allow manipulation of the actual components without the manipulating code knowing the actual types of the components. Currently the only thing we need the `ComponentManagerBase` for is being able to properly destroy all the component data when the world is deleted, this is automatically done by the compiler and standard library via the virtual destructors.

It's important to understand that we can do this because the *user* will always know the types of the data they want, and so they can simply access the concrete type. The instantiation of the concrete type will generate the necessary virtual functions using our template.

{{
TODO FOOTNOTE This design also leaves the door open for user customized component managers through template specialization.

```c++
/*** EXAMPLE ***/
template<>
struct ComponentManager<MySpecialComponent> : ComponentManagerBase {
    std::flat_map<Entity, TComp> values; // a data structure we will be using later

    virtual ~ComponentManager() = default;
};
```
}}

Now we can make the actual dynamic structure of arrays, and a function to help access it.

```c++
#include <memory>

class World {
        std::unordered_map<size_t, std::shared_ptr<ComponentManagerBase>> _components; // the dynamic structure
    public:
        template<typename TComp>
        auto requireComponent() -> std::shared_ptr<ComponentManager<TComp>> {
            auto key = typeid(TComp).hash_code();
            if (auto it = _components.find(key); it != _components.end()) // already exists
                // this static cast is safe because we indexed by typeid
                return std::static_pointer_cast<ComponentManager<TComp>>(it->second);
            auto res = std::make_shared<ComponentManager<TComp>>();
            _components[key] = res;
            return res;
        }
};
```

We use a single function to both make and retrieve our component managers. This way users can be confident in using any type as components without concern for if it was initialized at first or not.

{{
TODO FOOTNOTE `if(<var-def>; <condition>)` is a recent C++ addition that allows for the creation of `if`-block scoped variables that are used within the condition. The best way to understand the syntax here is to consider the similar `for` syntax `for(<var-def>; <condition>; <increment>)`. The `var-def` section behaves similarly in both keywords.
}}

{{
TODO FOOTNOTE Our use here of `typeid()` will require run time type information. It is the opinion of the authors that built in run time type information is not a notable burden if used appropriately, especially for smaller teams. And in this case it is also useful as a conventional way to communicate ideas, hence our use of it here. Any reflection system that can provide a unique ID for a type will suffice for the purposes of this book, feel free to use a custom one.
}}

This function is a relatively standard pattern of find, make if not extant, and return by using the iterator to prevent duplicate lookups while allowing us to check for existence. We use the hashcode of the component type as the key, and we use a shared pointer for safety and simplicity. Note that we return the exact type of the component manager, even if we have to downcast it, this is so the *user* can use the type specific `ComponentManager` interface allowing convenient direct access to the component values.

```c++
/*** USER CODE ***/

World world;

auto pos = world.requireComponent<Position>();
auto vel = world.requireComponent<Velocity>();
auto acl = world.requireComponent<Acceleration>();
```

The user can now access the components, every call to `world.requireComponent<Position>()` will return the same manager and data, so long as the world object is the same. We now have a very straight forward ECS interface through the exposed `std::unordered_map` container.

```c++
/*** USER CODE ***/

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
```

And with a quick helper method...

```c++
#include <ranges>

class World {
    public:
        auto allEntities() { return std::ranges::iota_view(1u, _nextEntity); }
};
```

... we can even start displaying some diagnostic information.

```c++
/*** USER CODE ***/

#include <iostream>
#include <format>

auto printAllEntities = [=,&world](){
    for (auto e : world.allEntities()) {
        std::cout << std::format("{:03}:   p<{:^8}>   v<{:^8}>", e,
            (pos->values.contains(e)) ? std::format("{}, {}", pos->values[e].x, pos->values[e].y) : "_, _",
            (vel->values.contains(e)) ? std::format("{}, {}", vel->values[e].x, vel->values[e].y) : "_, _"
        ) << std::endl;
    }
};
```

The above display method is notably inefficient. While the `.contains()` method is useful, it's also very inefficient as a check for the existence of a component if the point is to then *use* that component. Especially if we then lookup the component more than once (e.g. to access both it's properties), this is why we re-used the iterator in `requireComponent()`. However, since this is user code, this is actually a library problem (since we don't provide a more convenient method), so we'll have to work on fixing this up later.

### Systems

Systems are the chunks of logic we want our ECS to run. Specifically the logic that runs across the entire world, every frame, to every entity with the relevant information.

Logic that happens in response to triggers, like input or network events, are generally done outside of the world, using the library API and an external entity variable to "randomly access" the relevant components. And while plenty of ECS libraries do provide additional mechanisms for this, it is not the strength of the paradigm, and we will not be focusing on them much.

We will start with some quick library code, based on how we organized components earlier, to define what a system interface looks like:

```c++
struct SystemBase {
    virtual ~SystemBase() = default;

    virtual void update(class World* w) = 0; // `class World` is an inline forward declaration
};
```

We can start thinking about some user code, which might look like this, for example we might imagine the following:

```c++
/*** EXAMPLE CODE ***/

class VelocitySystem : SystemBase {
        std::shared_ptr<ComponentManager<Velocity>> _vel;
        std::shared_ptr<ComponentManager<Position>> _pos;
    public:
        VelocitySystem(class World* w)
            // we store these to skip the lookup every time
            : _vel(w->requireComponent<Velocity>()), _pos(w->requireComponent<Position>()) { }

        virtual void update(class World* w) override { // the dispatched method
            for (auto& [e, v] : _vel->values) { // the iteration
                if (_pos->values.contains(e)) {
                    auto& p = _pos->values[e];
                    p.x += v.x;
                    p.y += v.y;
                }
            }
        }
}
```

There are some issues with this user code, and we should be looking to simplify it with our library if we can. Notably the constructor and variable definitions are especially egregious examples of verbose boilerplate. But we also see a repeated lookup of our component values again, though we won't get to fixing that till later.

Now with user code, we can continue and think about what we want our ECS to generate.

```c++
/*** EXAMPLE CODE ***/

class Game {
        World world;

        /* constructor elided */

        VelocitySystem _velocitySystem;
        AccelerationSystem _accelerationSystem;
        Inputsystem _inputSystem;
    public:
        void update() {
            _velocitySystem.update(&world); // dispatch of iteration
            _accelerationSystem.update(&world); // dispatch of iteration
            _inputSystem.update(&world); // dispatch of iteration
        }
}
```

A dispatches of iteration. We could see how the user could then expand the game with new systems one at a time and add them to this list. However, what we actually want is for systems to be first-class objects within our library, such that the user doesn't have to write a bunch of boiler plate for each one. Because that's the job of a library.

To realize this we will repeat the same design pattern we just used for components, we will make this explicit structure into a dynamic list dispatches. Due to our declaration of `SystemBase` earlier this is trivial.

```c++
class World {
        std::vector<std::shared_ptr<SystemBase>> _systems;
    public:
        void update() {
            for (auto sys : _systems) { // iteration
                sys->update(this); // dispatch
            }
        }
}
```

We now have a dynamic dispatch list, ironically an iteration of dispatches (of iterations). The difference being that this iteration is fixed in size dependent on the design of the systems in question, regardless of how many game objects there are. Because the iteration of game objects is still inside our dispatched system `update()` calls.

Now it would be nice to remove the need for users to define an entire class just to write an update method. Thankfully modern C++ has a tool that allows us to manipulate single methods like that quite nicely, the lambda. The ideal user code would then be something along the lines of:

```c++
/*** USER CODE ***/

// these variables were made earlier when we added components
auto pos = world.requireComponent<Position>();
auto vel = world.requireComponent<Velocity>();

world.makeSystem([=](World* w) { //`[=]` is syntax for default value binding closed over variables
    for (auto& [e, v] : vel->values) {
        if (pos->values.contains(e)) {
            auto& p = pos->values[e];
            p.x += v.x;
            p.y += v.y;
        }
    }
});
```

Note all the boiler plate is gone through the use of the lambda value binding the smart pointers. This means each lambda will hold the smart pointer for as long as it exists, preventing us from needing to worry about the clean up order of systems and components (since components should never point at systems, there should be no cycles). The update code itself is exactly as it was, but without repeated boilerplate.

Enabling this library method will leverage a templated derived class quite similar to how we did components:

```c++
template<std::invocable<class World*> FExec>
struct SystemAnonymous : SystemBase {
    FExec execution;

    SystemAnonymous(FExec execution)
        : SystemBase(), execution(execution) { }
    virtual ~SystemAnonymous() = default;

    virtual void update(class World* w) override { execution(w); } // the actual dispatch
};

class World {
    public:
        template<std::invocable<World*> FExec>
        auto makeSystem(FExec exec) -> std::shared_ptr<SystemAnonymous<FExec>> {
            auto res = std::make_shared<SystemAnonymous<FExec>>(exec);
            _systems.emplace_back(res);
            return res;
        }
}
```

Hopefully nothing here is particularly surprising, except perhaps the usage of `std::invocable<World*>`, a C++20 `concept`. This feature of post-modern C++ helps ensure that we are getting exactly what we expect, something which can be invoked with a `World*` as the only argument. Otherwise this should appear as a relatively standard lambda wrapper and emplace to vector method.

We now have all the pieces required to update our world:

```c++
/*** USER CODE ***/

// update the world four times
world.update();
world.update();
world.update();
world.update();
```

### The Trinity Complete

And with that we have completed the basic trinity.

- [Current Library](01_trinity/dsecs_01.hpp)
- [Current Example](01_trinity/example_01.cpp)
- Lines of Code: `057loc`
- Update Performance: ~1.33x
- Insert Performance: ~3.20x

## Ergonomics I

We now have a working ECS, however there are a number of ways we could improve it. Ways to improve the ergonomics of it.

### With

The first thing to clean up is the verbose way by which we access components on entities. For reference of what this looks like:

```c++
for (auto& [e, v] : vel->values) {
    if (pos->values.contains(e)) {
        auto& p = pos->values[e];
        p.x += v.x;
        p.y += v.y;
    }
}
```

There are two problems with this. The first is that it's verbose, it takes us two lines to express the concept of "if the entity has a component, get a reference to it". The second is that it's inefficient, `.contains()` and the `.operator[]` both perform a lookup, instead of reusing one. We can easily fix both of these with a quick helper method:

```c++
struct ComponentManager : ComponentManagerBase {
    void with(Entity e, std::invocable<TComp&> auto chain) {
        if (auto it = values.find(e); it != values.end())
            chain(it->second); // reuse the found value
    }
};
```

The implicit use of the concept here saves us from needing the explicit `template` declaration. We didn't use it earlier in `.makeSystem()` because we needed to be able to explicitly reference the type in the template instantiation.

We can now re-write our code as follows:

```c++
for (auto& [e, v] : vel->values) {
    pos->with(e, [=](auto& p) {
        p.x += v.x;
        p.y += v.y;
    });
}
```

A line less, more concise, and more algorithmically efficient. A win on every front.

### Names

It would be convenient if we could refer to the various aspects of our ECS by names at runtime using strings.

We'll do entities first:

```c++
struct Name {
    std::string name;
};

class World {
    private:
        std::unordered_map<std::string_view, Entity> _entityNames;
    public:
        auto findEntity(std::string_view name) -> Entity {
            auto it = _entityNames.find(name);
            return (it != _entityNames.end()) ? it->second : NoEntity;
        }

        auto requireEntity(std::string_view name) -> Entity {
            // early exit if the name already exists
            // we don't attempt to reuse this lookup in this case because the string view in the index must come from the component for a safe lifetime.
            if (auto it = _entityNames.find(name); it != _entityNames.end())
                return it->second;
            
            auto e = newEntity();
            std::string_view name_str = (requireComponent<Name>()->values[e] = { std::string(name) }).name;
            return _entityNames[name_str] = e;
        }
};
```

Here, we use a component to store our name, because why not? The entire purpose of our paradigm is to allow for fine grained composition of data, and the names of our entities is a piece of data we want them to have.

Notice also our `_entityNames` that allows us to find and entity by name. This is effectively a database index on the column of names. It's worth noting that the `string_view` we use for this index is backed by the components they are indexing. Which prevents us from constructing the index entry "early".

Component names are next:

```c++
struct ComponentManagerBase {
    std::string name;

    ComponentManagerBase(std::string_view name)
        : name(name) { }
};
        
template<typename TComp>
struct ComponentManager : ComponentManagerBase {
    ComponentManager(std::string_view name)
        : ComponentManagerBase(name), values() { }
}

class World {
    public:
        auto requireComponent() -> std::shared_ptr<ComponentManager<TComp>> {
            /* ... */
            auto res = std::make_shared<ComponentManager<TComp>>(typeid(TComp).name());
            /* ... */
        }
};
```

A straight forward change to our constructors and base class, and the automatic determination of the name of the component using reflection. Unfortunately this reflection is unreliable and implementation dependent. However for our purposes we are mainly using these names for diagnostic purposes, which the implementation dependent nature does serve us for. Later we will see how to replace these with user determined names.

And finally, naming our systems:

```c++
struct SystemBase {
    std::string name;

    SystemBase(std::string_view name)
        : name(name) { }
};

template<std::invocable<class World*> FExec>
struct SystemManager : SystemBase {
    SystemManager(std::string_view name, FExec execution)
        : SystemBase(name), execution(execution) { }
};

class World {
    public:
        template<std::invocable<World*> FExec>
        auto makeSystem(std::string_view name, FExec exec) -> std::shared_ptr<SystemAnonymous<FExec>> {
            auto res = std::make_shared<SystemAnonymous<FExec>>(name, exec);
            _systems.emplace_back(res);
            return res;
        }
};
```

Another straight forward change to our constructors and a base object name. This time requiring our users to always name their systems. This is again primarily for display, and the names of the anonymous functions we are using in this case will rarely be useful.

```c++
// replacing our old definition of this system
world.makeSystem("velocity", [=](World* w) {
    for (auto& [e, v] : vel->values) {
        pos->with(e, [=](auto& p) {
            p.x += v.x;
            p.y += v.y;
        });
    }
});

// a new named entity
Entity foo = world.requireEntity("foo");
acl->values[foo] = { 1.0, 2.0 };
pos->values[foo] = { 3.0, 4.0 };
```

Now everything is named, but we can't really use it to display useful data yet. For that, we need...

### Component Printers (and Has)

Printers! Or, well formatters anyway, though they aren't quite that yet either. Basically we just want a way to render our components into strings.

```c++
struct ComponentManagerBase {
    virtual auto has(Entity e) const -> bool = 0;
    virtual auto str(Entity e) const -> std::string = 0;
};
```

We also need a way to know if an entity exists or not so that we can know whether the printer is even valid to call, hence we add the `has()` method.

Our implementation is going to require a bit of setup. We are going to use argument dependent lookup (ADL) to access the formatting method on our component types. And since C++ already has a convention for those, we will reuse it.

```c++
// just after our internal library struct
auto& operator<<(std::ostream& os, Name n) {
    return os << n.name;
}

// one of these for each of user structs, just after we defined them.
auto& operator<<(std::ostream& os, Position p) {
    return os << std::format("p<{:^+4}, {:^+4}>", p.x, p.y);
}
```

Now we need a way to detect it in our implementation to ensure we are safe to use it. For that we will use a quick concept.

```c++
template <typename T>
concept Streamable = requires(std::ostream &os, T value) {
    { os << value } -> std::convertible_to<std::ostream &>;
};
```

We now have all the pieces to implement our formatting method:

```c++
template<typename TComp>
struct ComponentManager : ComponentManagerBase {
    virtual auto has(Entity e) const -> bool override { return values.contains(e); }
    virtual auto str(Entity e) const -> std::string override {
        if (auto it = values.find(e); it != values.end())
            if constexpr (Streamable<TComp>) {
                std::stringstream ss;
                ss << it->second;
                return ss.str();
            } else
                return "<UNSTREAMABLE>";
        else
            return "<NULL>";
    }
};
```

For now we are avoiding the use of exceptions, and we instead return sentinel values. Our `if constexpr` allows us to make decisions about how our component manager will be structured at compile time, setting up our virtual function to dispatch to the stream formatter if available.

We now need one more helper method that we already had for entities:

```c++
class World {
    public:
        auto allComponents() { return _components | std::views::values; }
};
```

And now we can write diagnostics in user code:

```c++
std::cout << std::format("==== DIAGNOSE {:03} =====", foo) << std::endl;
for (auto c : world.allComponents()) {
    if (c->has(foo))
        std::cout << std::format("{:20} || {}", c->name, c->str(foo)) << std::endl;
}
```

### Systems Enable

Now we need to actually use our named systems for something interesting. In this case it will be enabling or disabling the system, allowing us to skip systems we don't care to run.

```c++
struct SystemBase {
    bool enable = true;
};
```

And then we'll define our helper methods again:

```c++
class World {
    public:
        auto allSystems() { return _systems | std::views::all; }

        auto findSystem(std::string_view name) -> std::shared_ptr<SystemBase> { 
            auto it = std::ranges::find_if(_systems, [&](auto s){ return s->name == name; });
            return (it != _systems.end()) ? *it : nullptr;
        }
};
```

And finally we need change our update our main update loop:

```c++
class World {
    public:
        void update() {
            for (auto sys : _systems | std::views::filter(&SystemBase::enable)) {
                sys->update(this);
            }
        }
};
```


Now we can turn our systems on and off as needed.

```c++
world.findSystem("acceleration")->enable = false;
world.update();
world.update();
world.update();
world.update();
```

### Kill

We have had the ability to remove components since our initial implementation.

```c++
acl->values.erase(foo);
```

To demonstrate a use case for this, we'll introduce a new component and system. Health.

```c++
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
```

An example value setup:

```c++
health->values[foo] = { 1.0, 500, -100 }; // full health unit, with 500 health, loosing 100 health every second (update in our case).
```

And the system for it that finally demonstrates our use of deleting a component.

```c++
world.makeSystem("health-tick", [=](World* w) {
    for (auto& [e, h] : health->values) {
        double health_point_pct = 1.0 / h.maximum;  // to prevent dividing multiple times
        h.current_pct += h.delta * health_point_pct;
        if (h.current_pct <= 0.5 * health_point_pct) // less than 0.5 health points, this is effectively our epsilon.
            health->values.erase(e); // our iterator is now invalidated, the h (and e!) above are now invalid because they were taken by reference.
    }
});
```

The above is perhaps a bit convoluted, but it has some nice properties. There are other valid designs with their own properties and users are encouraged to try their own. By using this solution I've left the naïve solution as a comparison for the reader to try themselves.

However, the part we are actually interested in is the "death" of our unit. This is of course a common pattern, being able to remove one's own component is an important paradigm, especially for temporary components like sound effects, animations, and other timed information. However in this situation we would also like to actually kill the whole entity, there are multiple ways of doing this, but for now we'll go for the simplest, which requires a new feature.

```c++
class World {
    public:
        void kill(Entity e) {
            for (auto c : allComponents())
                if (c->has(e))
                    c->del(e);
        }
};
```

We have another problem of course, the components need a way to delete things without access to the concrete type. Just another virtual method away:

```c++
    struct ComponentManagerBase {
        virtual void del(Entity e) = 0;
    };

    template<typename TComp>
    struct ComponentManager : ComponentManagerBase {
        virtual void del(Entity e) override { values.erase(e); }
    };
```

And we can now use kill in our `"health-tick"` system.

```c++
if (h.current_pct <= 0.5 * health_point_pct) // less than 0.5 health points, this is effectively our epsilon.
    w->kill(e); // our iterator is now invalidated, the h (and e!) above are now invalid because they were taken by reference.
```

We'll have to remember that this invalidates iterators on the entity. And deal with the issue in general at a later time.

### Ergonomics I Complete

And with that we have added a number of useful ergonomics improvements.

- [Current Library](0e_ergonomics1/dsecs_0e.hpp)
- [Current Example](0e_ergonomics1/example_0e.cpp)
- Lines of Code: `121loc`
- Update Performance: ~1.33x
- Insert Performance: ~3.20x

# —THE LINE—

- Everything above the line is working draft *at worst*. Everything beyond the line is rough draft *at best*.
- Everything above the line is *probably close* to the final text. Everything beyond the line is *probably trash* that I will delete or re-write.

<div align="left">
      <a href="https://www.youtube.com/watch?v=H8yQhXDquII">
         <img src="https://img.youtube.com/vi/H8yQhXDquII/0.jpg" style="width:100%;">
      </a>
</div>

## API Stability

Currently parts of our implementation implicitly form part of our API. To enable future improvements we'll need to stabilize our API. We will also take this opportunity to reorganize some aspects of it.

### Actual Component API

We have been moving towards this for a bit, but it's time to finally have an actual API for the components.

We already have a partial API in `has` and `del`. Of course one problem with them is that they are virtual functions, which will cause a performance penalty for their use.

```c++
template<typename TComp>
struct ComponentManager /* added */ final : ComponentManagerBase {
    virtual auto has(Entity e) const -> bool override /* added */ final { return values.contains(e); }
    virtual void del(Entity e) override /* added */ final { values.erase(e); }
}
```

By using final we ensure the compiler has all the hints we can give it that this virtual function should be de-virtualized when called on the derived type. Which is the type users will be using to interact directly with components. 

```c++
template<typename TComp>
struct ComponentManager final : ComponentManagerBase {
    auto get(Entity e) const -> TComp const& { return values.at(e); }
    auto mut(Entity e) -> TComp& { return values.at(e); }
    void set(Entity e, TComp&& v) { values.insert_or_assign(e, v); }
}
```

For the moment this interface is relatively simple. However some operations, especially `set()` and `del()`, may need additional functionality in the future. We also distinguish between `get()` and `mut()`, separating out the `at()` overload, so that we can make distinctions between possible writes and pure reads.

### Actual System API

Before we can implement improvements to our access to components, we first need to have an actual API for systems to communicate the components they will be accessing. Preferably this interface will not involve repeated expression of our intent.

Another issue is our inability to forward important information from our update call, like the delta time for the update.

To do this we will use 

## Archetypes

It's time to address the elephant in the room. This is slow.

```

```

### A New Manager Type

## Meta Entities

### The Name Refactor

### Freedom from Bases

### Entity Wrapper

{and names}

## Archetype Performance

### Inverting the Paradigm

## Ergonomics II

### Events

One last important form of ergonomics we could use are events. Ways to respond to various changes automatically. Now that we have an API, we can do this step.

Compare to Getters/Setters, ways to add custom behaviour in reaction to things.

### System Ordering

### Tags

### Indexes
