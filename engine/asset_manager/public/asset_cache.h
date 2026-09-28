#pragma once

#include <expected>
#include <filesystem>
#include <string>
#include <vector>

#include <render/public/mesh_data.h>
#include <render/public/texture_data.h>

namespace engine::asset::cache {

bool isCacheUpToDate(std::filesystem::path originFile, std::filesystem::path cacheFile);

std::expected<void, std::string> writeMeshCache(std::filesystem::path cachePath, const std::vector<graphics::MeshData>& meshes);
std::expected<std::vector<graphics::MeshData>, std::string> readMeshCache(std::filesystem::path cachePath);

std::expected<void, std::string> writeTextureCache(std::filesystem::path cachePath, const graphics::TextureData& texture);
std::expected<graphics::TextureData, std::string> readTextureCache(std::filesystem::path cachePath);

} // namespace engine::asset::cache
