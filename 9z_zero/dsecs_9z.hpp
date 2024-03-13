#pragma once
#include <memory>
#include <unordered_map>
#include <ranges>
#include <vector>
#include <string>
#include <sstream>
#include <concepts>

// dead simple ecs
namespace dsecs9z {
    /* concepts forward declare */
    template <typename T>
    concept Streamable = requires(std::ostream &os, T value) {
        { os << value } -> std::convertible_to<std::ostream &>;
    };

    /* entity trinity */

    using Entity = uint64_t;
    constexpr Entity NoEntity = 0;

    /* component trinity */

    struct ComponentManagerBase {
        std::string name;

        ComponentManagerBase(std::string_view name)
            : name(name) { }
        virtual ~ComponentManagerBase() = default;

        virtual auto has(Entity e) const -> bool = 0;
        virtual auto str(Entity e) const -> std::string = 0;
        virtual void del(Entity e) = 0;
    };

    template<typename TComp>
    struct ComponentManager : ComponentManagerBase {
        std::unordered_map<Entity, TComp> values; // the actual array

        ComponentManager(std::string_view name)
            : ComponentManagerBase(name), values() { }
        virtual ~ComponentManager() = default;

        void with(Entity e, std::invocable<TComp&> auto chain) {
            if (auto it = values.find(e); it != values.end())
                chain(it->second); // reuse the found iterator/lookup
        }

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
        virtual void del(Entity e) override {
            values.erase(e);
        }
    };

    /* system trinity */

    struct SystemBase {
        std::string name;
        bool enable = true;

        SystemBase(std::string_view name)
            : name(name) { }
        virtual ~SystemBase() = default;

        virtual void update(class World* w) = 0;
    };

    template<std::invocable<class World*> FExec>
    struct SystemAnonymous : SystemBase {
        FExec execution;

        SystemAnonymous(std::string_view name, FExec execution)
            : SystemBase(name), execution(execution) { }
        virtual ~SystemAnonymous() = default;

        virtual void update(class World* w) override { execution(w); } // the actual dispatch
    };

    /* name ergonomics */

    struct Name {
        std::string name;
    };

    auto& operator<<(std::ostream& os, Name n) {
        return os << n.name;
    }

    /* final world type */

    class World {
            Entity _nextEntity = 1;
            std::unordered_map<size_t, std::shared_ptr<ComponentManagerBase>> _components; // the dynamic structure
            std::vector<std::shared_ptr<SystemBase>> _systems; // the dynamic update list
            std::unordered_map<std::string_view, Entity> _entityNames;

        public:
            /* trinity */

            auto newEntity() -> Entity { return _nextEntity++; }

            template<typename TComp>
            auto requireComponent() -> std::shared_ptr<ComponentManager<TComp>> {
                auto key = typeid(TComp).hash_code();
                if (auto it = _components.find(key); it != _components.end())
                    // this static cast is safe because we index by typeid
                    return std::static_pointer_cast<ComponentManager<TComp>>(it->second);
                auto res = std::make_shared<ComponentManager<TComp>>(typeid(TComp).name());
                _components[key] = res;
                return res;
            }

            auto allEntities() { return std::ranges::iota_view(1u, _nextEntity); }

            void update() {
                for (auto sys : _systems | std::views::filter(&SystemBase::enable)) {
                    sys->update(this);
                }
            }
        
            template<std::invocable<World*> FExec>
            auto makeSystem(std::string_view name, FExec exec) -> std::shared_ptr<SystemAnonymous<FExec>> {
                auto res = std::make_shared<SystemAnonymous<FExec>>(name, exec);
                _systems.emplace_back(res);
                return res;
            }

            /* ergonomics I */

            auto findEntity(std::string_view name) -> Entity {
                auto it = _entityNames.find(name);
                return (it != _entityNames.end()) ? it->second : NoEntity;
            }

            auto requireEntity(std::string_view name) -> Entity {
                // early exit if the name already exists
                // we don't attempt to reuse this lookup in this case because the string view in the index must come from the component for a safe lifetime.
                if (auto e = findEntity(name); e != NoEntity)
                    return e;
                
                auto e = newEntity();
                std::string_view name_str = (requireComponent<Name>()->values[e] = { std::string(name) }).name;
                return _entityNames[name_str] = e;
            }

            auto allComponents() { return _components | std::views::values; }

            auto allSystems() { return _systems | std::views::all; }

            auto findSystem(std::string_view name) -> std::shared_ptr<SystemBase> { 
                auto it = std::ranges::find_if(_systems, [&](auto s){ return s->name == name; });
                return (it != _systems.end()) ? *it : nullptr;
            }

            void kill(Entity e) {
                for (auto c : allComponents())
                    if (c->has(e))
                        c->del(e);
            }
    };
}
