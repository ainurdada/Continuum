#pragma once

#include <vector>

#include <math/public/g_math.h>

namespace engine {

struct RenderCameraData {
    float verticalFovRadians{};
    float nearPlane{};
    float farPlane{};
    Mat4f viewMatrix = math::identity();
};

enum class GeometryId {
    Cube
};

struct RenderItem {
    GeometryId geometryId{};
    Mat4f modelMatrix{};
};

struct RenderFrameData {
    bool drawGlobalGrid = false;
    RenderCameraData camera{};
    std::vector<RenderItem> items{};
};

} // namespace engine
