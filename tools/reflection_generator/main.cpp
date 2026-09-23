#include "model.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <analysis/frontend.h>
#include <emitter.h>

std::vector<std::filesystem::path> readHeadersManifest(const std::filesystem::path& manifest) {
    std::ifstream file(manifest);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file" + manifest.string());
    }

    std::vector<std::filesystem::path> result{};
    std::string currentPath;

    while (std::getline(file, currentPath)) {
        result.push_back(std::filesystem::path{currentPath});
    }

    if (file.bad() || !file.eof()) {
        throw std::runtime_error("Failed to read file correctly" + manifest.string());
    }

    return result;
}

bool isValidModuleName(const std::string& moduleName) {
    if (moduleName.empty()) {
        return false;
    }

    if (moduleName.contains("__")) {
        return false;
    }

    if (moduleName[0] == '_') {
        return false;
    }

    for (auto c : moduleName) {
        if (c >= 'A' && c <= 'Z') {
            continue;
        }
        if (c >= 'a' && c <= 'z') {
            continue;
        }
        if (c >= '0' && c <= '9') {
            continue;
        }
        if (c == '_') {
            continue;
        }
        return false;
    }

    return true;
}

std::vector<std::string> readModulesManifest(const std::filesystem::path& manifestPath) {
    std::ifstream file(manifestPath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + manifestPath.generic_string());
    }

    std::vector<std::string> result{};
    std::string line;
    int lineNum = 1;
    while (std::getline(file, line)) {
        if (!isValidModuleName(line)) {
            throw std::runtime_error("Not valid module name: " + manifestPath.generic_string() + ":" + std::to_string(lineNum));
        }
        result.push_back(line);
        lineNum++;
    }

    if (file.bad() || !file.eof()) {
        throw std::runtime_error("Failed to read file correctly" + manifestPath.string());
    }

    return result;
}

