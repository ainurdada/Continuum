#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include <asset_manager/public/asset.h>

#include <asset_manager/public/asset_regisrty.h>

#define CONTENT_BROWSER_WINDOW_NAME "Content Browser"

namespace editor {

struct Project;

struct ContentBrowserDrawInfo {
    const Project& project;
    engine::asset::AssetRegistry& assetRegistry;
};

struct ContentBrowserDrawResult {
    std::optional<engine::asset::AssetID> reimportAssetId = std::nullopt;
};

class ContentBrowserPanel {
  private:
    std::filesystem::path _currentDirectory;
    std::optional<engine::asset::AssetID> _selectedFile = std::nullopt;

  public:
    ContentBrowserPanel(std::filesystem::path directory) : _currentDirectory(directory) {}

    /// @brief Draw content of current directory
    /// @param project Project info
    ContentBrowserDrawResult draw(ContentBrowserDrawInfo& info);
};

} // namespace editor
