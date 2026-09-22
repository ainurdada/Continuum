#pragma once

#include <reflection/public/markers.h>
#include <render/public/render_frame_data.h>

namespace engine::scene {

OBJECT(Component)
struct MeshRenderer {
    GeometryId geometryId{};
};

} // namespace engine::scene
