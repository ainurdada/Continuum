#include <scene/scene.h>

#include <scene/public/parent.h>

namespace engine {
void Scene::componentChanged(ecs::World::ComponentAddInfo info) {
    auto stash = _world.findStash(info.componentId);
    if (!stash) {
        return;
    }

    auto type = stash->nativeTypeKey();

    if (type == typeid(scene::SceneEntityId)) {
        if (info.added) {
            auto sceneId = static_cast<scene::SceneEntityId*>(info.componentData);
            _sceneIdEntityMap.emplace(sceneId->value, info.entity);
        } else {
            for (auto [k, v] : _sceneIdEntityMap) {
                if (v == info.entity) {
                    _sceneIdEntityMap.erase(k);
                    break;
                }
            }
        }
    }
    if (type == typeid(scene::Parent)) {
        if (info.added) {
            _childParentMap.insert_or_assign(info.entity, ((scene::Parent*)info.componentData)->entity);
        } else {
            _childParentMap.erase(info.entity);
        }
    }
}

Scene::Scene(ecs::World& world) : _world(world) {
    auto query = _world.query().build();
    auto& sceneIdStash = _world.getStash<scene::SceneEntityId>();
    auto& parentStash = _world.getStash<scene::Parent>();
    for (auto entity : query.view()) {
        if (sceneIdStash.has(entity)) {
            _sceneIdEntityMap.emplace(sceneIdStash.get(entity)->value, entity);
        }
        if (parentStash.has(entity)) {
            _childParentMap.insert_or_assign(entity, parentStash.get(entity)->entity);
        }
    }

    _onComponentAdd = [this](ecs::World::ComponentAddInfo info) { componentChanged(info); };
    _componentChangedHandle = _world.bindOnComponentChanged(_onComponentAdd);
}

Scene::~Scene() {
    _world.unbindOnComponentChanged(_componentChangedHandle);
}

void Scene::setChildParent(scene::SceneEntityId child, scene::SceneEntityId parent) {
    if (!_sceneIdEntityMap.contains(child.value) || !_sceneIdEntityMap.contains(parent.value)) {
        return;
    }
    auto entityChild = _sceneIdEntityMap.at(child.value);
    auto entityParent = _sceneIdEntityMap.at(parent.value);

    auto& parentStash = _world.getStash<scene::Parent>();
    parentStash.remove(entityChild);
    parentStash.add(entityChild, scene::Parent{.entity = entityParent});
}

std::vector<ecs::Entity> Scene::getChildren(ecs::Entity entity) const {
    std::vector<ecs::Entity> result;
    for (auto [k, v] : _childParentMap) {
        if (v == entity) {
            result.push_back(k);
        }
    }
    return result;
}

std::optional<ecs::Entity> Scene::getParent(ecs::Entity entity) const {
    if (!_childParentMap.contains(entity)) {
        return std::nullopt;
    }
    return _childParentMap.at(entity);
}

} // namespace engine