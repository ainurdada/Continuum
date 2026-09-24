#pragma once

#include <filesystem>
#include <unordered_map>
#include <optional>
#include <expected>
#include <string>

#include "asset.h"

namespace engine::asset {

struct AssetInfo {
    AssetDescription desc;
    std::filesystem::path path;
};

class AssetRegistry {
    std::filesystem::path _projectPath;
    std::unordered_map<AssetID, AssetInfo, AssetIDHash> _assets{};

  public:
    AssetRegistry(const std::filesystem::path& projectPath) : _projectPath(projectPath) {}

    std::expected<void, std::string> registerAsset(const std::filesystem::path& file);
    std::optional<AssetInfo> getAsset(const AssetID& id);
};

} // namespace engine::asset
