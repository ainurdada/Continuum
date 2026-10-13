#pragma once

#include <expected>
#include <string>

#include <math/public/g_math.h>

namespace engine::render {

struct PerspectiveOptions {
    // vertical field of view in radians
    float fov{};
    float nearPlane{};
    float farPlane{};
};

class View {
  private:
    Mat4f _viewMatrix;
    Mat4f _projectionMatrix;

    View(const Mat4f& viewMatrix, const Mat4f& projectionMatrix);

  public:
    static std::expected<View, std::string> create(Mat4f viewMatrix, PerspectiveOptions perspectiveOptions, unsigned int widthPixels, unsigned int heightPixels);

    Mat4f viewMatrix() const noexcept;
    Mat4f projectionMatrix() const noexcept;
};

} // namespace engine::render