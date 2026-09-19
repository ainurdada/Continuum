#pragma once

#include <stdexcept>
#include <vector>

#include <ecs/ecs.h>
#include <component_binding_registry.h>

namespace engine::ecs_reflection {

class WorldReflectionContext {
    ecs::World& _world;
    const ComponentBindingRegistry& _reg;
    std::vector<const ComponentBinding*> _bindings;

    void synchronyze() {
        auto stashCount = _world.stashCount();
        while (_bindings.size() < stashCount) {
            auto newId = static_cast<ecs::ComponentID>(_bindings.size());
            auto newStash = _world.findStash(newId);
            if (!newStash) {
                throw std::logic_error("stash not found");
            }
            _bindings.push_back(_reg.findBinding(newStash->nativeTypeKey()));
        }
    }

  public:
    WorldReflectionContext(ecs::World& world, const ComponentBindingRegistry& reg) : _world(world), _reg(reg) {
        if (!_reg.isFrozen()) {
            throw std::logic_error("Can't create world reflection context: registry is not frozen");
        }
    }

    /// @brief Makes visitor to visit all entity component stashes
    /// @param visitor example: void visitor(const IStash&, std::optional<engine::reflection::ObjectView>)
    /// @return Was visiting successful
    template <typename Visitor> bool visitComponents(ecs::Entity entity, Visitor&& visitor) {
        synchronyze();
        return _world.visitComponents(entity, [this, entity, &visitor](const ecs::IStash& stash) {
            auto bind = _bindings[stash.componentId()];
            std::optional<reflection::ObjectView> view;
            if (bind) {
                view = bind->read(stash, entity, *bind->type);
                if (!view) {
                    throw std::logic_error("registered component does not have reading object view");
                }
            }
            visitor(stash, view);
        });
    }

    /// @brief Makes visitor to visit all entity component stashes
    /// @param visitor example: void visitor(IStash&, std::optional<engine::reflection::ObjectView>)
    /// @return Was visiting successful
    template <typename Visitor> bool visitComponentsMut(ecs::Entity entity, Visitor&& visitor) {
        synchronyze();
        return _world.visitComponentsMut(entity, [this, entity, &visitor](ecs::IStash& stash) {
            auto bind = _bindings[stash.componentId()];
            std::optional<reflection::ObjectView> view;
            if (bind) {
                view = bind->write(stash, entity, *bind->type);
                if (!view) {
                    throw std::logic_error("registered component does not have writing object view");
                }
            }
            visitor(stash, view);
        });
    }
};

} // namespace engine::ecs_reflection
