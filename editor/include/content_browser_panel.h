#pragma once

#include <filesystem>
#include <string>

#include <asset_manager/public/asset_regisrty.h>

#define CONTENT_BROWSER_WINDOW_NAME "Content Browser"

namespace editor {

struct Project;

struct ContentBrowserDrawInfo {
    const Project& project;
    const engine::asset::AssetRegistry& assetRegistry;
};

class ContentBrowserPanel {
  private:
    std::filesystem::path _currentDirectory;
    std::optional<engine::asset::AssetID> _selectedFile = std::nullopt;

  public:
    ContentBrowserPanel(std::filesystem::path directory) : _currentDirectory(directory) {}

    /// @brief Draw content of current directory
    /// @param project Project info
    void draw(const ContentBrowserDrawInfo& info);
};

} // namespace editor
