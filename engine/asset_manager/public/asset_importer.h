#pragma once

#include <expected>
#include <filesystem>
#include <string>
#include <vector>

#include <render/public/mesh_data.h>
#include <render/public/texture_data.h>

namespace engine::asset::import {

std::expected<std::vector<engine::graphics::MeshData>, std::string> loadMeshes(std::filesystem::path file);
std::expected<engine::graphics::TextureData, std::string> loadTexture(std::filesystem::path file);

} // namespace engine::asset::import