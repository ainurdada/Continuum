#pragma once

#include <filesystem>
#include <string>

#define CONTENT_BROWSER_WINDOW_NAME "Content Browser"

namespace editor {

struct Project;

class ContentBrowserPanel {
  private:
    std::filesystem::path _currentDirectory;

  public:
    ContentBrowserPanel(std::filesystem::path directory) : _currentDirectory(directory) {}

    /// @brief Draw content of current directory
    /// @param project Project info
    void draw(const Project& project);
};

} // namespace editor
