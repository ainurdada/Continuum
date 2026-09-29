#pragma once

#include <vector>

#include "mesh_data.h"
#include <math/public/g_math.h>

namespace engine {

struct RenderCameraData {
    float verticalFovRadians{};
    float nearPlane{};
    float farPlane{};
    Mat4f viewMatrix = math::identity();
};

enum class GeometryId {
    Cube,
    UploadedMesh,
};

struct RenderItem {
    GeometryId geometryId{};
    graphics::MeshHandle meshHandle = 0;
    Mat4f modelMatrix{};
};

struct RenderFrameData {
    bool drawGlobalGrid = false;
    RenderCameraData camera{};
    std::vector<RenderItem> items{};
};

} // namespace engine
