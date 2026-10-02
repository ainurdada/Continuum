#pragma once

#include <vector>
#include <optional>

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
    Vec3f baseColor{
        1,
        1,
        1,
    };
};

struct RenderFrameData {
    std::optional<RenderCameraData> camera = std::nullopt;
    std::vector<RenderItem> items{};
};

} // namespace engine
