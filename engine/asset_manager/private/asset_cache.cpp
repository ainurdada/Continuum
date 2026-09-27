#include <asset_cache.h>

#include <fstream>

#include <nlohmann/json.hpp>

namespace engine::asset::cache {

namespace {
bool validateformatVersion(const nlohmann::json& json) {
    if (!json.contains("formatVersion") || !json.at("formatVersion").is_number_integer() || json.at("formatVersion").get<int>() != 1) {
        return false;
    }
    return true;
}
} // namespace

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

std::expected<std::vector<graphics::MeshData>, std::string> readMeshCache(std::filesystem::path cachePath) {
    std::error_code errorCode;
    std::filesystem::path canonicalPath = std::filesystem::canonical(cachePath, errorCode);
    if (errorCode) {
        return std::unexpected<std::string>("failed to get canonical path");
    }
    errorCode.clear();
    bool isRegularFile = std::filesystem::is_regular_file(canonicalPath, errorCode);
    if (errorCode) {
        return std::unexpected<std::string>(errorCode.message());
    }
    if (!isRegularFile) {
        return std::unexpected<std::string>("cache file is not regular file");
    }

    std::ifstream cacheStream{canonicalPath};
    if (!cacheStream.is_open()) {
        return std::unexpected<std::string>("could not open cache file");
    }

    auto cacheJson = nlohmann::json::parse(cacheStream, nullptr, false);

    if (!validateformatVersion(cacheJson)) {
        return std::unexpected<std::string>("Not valid formatVersion");
    }

    if (!cacheJson.contains("meshes") || !cacheJson.at("meshes").is_array()) {
        return std::unexpected<std::string>("Missing meshes");
    }

    std::vector<graphics::MeshData> result;

    for (auto& [key, meshJson] : cacheJson.at("meshes").items()) {
        if (!meshJson.contains("positions") || !meshJson.at("positions").is_array()) {
            return std::unexpected<std::string>("Missing positions");
        }

        graphics::MeshData mesh{};

        for (auto& posJson : meshJson.at("positions").items()) {
            if (!posJson.value().is_array() || posJson.value().size() != 3) {
                return std::unexpected<std::string>("Not valid positions");
            }

            if (!posJson.value()[0].is_number_float() || !posJson.value()[1].is_number_float() || !posJson.value()[2].is_number_float()) {
                return std::unexpected<std::string>("Not valid positions type");
            }

            graphics::PositionVertex pos{};
            pos.x = posJson.value()[0].get<float>();
            pos.y = posJson.value()[1].get<float>();
            pos.z = posJson.value()[2].get<float>();

            mesh.positionVertices.push_back(pos);
        }

        if (!meshJson.contains("indices") || !meshJson.at("indices").is_array()) {
            return std::unexpected<std::string>("Missing indices");
        }

        for (auto& indexJson : meshJson.at("indices").items()) {
            if (!indexJson.value().is_number_integer()) {
                return std::unexpected<std::string>("Not valid index");
            }
            mesh.indices.push_back(indexJson.value().get<std::uint32_t>());
        }

        result.push_back(mesh);
    }

    return result;
}

} // namespace engine::asset::cache
