
#include <memory>
#include <unordered_map>
#include <ranges>
#include <vector>
#include <string>

// dead simple ecs
namespace dsecs {
    /* entity helpers */

    using Entity = uint64_t;
    constexpr Entity NoEntity = 0;

    /* component helpers */

    struct ComponentManagerBase {
        virtual ~ComponentManagerBase() = default;
    };

    template<typename TComp>
    struct ComponentManager : ComponentManagerBase {
        std::unordered_map<Entity, TComp> values; // the actual array

        virtual ~ComponentManager() = default;

        template<typename FChain>
        void doif(Entity e, FChain chain) {
            auto it = values.find(e);
            if (it != values.end())
                chain(it->second);
        }
    };

    /* system helpers */

    struct SystemBase {
        std::string name;

        SystemManagerBase(std::string_view name) : name(name) { }
        virtual ~SystemManagerBase() = default;

        virtual void update(class World* w) = 0;
    };

    template<typename FExec>
    struct SystemManager : SystemBase {
        FExec execution;

        SystemManager(std::string_view name, FExec execution) : SystemManagerBase(name), execution(execution) { }
        virtual ~SystemManager() = default;

        virtual void update(class World* w) { execution(w, this); } // the actual dispatch
    };

    /* final world type */

    class World {
            Entity _nextEntity = 1;
            std::unordered_map<size_t, std::shared_ptr<ComponentManagerBase>> _components; // the dynamic structure
            std::vector<std::shared_ptr<SystemBase>> _systems; // the dynamic update list

        public:
            auto newEntity() -> Entity { return _nextEntity++; }

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

            auto allEntities() { return std::ranges::iota_view(1u, _nextEntity); }
        
            template<typename FExec>
            auto makeSystem(std::string_view name, FExec exec) -> std::shared_ptr<SystemManager<FExec>> {
                auto res = std::make_shared<SystemManager<FExec>>(name, exec);
                _systems.emplace_back(res);
                return res;
            }

            void update() {
                for (auto sys : _systems) {
                    sys->update(this);
                }
            }
    };
}
