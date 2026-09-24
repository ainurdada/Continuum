#pragma once

#include <string>
#include <filesystem>
#include <expected>

namespace engine::asset {

struct AssetID {
    std::string value;

    bool operator==(const AssetID&) const = default;
};

struct AssetIDHash {
    std::size_t operator()(const AssetID& id) const noexcept;
};

struct AssetDescription {
    int formatVersion;
    AssetID id;
};

std::expected<AssetDescription, std::string> createMetaFile(std::filesystem::path assetFile, const AssetID& assetId);
std::expected<AssetDescription, std::string> readMetaFile(std::filesystem::path metaFile);
std::expected<AssetDescription, std::string> getDescription(std::filesystem::path assetFile);
bool hasMetaFile(std::filesystem::path assetFile);


} // namespace engine::asset