#include <asset_regisrty.h>

#include <random>

namespace engine::asset {

namespace {

std::string generateAssetId() {
    const char* alphabet = "0123456789abcdef";
    std::random_device rd;
    std::uniform_int_distribution<unsigned int> dist(0, 255);

    std::string result;
    for (int i = 0; i < 16; i++) {
        unsigned int randVal = dist(rd);
        char a = alphabet[randVal / 16];
        char b = alphabet[randVal % 16];
        result.push_back(a);
        result.push_back(b);
    }

    return result;
}

} // namespace

std::expected<void, std::string> AssetRegistry::registerAsset(const std::filesystem::path& file) {
    if (!hasMetaFile(file)) {
        auto createResult = createMetaFile(file, AssetID{.value = generateAssetId()});
        if (!createResult) {
            return std::unexpected(createResult.error());
        }
    }

    auto desc = getDescription(file);
    if (!desc) {
        return std::unexpected(desc.error());
    }

    std::error_code errorCode;
    std::filesystem::path canonicalPath = std::filesystem::canonical(file, errorCode);
    if (errorCode) {
        return std::unexpected<std::string>("failed to get canonical path: " + file.generic_string());
    }

    errorCode.clear();
    std::filesystem::path relativePath = std::filesystem::relative(canonicalPath, _projectPath, errorCode);
    if (errorCode) {
        return std::unexpected<std::string>("failed to get relative path: " + canonicalPath.generic_string());
    }

    if (_assets.contains(desc->id)) {
        if (_assets.at(desc->id).path != relativePath) {
            return std::unexpected<std::string>("failed to register not unique asset id: " + canonicalPath.generic_string());
        }
    } else {
        if (!_assets.emplace(desc->id, AssetInfo{.desc = desc.value(), .path = relativePath}).second) {
            return std::unexpected("Failed to register asset: " + file.generic_string());
        }
    }

    return {};
}

std::optional<AssetInfo> AssetRegistry::getAsset(const AssetID& id) {
    if (!_assets.contains(id)) {
        return std::nullopt;
    }

    return _assets.at(id);
}

} // namespace engine::asset