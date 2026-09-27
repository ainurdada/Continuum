#pragma once

#include <expected>
#include <filesystem>
#include <optional>
#include <vector>

#include <render/public/mesh_data.h>
#include "asset_regisrty.h"

namespace engine::asset {

struct ModelLoadResult {
    std::vector<graphics::MeshData> meshes;
    std::optional<std::string> cacheWarning = std::nullopt;
};

std::expected<ModelLoadResult, std::string> loadModel(std::filesystem::path projectRoot, const AssetRegistry& reg, AssetID id, bool forceReimport = false);

} // namespace engine::asset