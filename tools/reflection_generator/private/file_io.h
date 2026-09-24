#pragma once

#include <expected>
#include <filesystem>
#include <string>
#include <vector>

std::expected<void, std::string> writeTextFile(const std::filesystem::path& path, const std::string& content);

std::expected<std::vector<std::string>, std::string> readTextLines(const std::filesystem::path& path);
