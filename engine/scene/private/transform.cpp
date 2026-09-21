#include <transform.h>

namespace engine::scene {

Mat4f localMatrix(const Transform& transform) {
    return math::translation(transform.position) * math::rotation(transform.rotation.z, math::worldForward) * math::rotation(transform.rotation.y, math::worldUp) * math::rotation(transform.rotation.x, math::worldRight) * math::scaling(transform.scale);
}

Transform fromMat4(const Mat4f& matrix) {
    Transform result{};
    result.position = math::extractPosition(matrix);
    result.rotation = math::extractRotation(matrix);
    result.scale = math::extractScale(matrix);
    return result;
}

} // namespace engine::scene
