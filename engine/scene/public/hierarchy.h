#pragma once

#include <expected>

#include <math/public/g_math.h>
#include <ecs/ecs.h>

namespace engine::scene {

enum class WorldMatrixError {
    MissingEntity,
    MissingTransform,
    CycleDetected
};

[[nodiscard]] std::expected<Mat4f, WorldMatrixError> worldMatrix(const ecs::World& world, ecs::Entity entity);

/// @brief Convert world matrix to local matrix
/// @param world ECS world
/// @param entity ECS entity
/// @param m World matrix
/// @return Local matrix or error
[[nodiscard]] std::expected<Mat4f, WorldMatrixError> worldToLocal(const ecs::World& world, ecs::Entity entity, const Mat4f& m);

} // namespace engine::scene
