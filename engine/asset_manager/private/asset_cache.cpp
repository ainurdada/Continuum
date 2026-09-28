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

std::expected<void, std::string> writeJsonToCahceFile(std::filesystem::path cachePath, const nlohmann::json& json) {
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

std::expected<nlohmann::json, std::string> getCachedJson(std::filesystem::path cachePath) {
    std::error_code errorCode;
    std::filesystem::path canonicalPath = std::filesystem::canonical(cachePath, errorCode);
    if (errorCode) {
        return std::unexpected("failed to get canonical path");
    }
    errorCode.clear();
    bool isRegularFile = std::filesystem::is_regular_file(canonicalPath, errorCode);
    if (errorCode) {
        return std::unexpected(errorCode.message());
    }
    if (!isRegularFile) {
        return std::unexpected("cache file is not regular file");
    }

    std::ifstream cacheStream{canonicalPath};
    if (!cacheStream.is_open()) {
        return std::unexpected("could not open cache file");
    }

    return nlohmann::json::parse(cacheStream, nullptr, false);
}
} // namespace

bool isCacheUpToDate(std::filesystem::path originFile, std::filesystem::path cacheFile) {
    std::error_code ec;
    auto originFileLastWrite = std::filesystem::last_write_time(originFile, ec);
    if (ec) {
        return false;
    }

    auto cahceFileLastWrite = std::filesystem::last_write_time(cacheFile, ec);
    if (ec) {
        return false;
    }

    if (originFileLastWrite <= cahceFileLastWrite) {
        return true;
    }

    return false;
}

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

    return writeJsonToCahceFile(cachePath, json);
}

std::expected<std::vector<graphics::MeshData>, std::string> readMeshCache(std::filesystem::path cachePath) {
    auto cacheJsonResult = getCachedJson(cachePath);
    if (!cacheJsonResult) {
        return std::unexpected(cacheJsonResult.error());
    }
    auto cacheJson = cacheJsonResult.value();

    if (!validateformatVersion(cacheJson)) {
        return std::unexpected("Not valid formatVersion");
    }

    if (!cacheJson.contains("meshes") || !cacheJson.at("meshes").is_array()) {
        return std::unexpected("Missing meshes");
    }

    std::vector<graphics::MeshData> result;

    for (auto& [key, meshJson] : cacheJson.at("meshes").items()) {
        if (!meshJson.contains("positions") || !meshJson.at("positions").is_array()) {
            return std::unexpected("Missing positions");
        }

        graphics::MeshData mesh{};

        for (auto& posJson : meshJson.at("positions").items()) {
            if (!posJson.value().is_array() || posJson.value().size() != 3) {
                return std::unexpected("Not valid positions");
            }

            if (!posJson.value()[0].is_number_float() || !posJson.value()[1].is_number_float() || !posJson.value()[2].is_number_float()) {
                return std::unexpected("Not valid positions type");
            }

            graphics::PositionVertex pos{};
            pos.x = posJson.value()[0].get<float>();
            pos.y = posJson.value()[1].get<float>();
            pos.z = posJson.value()[2].get<float>();

            mesh.positionVertices.push_back(pos);
        }

        if (!meshJson.contains("indices") || !meshJson.at("indices").is_array()) {
            return std::unexpected("Missing indices");
        }

        for (auto& indexJson : meshJson.at("indices").items()) {
            if (!indexJson.value().is_number_integer()) {
                return std::unexpected("Not valid index");
            }
            mesh.indices.push_back(indexJson.value().get<std::uint32_t>());
        }

        result.push_back(mesh);
    }

    return result;
}

std::expected<void, std::string> writeTextureCache(std::filesystem::path cachePath, const graphics::TextureData& texture) {
    nlohmann::json json = nlohmann::json::object();
    json["formatVersion"] = 1;
    json["width"] = texture.width;
    json["height"] = texture.height;

    nlohmann::json bytesJson = nlohmann::json::array();
    for (auto& b : texture.bytes) {
        bytesJson.push_back(b);
    }

    json["bytes"] = bytesJson;

    return writeJsonToCahceFile(cachePath, json);
}

std::expected<graphics::TextureData, std::string> readTextureCache(std::filesystem::path cachePath) {
    auto cacheJsonResult = getCachedJson(cachePath);
    if (!cacheJsonResult) {
        return std::unexpected(cacheJsonResult.error());
    }
    auto cacheJson = cacheJsonResult.value();

    if (!validateformatVersion(cacheJson)) {
        return std::unexpected("Not valid formatVersion");
    }

    if (!cacheJson.contains("width") || !cacheJson.at("width").is_number_unsigned() || !cacheJson.contains("height") || !cacheJson.at("height").is_number_unsigned() || !cacheJson.contains("bytes") || !cacheJson.at("bytes").is_array()) {
        return std::unexpected("Not valid texture json");
    }

    graphics::TextureData tex{};
    tex.width = cacheJson.at("width").get<std::uint32_t>();
    tex.height = cacheJson.at("height").get<std::uint32_t>();

    if (tex.width <= 0 || tex.height <= 0) {
        return std::unexpected("Not valid texture sizes");
    }

    auto targetBytesSize = (std::size_t)tex.width * tex.height * 4;
    if (targetBytesSize != cacheJson.at("bytes").size()) {
        return std::unexpected("Not correct bytes amount");
    }

    tex.bytes.reserve(targetBytesSize);

    for (auto& [key, byteJson] : cacheJson.at("bytes").items()) {
        if (!byteJson.is_number_unsigned()) {
            return std::unexpected("Not valid texture byte");
        }
        tex.bytes.push_back(byteJson.get<std::uint8_t>());
    }

    return tex;
}

} // namespace engine::asset::cache
