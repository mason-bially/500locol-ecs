
#include <memory>
#include <unordered_map>
#include <ranges>
#include <vector>
#include <string>

// dead simple ecs
namespace dsecs {
    /* entity trinity */

    using Entity = uint64_t;
    constexpr Entity NoEntity = 0;

    /* component trinity */

    struct ComponentManagerBase {
        std::string name;

        ComponentManagerBase(std::string_view name)
            : name(name) { }
        virtual ~ComponentManagerBase() = default;
    };

    template<typename TComp>
    struct ComponentManager : ComponentManagerBase {
        std::unordered_map<Entity, TComp> values; // the actual array

        ComponentManager(std::string_view name)
            : ComponentManagerBase(name), values() { }
        virtual ~ComponentManager() = default;

        void with(Entity e, std::invocable<TComp&> auto chain) {
            auto it = values.find(e);
            if (it != values.end())
                chain(it->second); // reuse the found value
        }
    };

    /* system trinity */

    struct SystemBase {
        std::string name;

        SystemBase(std::string_view name)
            : name(name) { }
        virtual ~SystemBase() = default;

        virtual void update(class World* w) = 0;
    };

    template<std::invocable<class World*> FExec>
    struct SystemAnonymous : SystemBase {
        FExec execution;

        SystemAnonymous(std::string_view name, FExec execution)
            : SystemBase { name }, execution(execution) { }
        virtual ~SystemAnonymous() = default;

        virtual void update(class World* w) override { execution(w); } // the actual dispatch
    };

    /* name ergonomics */

    struct Name {
        std::string name;
    };

    /* final world type */

    class World {
            Entity _nextEntity = 1;
            std::unordered_map<size_t, std::shared_ptr<ComponentManagerBase>> _components; // the dynamic structure
            std::vector<std::shared_ptr<SystemBase>> _systems; // the dynamic update list
            std::unordered_map<std::string_view, Entity> _entityNames;

        public:
            auto newEntity() -> Entity { return _nextEntity++; }

            template<typename TComp>
            auto requireComponent() -> std::shared_ptr<ComponentManager<TComp>> {
                auto key = typeid(TComp).hash_code();
                auto it = _components.find(key);
                if (it != _components.end())
                    // this static cast is safe because we index by typeid
                    return std::static_pointer_cast<ComponentManager<TComp>>(it->second);
                auto res = std::make_shared<ComponentManager<TComp>>(typeid(TComp).name());
                _components[key] = res;
                return res;
            }

            auto allEntities() { return std::ranges::iota_view(1u, _nextEntity); }

            void update() {
                for (auto sys : _systems) {
                    sys->update(this);
                }
            }
        
            template<std::invocable<World*> FExec>
            auto makeSystem(std::string_view name, FExec exec) -> std::shared_ptr<SystemAnonymous<FExec>> {
                auto res = std::make_shared<SystemAnonymous<FExec>>(name, exec);
                _systems.emplace_back(res);
                return res;
            }

            auto newEntity(std::string_view name) -> Entity {
                auto e = newEntity();
                return _entityNames[(requireComponent<Name>()->values[e] = { std::string(name) }).name] = e;
            }
    };
}
