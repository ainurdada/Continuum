#include <asset.h>

#include <fstream>

#include <nlohmann/json.hpp>

namespace engine::asset {

std::expected<AssetDescription, std::string> createMetaFile(std::filesystem::path assetFile, const AssetID& assetId) {
    nlohmann::json content = nlohmann::json::object();
    content["formatVersion"] = 1;
    content["id"] = assetId.value;

    std::filesystem::path metaFilePath = assetFile.generic_string() + ".meta";

    std::ofstream metaFile(metaFilePath);
    if (!metaFile.is_open()) {
        return std::unexpected("Failed to open file: " + metaFilePath.generic_string());
    }

    metaFile << content;
    if (metaFile.fail()) {
        return std::unexpected("Failed to write into file: " + metaFilePath.generic_string());
    }

    metaFile.close();
    if (metaFile.fail()) {
        return std::unexpected("Failed to close file: " + metaFilePath.generic_string());
    }

    return readMetaFile(metaFilePath);
}

std::expected<AssetDescription, std::string> readMetaFile(std::filesystem::path metaFile) {
    std::error_code errorCode;
    std::filesystem::path canonicalPath = std::filesystem::canonical(metaFile, errorCode);
    if (errorCode) {
        return std::unexpected<std::string>("failed to get canonical path: " + metaFile.generic_string());
    }
    errorCode.clear();
    bool isRegularFile = std::filesystem::is_regular_file(canonicalPath, errorCode);
    if (errorCode) {
        return std::unexpected<std::string>(errorCode.message());
    }
    if (!isRegularFile) {
        return std::unexpected<std::string>("asset meta file is not regular file: " + metaFile.generic_string());
    }
    if (canonicalPath.extension() != ".meta") {
        return std::unexpected<std::string>("missing meta extension: " + metaFile.generic_string());
    }

    std::ifstream fileStream{canonicalPath};
    if (!fileStream.is_open()) {
        return std::unexpected<std::string>("could not open meta file: " + metaFile.generic_string());
    }

    auto json = nlohmann::json::parse(fileStream, nullptr, false);

    if (!json.is_object()) {
        return std::unexpected("not correct .meta json structure: " + metaFile.generic_string());
    }
    if (!json.contains("formatVersion")) {
        return std::unexpected("missing formatVersion in .meta: " + metaFile.generic_string());
    }
    if (!json.contains("id")) {
        return std::unexpected("missing id in .meta: " + metaFile.generic_string());
    }

    auto formatVersion = json.at("formatVersion");
    if (!formatVersion.is_number_integer()) {
        return std::unexpected("formatVersion is not a number integer: " + metaFile.generic_string());
    }

    auto id = json.at("id");
    if (!id.is_string()) {
        return std::unexpected("id is not a string: " + metaFile.generic_string());
    }

    return AssetDescription{.formatVersion = formatVersion.get<int>(), .id = AssetID{.value = id.get<std::string>()}};
}

std::expected<AssetDescription, std::string> getDescription(std::filesystem::path assetFile) {
    std::error_code errorCode;
    std::filesystem::path canonicalPath = std::filesystem::canonical(assetFile, errorCode);
    if (errorCode) {
        return std::unexpected<std::string>("failed to get canonical path: " + assetFile.generic_string());
    }
    errorCode.clear();
    if (!std::filesystem::exists(canonicalPath)) {
        return std::unexpected<std::string>("missing file: " + assetFile.generic_string());
    }

    std::filesystem::path metaFilePath = canonicalPath.generic_string() + ".meta";
    if (!std::filesystem::exists(metaFilePath)) {
        return std::unexpected<std::string>("missing meta file: " + metaFilePath.generic_string());
    }

    return readMetaFile(metaFilePath);
}

bool hasMetaFile(std::filesystem::path assetFile) {
    std::error_code errorCode;
    std::filesystem::path canonicalPath = std::filesystem::canonical(assetFile, errorCode);
    if (errorCode) {
        return false;
    }
    errorCode.clear();
    if (!std::filesystem::exists(canonicalPath)) {
        return false;
    }
    std::filesystem::path metaFilePath = canonicalPath.generic_string() + ".meta";
    return std::filesystem::exists(metaFilePath);
}

std::size_t AssetIDHash::operator()(const AssetID& id) const noexcept {
    std::size_t seed = 0x9e3779b9U;

    for (const auto& c : id.value) {
        seed ^= c + std::size_t{0x9e3779b9U} + (seed << 6U) + (seed >> 2U);
    }
    return seed;
}

} // namespace engine::asset