int main(int argc, char* argv[]) {
    if (argc > 1 && argv[1] == std::string("--bootstrap")) {
        if (argc != 5 && argc != 6) {
            std::cerr << "Usage: ContinuumReflectionGenerator <--bootstrap> <manifest-file> <out-file> <out-ecs-file> [out-editor-file]" << std::endl;
            return 1;
        }

        try {
            auto modules = readModulesManifest(argv[2]);
            auto reflectionBootstrap = emitReflectionBootstrapSource(modules);
            auto reflectionEcsBootstrap = emitEcsBootstrapSource(modules);
            std::string editorbootstrap = "";
            if (argc == 6) {
                editorbootstrap = emitSnapshotBootstrapSource(modules);
            }

            std::ofstream file(argv[3]);
            if (!file.is_open()) {
                std::cerr << "Failed to open file: " << argv[3];
                return 1;
            }
            file << reflectionBootstrap;
            if (file.fail()) {
                std::cerr << "failed to write into file: " << argv[3] << std::endl;
                return 1;
            }
            file.close();
            if (file.fail()) {
                std::cerr << "failed to close file: " << argv[3] << std::endl;
                return 1;
            }

            std::ofstream fileEcs(argv[4]);
            if (!fileEcs.is_open()) {
                std::cerr << "Failed to open file: " << argv[4];
                return 1;
            }
            fileEcs << reflectionEcsBootstrap;
            if (fileEcs.fail()) {
                std::cerr << "failed to write into file: " << argv[4] << std::endl;
                return 1;
            }
            fileEcs.close();
            if (fileEcs.fail()) {
                std::cerr << "failed to close file: " << argv[4] << std::endl;
                return 1;
            }

            if (argc == 6) {
                std::ofstream fileEditor(argv[5]);
                if (!fileEditor.is_open()) {
                    std::cerr << "Failed to open file: " << argv[5];
                    return 1;
                }
                fileEditor << editorbootstrap;
                if (fileEditor.fail()) {
                    std::cerr << "failed to write into file: " << argv[5] << std::endl;
                    return 1;
                }
                fileEditor.close();
                if (fileEditor.fail()) {
                    std::cerr << "failed to close file: " << argv[5] << std::endl;
                    return 1;
                }
            }
            return 0;
        } catch (const std::exception& err) {
            std::cerr << err.what() << std::endl;
            return 1;
        }
    }

    if (argc < 5) {
        std::cerr << "Usage: ContinuumReflectionGenerator [--manifest] <source-file> <out-file> <out-ecs-file> <module-name> [--compdb] [--] [clang-args...]" << std::endl;
        return 1;
    }

    bool manifestMode = false;
    if (argv[1] == std::string("--manifest")) {
        manifestMode = true;
    }

    int inputIndex = manifestMode ? 2 : 1;
    int outputIndex = inputIndex + 1;
    int ecsOutputIndex = inputIndex + 2;
    int moduleIndex = inputIndex + 3;
    int separatorIndex = inputIndex + 4;

    if (argc <= moduleIndex) {
        std::cerr << "Too few arguments" << std::endl;
        return 1;
    }

    std::string moduleName = argv[moduleIndex];
    if (!isValidModuleName(moduleName)) {
        std::cerr << "Not correct module name: " << moduleName << std::endl;
        return 1;
    }

    std::optional<std::filesystem::path> compilationDatabaseDir;
    if (argc > separatorIndex && std::string(argv[separatorIndex]) == "--compdb") {
        if (argc == separatorIndex + 1 || std::string(argv[separatorIndex + 1]) == "" || std::string(argv[separatorIndex + 1]) == "--") {
            std::cerr << "Not valid arguments";
            return 1;
        }
        compilationDatabaseDir = argv[separatorIndex + 1];
        separatorIndex += 2;
    }

    if (argc > separatorIndex && std::string(argv[separatorIndex]) != "--") {
        std::cerr << "No split string --" << std::endl;
        return 1;
    }

    if (compilationDatabaseDir && argc > separatorIndex + 1) {
        std::cerr << "Clang arguments are not supported in --compdb mode" << std::endl;
        return 1;
    }

    std::vector<std::string> clangArgs{
        {"-std=c++23"},
        {"-x"},
        {"c++"},
    };

    for (int i = separatorIndex + 1; i < argc; i++) {
        clangArgs.push_back(argv[i]);
    }

    try {
        std::filesystem::path inputFile = std::filesystem::absolute(argv[inputIndex]);
        std::filesystem::path outputDirectory = std::filesystem::absolute(argv[outputIndex]).parent_path();
        std::filesystem::path outputEcsDirectory = std::filesystem::absolute(argv[ecsOutputIndex]).parent_path();
        std::filesystem::path snapshotOutputPath = outputDirectory / (moduleName + ".editor_reflection.generated.cpp");

        std::vector<std::filesystem::path> headers = manifestMode ? readHeadersManifest(inputFile) : std::vector<std::filesystem::path>{inputFile};

        std::filesystem::path reflectionInputPath = outputDirectory / (moduleName + ".reflection_input.cpp");

        auto analyseResult = analyzeHeaders(headers, reflectionInputPath, compilationDatabaseDir, clangArgs);

        if (analyseResult.has_value()) {
            std::vector<std::string> includePaths{};
            std::vector<std::string> includeEcsPaths{};
            auto typeModels = analyseResult.value();

            for (auto& header : headers) {
                includePaths.push_back(std::filesystem::relative(header, outputDirectory).generic_string());
                includeEcsPaths.push_back(std::filesystem::relative(header, outputEcsDirectory).generic_string());
            }

            auto emitedSource = emitSource(typeModels, includePaths, moduleName);
            auto emitedEcsSource = emitEcsBindingSource(typeModels, includeEcsPaths, moduleName);
            auto emitedSnapshotSource = emitSnapshotPolicySource(typeModels, includePaths, moduleName);

            std::ofstream outFile(argv[outputIndex]);
            if (!outFile.is_open()) {
                std::cerr << "failed to open file: " << argv[outputIndex] << std::endl;
                return 1;
            }
            outFile << emitedSource;
            if (outFile.fail()) {
                std::cerr << "failed to write into file: " << argv[outputIndex] << std::endl;
                return 1;
            }
            outFile.close();
            if (outFile.fail()) {
                std::cerr << "failed to close file: " << argv[outputIndex] << std::endl;
                return 1;
            }

            std::ofstream outEcsFile(argv[ecsOutputIndex]);
            if (!outEcsFile.is_open()) {
                std::cerr << "failed to open file: " << argv[ecsOutputIndex] << std::endl;
                return 1;
            }
            outEcsFile << emitedEcsSource;
            if (outEcsFile.fail()) {
                std::cerr << "failed to write into file: " << argv[ecsOutputIndex] << std::endl;
                return 1;
            }
            outEcsFile.close();
            if (outEcsFile.fail()) {
                std::cerr << "failed to close file: " << argv[ecsOutputIndex] << std::endl;
                return 1;
            }

            std::ofstream outSnapshotPolicyFile(snapshotOutputPath);
            if (!outSnapshotPolicyFile.is_open()) {
                std::cerr << "failed to open file: " << snapshotOutputPath.generic_string() << std::endl;
                return 1;
            }
            outSnapshotPolicyFile << emitedSnapshotSource;
            if (outSnapshotPolicyFile.fail()) {
                std::cerr << "failed to write into file: " << snapshotOutputPath.generic_string() << std::endl;
                return 1;
            }
            outSnapshotPolicyFile.close();
            if (outSnapshotPolicyFile.fail()) {
                std::cerr << "failed to close file: " << snapshotOutputPath.generic_string() << std::endl;
                return 1;
            }
        } else {
            std::cerr << analyseResult.error() << std::endl;
            return 1;
        }
        return 0;
    } catch (const std::exception& err) {
        std::cerr << err.what();
        return 1;
    }
}