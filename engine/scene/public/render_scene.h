#pragma once

#include <unordered_map>

#include <asset_manager/public/asset.h>
#include <ecs/ecs.h>
#include <render/public/mesh_data.h>
#include <render/public/render_frame_data.h>

namespace engine {

struct RenderFrameInput {
    const engine::ecs::World& world;
    const engine::ecs::Query renderEntities;
    const engine::ecs::Query cameraEntities;
    const std::unordered_map<asset::AssetID, std::vector<graphics::MeshHandle>, asset::AssetIDHash>& meshHandles;
};

void collectRenderFrameData(RenderFrameInput& in, RenderFrameData& out);

} // namespace engine