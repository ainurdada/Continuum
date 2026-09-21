#include <content_browser_panel.h>

#include <algorithm>
#include <vector>

#include <imgui.h>

#include <project.h>

namespace editor {

void ContentBrowserPanel::draw(const Project& project) {
    if (ImGui::Begin(CONTENT_BROWSER_WINDOW_NAME)) {
        std::string browserError{};
        std::vector<std::filesystem::path> directories{};
        std::vector<std::filesystem::path> files{};

        std::error_code iterationError{};
        std::filesystem::directory_iterator it(_currentDirectory, iterationError);
        std::filesystem::directory_iterator end;

        auto relativePath = _currentDirectory.lexically_relative(project.projectRoot);
        std::string directoryLabel;
        if (relativePath.empty() || relativePath == ".") {
            directoryLabel = project.name;
        } else {
            directoryLabel = project.name + "/" + relativePath.generic_string();
        }
        ImGui::TextUnformatted(directoryLabel.c_str());

        ImGui::BeginDisabled(_currentDirectory == project.projectRoot);
        if (ImGui::Button("Up")) {
            std::error_code directoryError;
            auto targetCanonicalPath = std::filesystem::canonical(_currentDirectory.parent_path(), directoryError);
            if (!directoryError) {
                auto relativePath = targetCanonicalPath.lexically_relative(project.projectRoot);
                bool insideProject = !relativePath.empty() && !relativePath.is_absolute();
                for (const auto& part : relativePath) {
                    if (part == "..") {
                        insideProject = false;
                        break;
                    }
                }
                if (insideProject) {
                    _currentDirectory = targetCanonicalPath;
                } else {
                    browserError += "path is not inside project";
                }
            } else {
                browserError += "\n" + directoryError.message();
            }
        }
        ImGui::EndDisabled();

        while (!iterationError && it != end) {
            const std::filesystem::directory_entry& entry = *it;

            std::error_code entryError{};
            if (entry.is_directory(entryError)) {
                directories.push_back(entry);
            } else if (!entryError && entry.is_regular_file(entryError)) {
                files.push_back(entry);
            } else if (entryError) {
                browserError += "\n" + entryError.message();
                break;
            }

            it.increment(iterationError);
        }
        if (iterationError) {
            ImGui::TextUnformatted(iterationError.message().c_str());
        }

        std::sort(directories.begin(), directories.end(), [](const std::filesystem::path& a, const std::filesystem::path& b) { return a.filename().string() < b.filename().string(); });
        std::sort(files.begin(), files.end(), [](const std::filesystem::path& a, const std::filesystem::path& b) { return a.filename().string() < b.filename().string(); });

        for (auto directory : directories) {
            std::string directoryLabel = "[D] " + directory.filename().string();
            if (ImGui::Selectable(directoryLabel.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                    std::error_code directoryError;
                    auto targetCanonicalPath = std::filesystem::canonical(directory, directoryError);
                    if (!directoryError) {
                        auto targetRelativePath = targetCanonicalPath.lexically_relative(project.projectRoot);
                        bool insideProject = !targetRelativePath.empty() && !targetRelativePath.is_absolute();
                        for (const auto& part : targetRelativePath) {
                            if (part == "..") {
                                insideProject = false;
                                break;
                            }
                        }
                        if (insideProject) {
                            _currentDirectory = targetCanonicalPath;
                        } else {
                            browserError += "path is not inside project";
                        }
                    } else {
                        browserError += "\n" + directoryError.message();
                    }
                }
            }
        }
        for (auto file : files) {
            std::string fileLabel = "[F] " + file.filename().string();
            if (ImGui::Selectable(fileLabel.c_str())) {
            }
        }

        if (!browserError.empty()) {
            ImGui::TextWrapped("%s", browserError.c_str());
        }
    }
    ImGui::End();
}

} // namespace editor
