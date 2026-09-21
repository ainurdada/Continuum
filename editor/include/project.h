#pragma once

#include <expected>
#include <filesystem>
#include <string>

namespace editor {

struct Project {
    std::filesystem::path projectFile{};
    std::filesystem::path projectRoot{};
    int formatVersion{};
    std::string name{};

    static std::expected<Project, std::string> openProject(const std::filesystem::path& path);
};

} // namespace editor
