#pragma once

#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <model.h>

std::expected<std::vector<TypeModel>, std::string> analyzeHeaders(const std::vector<std::filesystem::path>& headers, std::filesystem::path reflectionInputPath, std::optional<std::filesystem::path> compilationDatabaseDir, std::vector<std::string>& clangArgs);