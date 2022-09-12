
#include <memory>
#include <string>
#include <unordered_map>
#include <ranges>

// dead simple ecs
namespace dsecs {
    using Entity = uint64_t;
    constexpr Entity NoEntity = 0;

    struct ComponentManagerBase {
        virtual ~ComponentManagerBase() = default;
    };

    template<typename TComp>
    struct ComponentManager : ComponentManagerBase {
        std::unordered_map<Entity, TComp> values;

        virtual ~ComponentManager() = default;

        template<typename FChain>
        void doif(Entity e, FChain chain) {
            auto it = values.find(e);
            if (it != values.end())
                chain(it->second);
        }
    };

    struct SystemManagerBase {
        virtual ~SystemManagerBase() = default;
        virtual void update(class World* w) = 0;
    };

    template<typename FExec>
    struct SystemManager : SystemManagerBase { 
        FExec execution;

        SystemManager(FExec execution) : execution(execution) { }
        virtual ~SystemManager() = default;

        virtual void update(class World* w) { execution(w, this); }
    };

    // structured this way to avoid needing to do static/extern
    class World {
            Entity _nextEntity = 1;
            std::unordered_map<size_t, std::shared_ptr<ComponentManagerBase>> _components;
            std::unordered_map<std::string, std::unique_ptr<SystemManagerBase>> _systems;

        public:
            Entity newEntity() { return _nextEntity++; }

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
            SystemManager<FExec> const* makeSystem(std::string name, FExec exec) {
                auto& res
                    = _systems[name]
                    = std::unique_ptr<SystemManagerBase>(new SystemManager<FExec>(exec));
                return &reinterpret_cast<SystemManager<FExec>&>(*res);
            }

            void update() {
                for (auto& [_, sys] : _systems) {
                    sys->update(this);
                }
            }
    };
}
