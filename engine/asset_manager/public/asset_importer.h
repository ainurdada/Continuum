#pragma once

#include <expected>
#include <filesystem>
#include <string>

namespace engine::asset::import {

std::expected<unsigned int, std::string> getMeshCount(std::filesystem::path file);

}