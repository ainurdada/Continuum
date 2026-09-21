#include "project.h"
#include <nlohmann/json.hpp>
#include <system_error>
#include <fstream>

namespace editor {

std::expected<Project, std::string> Project::openProject(const std::filesystem::path& path) {
    std::error_code errorCode;
    std::filesystem::path canonicalPath = std::filesystem::canonical(path, errorCode);
    if (errorCode) {
        return std::unexpected<std::string>("failed to get canonical path");
    }

    std::filesystem::path projectFile = canonicalPath;
    std::filesystem::path projectRoot = canonicalPath.parent_path();

    errorCode.clear();
    bool isRegularFile = std::filesystem::is_regular_file(projectFile, errorCode);
    if (errorCode) {
        return std::unexpected<std::string>(errorCode.message());
    }
    if (!isRegularFile) {
        return std::unexpected<std::string>("project file is not regular file");
    }
    if (projectFile.extension() != ".continuum") {
        return std::unexpected<std::string>("missing continuum project extension");
    }

    std::ifstream projectStream{projectFile};
    if (!projectStream.is_open()) {
        return std::unexpected<std::string>("could not open project file");
    }

    auto jsonProject = nlohmann::json::parse(projectStream, nullptr, false);
    if (jsonProject.is_discarded()) {
        return std::unexpected<std::string>("not correct json file");
    }

    if (!jsonProject.is_object()) {
        return std::unexpected<std::string>("not correct json structure");
    }

    if (!jsonProject.contains("formatVersion")) {
        return std::unexpected<std::string>("missing formatVersion");
    }

    const auto& formatVersionJson = jsonProject.at("formatVersion");
    if (!formatVersionJson.is_number_integer()) {
        return std::unexpected<std::string>("formatVersion must be integer");
    }

    int formatVersion = formatVersionJson.get<int>();
    if (formatVersion != 1) {
        return std::unexpected<std::string>("required formatVersion: 1");
    }

    if (!jsonProject.contains("name")) {
        return std::unexpected<std::string>("missing name");
    }

    const auto& nameJson = jsonProject.at("name");
    if (!nameJson.is_string()) {
        return std::unexpected<std::string>("name must be string");
    }

    std::string name = nameJson.get<std::string>();
    if (name == "") {
        return std::unexpected<std::string>("required not empty name");
    }

    return Project{.projectFile = projectFile, .projectRoot = projectRoot, .formatVersion = formatVersion, .name = name};
}

} // namespace editor
