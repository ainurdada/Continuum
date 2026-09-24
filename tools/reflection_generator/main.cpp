#include "model.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <analysis/frontend.h>
#include <emission/ecs_emitter.h>
#include <emission/snapshot_emitter.h>
#include <emission/reflection_emitter.h>
#include <file_io.h>

std::vector<std::filesystem::path> readHeadersManifest(const std::filesystem::path& manifest) {
    std::vector<std::filesystem::path> result{};
    auto read = readTextLines(manifest);
    if (!read) {
        throw std::runtime_error(read.error());
    }

    for (auto& line : read.value()) {
        result.push_back(std::filesystem::path{line});
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
    auto read = readTextLines(manifestPath);
    if (!read) {
        throw std::runtime_error(read.error());
    }

    int lineNum = 1;
    for (auto& line : read.value()) {
        if (!isValidModuleName(line)) {
            throw std::runtime_error("Not valid module name: " + manifestPath.generic_string() + ":" + std::to_string(lineNum));
        }
        lineNum++;
    }

    return read.value();
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

            auto writeResult = writeTextFile(argv[3], reflectionBootstrap);
            if (!writeResult) {
                std::cerr << writeResult.error() << "\n";
                return 1;
            }

            writeResult = writeTextFile(argv[4], reflectionEcsBootstrap);
            if (!writeResult) {
                std::cerr << writeResult.error() << "\n";
                return 1;
            }

            if (argc == 6) {
                writeResult = writeTextFile(argv[5], editorbootstrap);
                if (!writeResult) {
                    std::cerr << writeResult.error() << "\n";
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

            auto emitedSource = emitReflectionSource(typeModels, includePaths, moduleName);
            auto emitedEcsSource = emitEcsBindingSource(typeModels, includeEcsPaths, moduleName);
            auto emitedSnapshotSource = emitSnapshotPolicySource(typeModels, includePaths, moduleName);

            auto writeResult = writeTextFile(argv[outputIndex], emitedSource);
            if (!writeResult) {
                std::cerr << writeResult.error() << "\n";
                return 1;
            }

            writeResult = writeTextFile(argv[ecsOutputIndex], emitedEcsSource);
            if (!writeResult) {
                std::cerr << writeResult.error() << "\n";
                return 1;
            }

            writeResult = writeTextFile(snapshotOutputPath, emitedSnapshotSource);
            if (!writeResult) {
                std::cerr << writeResult.error() << "\n";
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