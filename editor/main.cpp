#include <iostream>

#include <editor_application.h>
#include <project.h>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: ContinuumEditor <project-file>" << std::endl;
        return 1;
    }
    std::filesystem::path path = argv[1];

    auto openProjectresult = editor::Project::openProject(path);
    if (!openProjectresult.has_value()) {
        std::cerr << "error: " << openProjectresult.error() << "\n" << path << std::endl;
        return 1;
    }
    const auto& project = openProjectresult.value();

    std::cout << "project file: " << project.projectFile << std::endl;
    std::cout << "project root: " << project.projectRoot << std::endl;
    std::cout << "opened project: " << project.name << std::endl;

    editor::EditorApplication editor{project};

    // run editor
    return editor.run();
}
