#pragma once

#include <type_traits>

#include <glm/common.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

using Vec2f = glm::vec2;
using Vec3f = glm::vec3;
using Vec3d = glm::dvec3;
using Mat4f = glm::mat4;

static_assert(sizeof(Mat4f) == 16 * sizeof(float));
static_assert(std::is_trivially_copyable_v<Mat4f>);

namespace math {

using glm::clamp;
using glm::cross;
using glm::distance;
using glm::dot;
using glm::length;
using glm::normalize;
using glm::pi;
using glm::transpose;
using glm::min;
using glm::max;

inline constexpr Vec3f worldRight{1.0, 0.0, 0.0};
inline constexpr Vec3f worldUp{0.0, 1.0, 0.0};
inline constexpr Vec3f worldForward{0.0, 0.0, 1.0};

[[nodiscard]] Mat4f identity();
[[nodiscard]] Mat4f translation(const Vec3f& position);
[[nodiscard]] Mat4f rotation(float angleRadians, const Vec3f& axis);
[[nodiscard]] Mat4f scaling(const Vec3f& scale);
[[nodiscard]] Mat4f perspective(float verticalFovRadians, float aspectRatio, float nearPlane, float farPlane);
[[nodiscard]] Mat4f inverse(const Mat4f& mat);

[[nodiscard]] Vec3f transformPosition(const Mat4f& m, const Vec3f& v);
[[nodiscard]] Vec3f transformDirection(const Mat4f& m, const Vec3f& v);

[[nodiscard]] Vec3f extractPosition(const Mat4f& matrix);
[[nodiscard]] Vec3f extractRotation(const Mat4f& matrix);
[[nodiscard]] Vec3f extractScale(const Mat4f& matrix);

[[nodiscard]] float radians(float degrees);
[[nodiscard]] Vec3f radians(Vec3f degrees);
[[nodiscard]] float degrees(float radians);
[[nodiscard]] Vec3f degrees(Vec3f radians);

[[nodiscard]] float toFloatSafe(double value);
[[nodiscard]] int toIntSafe(double value);

} // namespace math
