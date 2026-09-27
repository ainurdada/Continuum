#include <asset_cache.h>

#include <fstream>

#include <nlohmann/json.hpp>

namespace engine::asset::cache {

std::expected<void, std::string> writeMeshCache(std::filesystem::path cachePath, const std::vector<graphics::MeshData>& meshes) {
    nlohmann::json json = nlohmann::json::object();
    json["formatVersion"] = 1;

    nlohmann::json meshesJson = nlohmann::json::array();
    for (auto& mesh : meshes) {
        nlohmann::json meshJson = nlohmann::json::object();

        nlohmann::json verticesJson = nlohmann::json::array();
        for (auto& vertex : mesh.positionVertices) {
            nlohmann::json vertexJson = nlohmann::json::array();
            vertexJson.push_back(vertex.x);
            vertexJson.push_back(vertex.y);
            vertexJson.push_back(vertex.z);

            verticesJson.push_back(vertexJson);
        }
        meshJson["positions"] = verticesJson;

        nlohmann::json indicesJson = nlohmann::json::array();
        for (auto& index : mesh.indices) {
            indicesJson.push_back(index);
        }
        meshJson["indices"] = indicesJson;

        meshesJson.push_back(meshJson);
    }

    json["meshes"] = meshesJson;

    std::error_code ec;
    std::filesystem::create_directories(cachePath.parent_path(), ec);
    if (ec) {
        return std::unexpected(ec.message());
    }

    std::ofstream cache(cachePath);
    if (!cache.is_open()) {
        return std::unexpected("Failed to create file: " + cachePath.generic_string());
    }

    cache << json;
    if (cache.fail()) {
        return std::unexpected("Failed to write to file: " + cachePath.generic_string());
    }

    cache.close();
    if (cache.fail()) {
        return std::unexpected("Failed to close file: " + cachePath.generic_string());
    }

    return {};
}

} // namespace engine::asset::cache
