#pragma once

#include <reflection/public/markers.h>
#include <render/public/render_frame_data.h>

namespace engine::scene {

OBJECT(Component)
struct MeshRenderer {
    GeometryId geometryId{};

    FIELD(ShowInInspector)
    Vec3f baseColor{1, 1, 1};
};

} // namespace engine::scene
