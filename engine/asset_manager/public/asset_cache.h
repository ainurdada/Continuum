#pragma once

#include <expected>
#include <filesystem>
#include <string>
#include <vector>

#include <render/public/mesh_data.h>

namespace engine::asset::cache {

std::expected<void, std::string> writeMeshCache(std::filesystem::path cachePath, const std::vector<graphics::MeshData>& meshes);
std::expected<std::vector<graphics::MeshData>, std::string> readMeshCache(std::filesystem::path cachePath);

} // namespace engine::asset::cache
