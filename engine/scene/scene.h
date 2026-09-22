#pragma once

#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

#include <ecs/ecs.h>
#include <scene/public/scene_entity_id.h>

namespace engine::ecs {
struct Entity;
} // namespace engine::ecs

namespace engine {

class Scene {
    ecs::World& _world;

    std::unordered_map<ecs::Entity, ecs::Entity, ecs::EntityHash> _childParentMap{};
    std::unordered_map<std::uint64_t, ecs::Entity> _sceneIdEntityMap{};

    ecs::World::OnComponentAddFn _onComponentAdd;
    std::size_t _componentChangedHandle;

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    void componentChanged(ecs::World::ComponentAddInfo info);

  public:
    Scene(ecs::World& world);
    ~Scene();

    void setChildParent(scene::SceneEntityId child, scene::SceneEntityId parent);
    std::vector<ecs::Entity> getChildren(ecs::Entity entity) const;
    std::optional<ecs::Entity> getParent(ecs::Entity entity) const;
};

} // namespace engine
