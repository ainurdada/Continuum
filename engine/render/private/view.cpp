#include <view.h>

#include <cmath>

namespace engine::render {

View::View(const Mat4f& viewMatrix, const Mat4f& projectionMatrix) : _viewMatrix(viewMatrix), _projectionMatrix(projectionMatrix) {}

std::expected<View, std::string> View::create(Mat4f viewMatrix, PerspectiveOptions perspectiveOptions, unsigned int widthPixels, unsigned int heightPixels) {
    if (!std::isfinite(perspectiveOptions.fov) || perspectiveOptions.fov <= 0 || perspectiveOptions.fov >= math::pi<float>()) {
        return std::unexpected("Failed to create view: not valid vertical fov");
    }

    if (!std::isfinite(perspectiveOptions.nearPlane) || perspectiveOptions.nearPlane <= 0) {
        return std::unexpected("Failed to create view: not valid near plane");
    }

    if (!std::isfinite(perspectiveOptions.farPlane) || perspectiveOptions.farPlane <= perspectiveOptions.nearPlane) {
        return std::unexpected("Failed to create view: not valid far plane");
    }

    if (widthPixels == 0) {
        return std::unexpected("Failed to create view: not valid width");
    }

    if (heightPixels == 0) {
        return std::unexpected("Failed to create view: not valid height");
    }

    Mat4f projectionMatrix = math::perspective(perspectiveOptions.fov, static_cast<float>(widthPixels) / static_cast<float>(heightPixels), perspectiveOptions.nearPlane, perspectiveOptions.farPlane);

    return View(viewMatrix, projectionMatrix);
}

Mat4f View::viewMatrix() const noexcept {
    return _viewMatrix;
}

Mat4f View::projectionMatrix() const noexcept {
    return _projectionMatrix;
}

} // namespace engine::render