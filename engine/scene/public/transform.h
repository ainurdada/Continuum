#pragma once

#include <optional>
#include <math/public/g_math.h>
#include <reflection/public/markers.h>

namespace engine::scene {

OBJECT(Component)
struct Transform {
    // local position of object in meters
    Vec3f position{0, 0, 0};
    // local rotation of object in Euler angles (radians)
    Vec3f rotation{0, 0, 0};
    Vec3f scale{1, 1, 1};
};

[[nodiscard]] Mat4f localMatrix(const Transform& transform);
[[nodiscard]] Transform fromMat4(const Mat4f& matrix);

} // namespace engine::scene
