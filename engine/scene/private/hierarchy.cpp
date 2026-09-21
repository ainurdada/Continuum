#include <hierarchy.h>

#include <algorithm>
#include <vector>

#include <parent.h>
#include <transform.h>

namespace engine::scene {

std::expected<Mat4f, WorldMatrixError> worldMatrix(const ecs::World& world, ecs::Entity entity) {
    std::vector<engine::ecs::Entity> visitedEntities{};

    const auto* transformStash = world.findStash<scene::Transform>();
    if (!transformStash) {
        return std::unexpected(WorldMatrixError::MissingTransform);
    }
    const auto* parentStash = world.findStash<scene::Parent>();
    if (!parentStash) {
        if (transformStash->has(entity)) {
            return scene::localMatrix(*transformStash->get(entity));
        }
        return std::unexpected(WorldMatrixError::MissingTransform);
    }

    Mat4f result = math::identity();
    engine::ecs::Entity currentEntity = entity;
    bool foundTransform = false;
    while (true) {
        if (!world.hasEntity(currentEntity)) {
            return std::unexpected(WorldMatrixError::MissingEntity);
        }
        if (std::find(visitedEntities.begin(), visitedEntities.end(), currentEntity) != visitedEntities.end()) {
            return std::unexpected(WorldMatrixError::CycleDetected);
        }
        const auto* transform = transformStash->get(currentEntity);
        if (!transform) {
            visitedEntities.push_back(currentEntity);
            auto parent = parentStash->get(currentEntity);
            if (parent) {
                currentEntity = parent->entity;
            } else {
                break;
            }
            continue;
        }
        foundTransform = true;
        result = scene::localMatrix(*transform) * result;
        visitedEntities.push_back(currentEntity);
        auto parent = parentStash->get(currentEntity);
        if (parent) {
            currentEntity = parent->entity;
        } else {
            break;
        }
    }

    if (!foundTransform) {
        return std::unexpected(WorldMatrixError::MissingTransform);
    }

    return result;
}

std::expected<Mat4f, WorldMatrixError> worldToLocal(const ecs::World& world, ecs::Entity entity, const Mat4f& m) {
    if (!world.hasEntity(entity)) {
        return std::unexpected(WorldMatrixError::MissingEntity);
    }
    const auto* transformStash = world.findStash<scene::Transform>();
    if (!transformStash) {
        return m;
    }
    const auto* parentStash = world.findStash<scene::Parent>();
    if (parentStash && parentStash->has(entity)) {
        auto parentWorldMatrix = worldMatrix(world, parentStash->get(entity)->entity);
        if (parentWorldMatrix) {
            return math::inverse(parentWorldMatrix.value()) * m;
        }
        if (parentWorldMatrix.error() == WorldMatrixError::MissingTransform) {
            return m;
        }
        return std::unexpected(parentWorldMatrix.error());
    }
    return m;
}

} // namespace engine::scene
