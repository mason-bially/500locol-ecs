#pragma once
#include <memory>
#include <unordered_map>
#include <ranges>
#include <vector>
#include <string>

// dead simple ecs
namespace dsecs03 {
    /* entity trinity */

    using Entity = uint64_t;
    constexpr Entity NoEntity = 0;

    /* component trinity */

    struct ComponentManagerBase {
        virtual ~ComponentManagerBase() = default;
    };

    template<typename TComp>
    struct ComponentManager : ComponentManagerBase {
        std::unordered_map<Entity, TComp> values; // the actual array

        virtual ~ComponentManager() = default;
    };

    /* system trinity */

    struct SystemBase {
        virtual ~SystemBase() = default;

        virtual void update(class World* w) = 0;
    };

    template<std::invocable<class World*> FExec>
    struct SystemAnonymous : SystemBase {
        FExec execution;

        SystemAnonymous(FExec execution)
            : SystemBase(), execution(execution) { }
        virtual ~SystemAnonymous() = default;

        virtual void update(class World* w) override { execution(w); } // the actual dispatch
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
                if (auto it = _components.find(key); it != _components.end())
                    // this static cast is safe because we index by typeid
                    return std::static_pointer_cast<ComponentManager<TComp>>(it->second);
                auto res = std::make_shared<ComponentManager<TComp>>();
                _components[key] = res;
                return res;
            }

            auto allEntities() { return std::ranges::iota_view(1u, _nextEntity); }
        
            template<std::invocable<World*> FExec>
            auto makeSystem(FExec exec) -> std::shared_ptr<SystemAnonymous<FExec>> {
                auto res = std::make_shared<SystemAnonymous<FExec>>(exec);
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
