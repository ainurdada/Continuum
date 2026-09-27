#pragma once

#include <asset_manager/public/asset.h>
#include <reflection/public/markers.h>

namespace engine::scene {

OBJECT(Component)
struct MeshReference {
    FIELD()
    asset::AssetID modelId;
};

} // namespace engine::scene
