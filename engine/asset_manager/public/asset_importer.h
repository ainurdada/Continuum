#pragma once

#include <expected>
#include <filesystem>
#include <string>
#include <vector>

#include <render/public/mesh_data.h>

namespace engine::asset::import {

std::expected<std::vector<engine::graphics::MeshData>, std::string> loadMeshes(std::filesystem::path file);

}