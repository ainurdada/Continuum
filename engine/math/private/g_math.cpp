#include <g_math.h>

#include <cassert>
#include <limits>

#include <glm/ext.hpp>

namespace math {

Mat4f identity() {
    // clang-format off
    return {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    // clang-format on
}

Mat4f translation(const Vec3f& position) {
    return glm::translate(identity(), position);
}

Mat4f rotation(float angleRadians, const Vec3f& axis) {
    return glm::rotate(identity(), angleRadians, axis);
}

Mat4f scaling(const Vec3f& scale) {
    return glm::scale(identity(), scale);
}

Mat4f perspective(float verticalFovRadians, float aspectRatio, float nearPlane, float farPlane) {
    assert(verticalFovRadians > 0 && verticalFovRadians < pi<float>() && aspectRatio > 0 && nearPlane > 0 && farPlane > nearPlane);
    return glm::perspectiveLH_ZO(verticalFovRadians, aspectRatio, nearPlane, farPlane);
}

Mat4f inverse(const Mat4f& mat) {
    return glm::inverse(mat);
}

Vec3f transformPosition(const Mat4f& m, const Vec3f& v) {
    auto result = m * glm::vec4{v.x, v.y, v.z, 1.0f};
    return Vec3f{result.x, result.y, result.z};
}

Vec3f transformDirection(const Mat4f& m, const Vec3f& v) {
    auto result = m * glm::vec4{v.x, v.y, v.z, 0.0f};
    return Vec3f{result.x, result.y, result.z};
}

Vec3f extractPosition(const Mat4f& matrix) {
    return Vec3f{matrix[3][0], matrix[3][1], matrix[3][2]};
}

Vec3f extractRotation(const Mat4f& matrix) {
    Vec3f scale = extractScale(matrix);
    glm::mat3 rotationMatrix = matrix;
    for (int i = 0; i < 3; i++) {
        rotationMatrix[0][i] /= scale.x;
    }
    for (int i = 0; i < 3; i++) {
        rotationMatrix[1][i] /= scale.y;
    }
    for (int i = 0; i < 3; i++) {
        rotationMatrix[2][i] /= scale.z;
    }
    return glm::eulerAngles(glm::normalize(glm::quat_cast(rotationMatrix)));
}

Vec3f extractScale(const Mat4f& matrix) {
    Vec3f axis0 = Vec3f{matrix[0][0], matrix[0][1], matrix[0][2]};
    Vec3f axis1 = Vec3f{matrix[1][0], matrix[1][1], matrix[1][2]};
    Vec3f axis2 = Vec3f{matrix[2][0], matrix[2][1], matrix[2][2]};
    return Vec3f{length(axis0), length(axis1), length(axis2)};
}

float radians(float degrees) {
    return glm::radians(degrees);
}

Vec3f radians(Vec3f degrees) {
    return glm::radians(degrees);
}

float degrees(float radians) {
    return glm::degrees(radians);
}

Vec3f degrees(Vec3f radians) {
    return glm::degrees(radians);
}

float toFloatSafe(double value) {
    if (value > static_cast<double>(std::numeric_limits<float>::max())) {
        return std::numeric_limits<float>::max();
    }
    if (value < static_cast<double>(std::numeric_limits<float>::lowest())) {
        return std::numeric_limits<float>::lowest();
    }
    return static_cast<float>(value);
}

int toIntSafe(double value) {
    value = glm::round(value);
    return static_cast<int>(value);
}

} // namespace math
