#include <scene_document.h>

#include <ecs_reflection/public/world_reflection_context.h>
#include <reflection_json/public/json_serialization_registry.h>
#include <reflection/public/type_registry.h>
#include <scene/public/hierarchy.h>
#include <scene/public/name.h>
#include <scene/public/parent.h>
#include <scene/public/scene_entity_id.h>
#include <scene/public/scene_serializer.h>
#include <scene/public/transform.h>

#include <unordered_map>
#include <vector>

namespace editor {

std::expected<std::unique_ptr<SceneDocument>, std::string> SceneDocument::open(const std::filesystem::path& scenePath, engine::scene::serialization::ISceneSerializer& serializer) {
    std::unique_ptr<SceneDocument> sceneDocumentPtr{new SceneDocument()};
    auto deserializationResult = serializer.deserialize(sceneDocumentPtr->_world, scenePath);
    if (!deserializationResult) {
        return std::unexpected(deserializationResult.error());
    }
    sceneDocumentPtr->_scenePath = scenePath;
    return std::move(sceneDocumentPtr);
}

const engine::ecs::World& SceneDocument::world() const noexcept {
    return _world;
}

engine::ecs::World& SceneDocument::worldMut() {
    return _world;
}

const std::filesystem::path& SceneDocument::path() const noexcept {
    return _scenePath;
}

bool SceneDocument::isDirty() const noexcept {
    return _isDirty;
}

void SceneDocument::markDirty() {
    _isDirty = true;
}

std::expected<void, std::string> SceneDocument::save(engine::scene::serialization::ISceneSerializer& serializer) {
    auto saveResult = serializer.serialize(_world, _scenePath);
    if (!saveResult.has_value()) {
        return std::unexpected(saveResult.error());
    }
    _isDirty = false;
    return {};
}

std::expected<engine::ecs::Entity, std::string> SceneDocument::createEmptyEntity() {
    auto& sceneEntityIdStash = _world.getStash<engine::scene::SceneEntityId>();
    auto& nameStash = _world.getStash<engine::scene::Name>();
    auto& transformStash = _world.getStash<engine::scene::Transform>();
    auto sceneEntityIdQuery = _world.query().with<engine::scene::SceneEntityId>().build();
    std::uint64_t maxId = 0;
    for (auto entity : sceneEntityIdQuery.view()) {
        auto entityId = sceneEntityIdStash.get(entity);
        if (maxId < entityId->value) {
            maxId = entityId->value;
        }
    }
    if (maxId == std::numeric_limits<std::uint64_t>::max()) {
        return std::unexpected("max scene id is reached");
    }

    maxId++;
    auto newEntity = _world.createEntity();
    sceneEntityIdStash.add(newEntity, engine::scene::SceneEntityId{.value = maxId});
    if (!sceneEntityIdStash.has(newEntity)) {
        _world.destroyEntity(newEntity);
        return std::unexpected("fail to add scene entity id");
    }
    nameStash.add(newEntity, engine::scene::Name{.value = "Entity " + std::to_string(maxId)});
    if (!nameStash.has(newEntity)) {
        _world.destroyEntity(newEntity);
        return std::unexpected("fail to add name");
    }
    transformStash.add(newEntity, {});
    if (!transformStash.has(newEntity)) {
        _world.destroyEntity(newEntity);
        return std::unexpected("fail to add transform");
    }
    markDirty();
    return newEntity;
}

std::expected<void, std::string> SceneDocument::deleteEntity(engine::ecs::Entity entity, bool deleteChildren) {
    if (!_world.hasEntity(entity)) {
        return std::unexpected("world does not have this entity");
    }
    auto parentQuery = _world.query().with<engine::scene::Parent>().build();
    auto& parentStash = _world.getStash<engine::scene::Parent>();
    auto& transformStash = _world.getStash<engine::scene::Transform>();
    std::unordered_map<engine::ecs::Entity, std::vector<engine::ecs::Entity>, engine::ecs::EntityHash> parentChildrenMap{};

    for (auto childrenEntity : parentQuery.view()) {
        auto parent = parentStash.get(childrenEntity);
        if (!parentChildrenMap.contains(parent->entity)) {
            parentChildrenMap.emplace(parent->entity, std::vector<engine::ecs::Entity>{});
        }
        parentChildrenMap.at(parent->entity).push_back(childrenEntity);
    }

    if (deleteChildren) {
        if (parentChildrenMap.contains(entity)) {
            std::vector<engine::ecs::Entity> children = parentChildrenMap.at(entity);
            while (!children.empty()) {
                auto child = children.back();
                children.pop_back();
                if (parentChildrenMap.contains(child)) {
                    children.append_range(parentChildrenMap.at(child));
                }
                _world.destroyEntity(child);
            }
        }
    } else if (parentChildrenMap.contains(entity)) {
        std::unordered_map<engine::ecs::Entity, Mat4f, engine::ecs::EntityHash> entityMatrixMap{};
        std::vector<std::size_t> currentCheckIndex{};
        std::vector<std::size_t> currentCheckIndexSize{};
        std::vector<engine::ecs::Entity> children = parentChildrenMap.at(entity);
        currentCheckIndex.push_back(-1);
        currentCheckIndexSize.push_back(children.size());

        while (!currentCheckIndex.empty()) {
            if (++currentCheckIndex.back() == currentCheckIndexSize.back()) {
                currentCheckIndex.pop_back();
                currentCheckIndexSize.pop_back();
                continue;
            }
            engine::ecs::Entity child = children[currentCheckIndex.back()];
            if (transformStash.has(child)) {
                auto worldMatrix = engine::scene::worldMatrix(_world, child);
                if (!worldMatrix.has_value()) {
                    switch (worldMatrix.error()) {
                    case engine::scene::WorldMatrixError::CycleDetected:
                        return std::unexpected("failed to get world matrix: cycle detected");
                    case engine::scene::WorldMatrixError::MissingEntity:
                        return std::unexpected("failed to get world matrix: missing entity");
                    case engine::scene::WorldMatrixError::MissingTransform:
                        return std::unexpected("failed to get world matrix: missing transform");
                        break;
                    default:
                        break;
                    }
                } else {
                    entityMatrixMap.emplace(child, worldMatrix.value());
                }
            } else if (parentChildrenMap.contains(child)) {
                currentCheckIndex.push_back(children.size() - 1);
                currentCheckIndexSize.push_back(children.size() + parentChildrenMap.at(child).size());
                children.append_range(parentChildrenMap.at(child));
            }
        }
        for (auto [k, v] : entityMatrixMap) {
            *transformStash.getMut(k) = engine::scene::fromMat4(v);
        }
        for (auto child : parentChildrenMap.at(entity)) {
            parentStash.remove(child);
        }
    }

    _isDirty = true;
    _world.destroyEntity(entity);
    return {};
}

} // namespace editor
