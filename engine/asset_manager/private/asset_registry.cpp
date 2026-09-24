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

} // namespace engine::asset