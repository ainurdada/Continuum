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

std::expected<std::filesystem::path, std::string> getAbsolutePath(std::filesystem::path projectPath, std::filesystem::path p) {
    std::filesystem::path assetsRootAbs = std::filesystem::weakly_canonical(projectPath / "assets");
    std::filesystem::path targetAbs = std::filesystem::weakly_canonical(projectPath / p);

    auto [mismatchIt1, mismatchIt2] = std::mismatch(assetsRootAbs.begin(), assetsRootAbs.end(), targetAbs.begin(), targetAbs.end());

    if (mismatchIt1 != assetsRootAbs.end()) {
        return std::unexpected("Path is outside of assets: " + targetAbs.generic_string());
    }

    return targetAbs;
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

std::optional<AssetInfo> AssetRegistry::getAsset(const AssetID& id) const {
    if (!_assets.contains(id)) {
        return std::nullopt;
    }

    return _assets.at(id);
}

std::optional<AssetInfo> AssetRegistry::getAsset(const std::filesystem::path& path) const {
    for (auto& [k, v] : _assets) {
        if (v.path == path) {
            return v;
        }
    }

    return std::nullopt;
}

std::expected<void, std::string> AssetRegistry::registerProjectFiles() {
    std::filesystem::path assetsPath = _projectPath / "assets";
    std::error_code ec;
    if (!std::filesystem::exists(assetsPath, ec)) {
        if (ec) {
            return std::unexpected("Failed to check assets folder: " + ec.message());
        }
        return std::unexpected("Missing assets folder: " + assetsPath.string());
    }

    try {
        auto iter = std::filesystem::recursive_directory_iterator(assetsPath, std::filesystem::directory_options::none, ec);

        if (ec) {
            return std::unexpected("Failed initialize assets folder iterator: " + ec.message());
        }

        auto endIter = std::filesystem::recursive_directory_iterator();

        while (iter != endIter) {
            const auto& entry = *iter;

            try {
                if (entry.is_regular_file() && entry.path().extension() == ".obj") {
                    auto registrationResult = registerAsset(entry.path());
                    if (!registrationResult) {
                        return std::unexpected(registrationResult.error());
                    }
                }
            } catch (const std::filesystem::filesystem_error& e) {
                return std::unexpected("Failed to get access to file: " + entry.path().string() + " (" + e.what() + ")");
            }

            auto p = iter->path();
            iter.increment(ec);
            if (ec) {
                return std::unexpected("Failed to read element: " + p.string() + " (" + ec.message() + ")");
            }
        }
    } catch (const std::exception& e) {
        return std::unexpected(std::string("Unexpected error: ") + e.what());
    }

    return {};
}

std::expected<void, std::string> AssetRegistry::moveAsset(AssetID id, std::filesystem::path newPath) {
    if (!_assets.contains(id)) {
        return std::unexpected("missing asset info");
    }

    auto& assetInfo = _assets.at(id);

    auto currentAssetPath = getAbsolutePath(_projectPath, assetInfo.path);
    if (!currentAssetPath) {
        return std::unexpected(currentAssetPath.error());
    }
    std::filesystem::path currentMetaPath = currentAssetPath->generic_string() + ".meta";

    if (!std::filesystem::exists(currentAssetPath.value()) || !std::filesystem::exists(currentMetaPath)) {
        return std::unexpected("missing asset files");
    }

    auto newPathAbs = getAbsolutePath(_projectPath, newPath);
    if (!newPathAbs) {
        return std::unexpected(newPathAbs.error());
    }

    if (!std::filesystem::is_directory(newPathAbs.value())) {
        return std::unexpected("new path is not directories");
    }

    auto targetAssetPath = newPathAbs.value() / currentAssetPath->filename();
    std::filesystem::path targetMetaPath = targetAssetPath.generic_string() + ".meta";

    if (std::filesystem::exists(targetAssetPath) || std::filesystem::exists(targetMetaPath)) {
        return std::unexpected("File already exists in " + newPathAbs->generic_string());
    }

    std::error_code ec;
    auto newRelativePath = std::filesystem::relative(targetAssetPath, _projectPath, ec);
    if (ec) {
        return std::unexpected(ec.message());
    }

    std::filesystem::rename(currentMetaPath, targetMetaPath, ec);
    if (ec) {
        return std::unexpected("failed to move meta file: " + ec.message());
    }
    std::filesystem::rename(currentAssetPath.value(), targetAssetPath, ec);
    if (ec) {
        std::error_code ec2;
        std::filesystem::rename(targetMetaPath, currentMetaPath, ec2);
        if (ec2) {
            return std::unexpected("failed to move asset file: " + ec.message() + "\nFailed to move meta file back: " + ec2.message());
        }
        return std::unexpected("failed to move asset file: " + ec.message());
    }

    assetInfo.path = newRelativePath;

    return {};
}

} // namespace engine::asset