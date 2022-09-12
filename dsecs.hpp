
#include <memory>
#include <string>
#include <unordered_map>
#include <ranges>

// dead simple ecs
namespace dsecs {
    typedef size_t Entity;

    // structured this way to avoid needing to do static/extern
    struct World {
        // entities
        public:
            Entity _nextEntity = 1;

            inline Entity newEntity() { return _nextEntity++; }

            inline auto allEntities() { return std::views::iota((Entity)1, _nextEntity); }

        // components
        public:
            // this is technically an implementation detail
            struct ComponentManagerBase {
                virtual ~ComponentManagerBase() = default;
            };

            template<typename TComp>
            struct ComponentManager
                : ComponentManagerBase {

                std::unordered_map<Entity, TComp> components;

                virtual ~ComponentManager() = default;

                template<typename FChain>
                inline void doif(Entity e, FChain chain) {
                    auto it = components.find(e);
                    if (it != components.end())
                        chain(it->second);
                }
            };

            std::unordered_map<size_t, std::unique_ptr<ComponentManagerBase>> _components;

            template<typename TComp>
            inline ComponentManager<TComp>* requireComponent() {
                auto it = _components.find(typeid(TComp).hash_code());
                if (it != _components.end())
                    return &reinterpret_cast<ComponentManager<TComp>&>(*it->second);
                auto& res
                    = _components[typeid(TComp).hash_code()]
                    = std::unique_ptr<ComponentManagerBase>(new ComponentManager<TComp>());
                return &reinterpret_cast<ComponentManager<TComp>&>(*res);
            }
        
        // systems
        public:
            // this is technically an implementation detail
            struct SystemManagerBase {
                virtual ~SystemManagerBase() = default;
                virtual void update(World* w) = 0;
            };

            template<typename FExec>
            struct SystemManager
                : SystemManagerBase
            { 
                FExec execution;

                SystemManager(FExec execution) : execution(execution) { }
                virtual ~SystemManager() = default;

                inline virtual void update(World* w) { execution(w, this); }
            };

            std::unordered_map<std::string, std::unique_ptr<SystemManagerBase>> _systems;

            template<typename FExec>
            inline SystemManager<FExec> const* makeSystem(std::string name, FExec exec) {
                auto& res
                    = _systems[name]
                    = std::unique_ptr<SystemManagerBase>(new SystemManager<FExec>(exec));
                return &reinterpret_cast<SystemManager<FExec>&>(*res);
            }

            inline void update() {
                for (auto& [_, sys] : _systems) {
                    sys->update(this);
                }
            }
    };
}
