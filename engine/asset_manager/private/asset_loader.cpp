#include <asset_loader.h>

#include <asset_cache.h>
#include <asset_importer.h>

namespace engine::asset {

std::expected<ModelLoadResult, std::string> loadModel(std::filesystem::path projectRoot, const AssetRegistry& reg, AssetID id, bool forceReimport) {
    auto assetInfo = reg.getAsset(id);
    if (!assetInfo) {
        return std::unexpected("Failed to get asset info");
    }

    ModelLoadResult result{};

    if (!forceReimport && engine::asset::cache::isCacheUpToDate(projectRoot / assetInfo->path, projectRoot / "cache/models" / (assetInfo->desc.id.value + ".json"))) {
        auto meshes = engine::asset::cache::readMeshCache(projectRoot / "cache/models" / (assetInfo->desc.id.value + ".json"));
        if (!meshes) {
            result.cacheWarning = "Failed to read cache file: " + meshes.error();
        } else {
            result.meshes = meshes.value();
            return result;
        }
    }

    auto meshes = engine::asset::import::loadMeshes(projectRoot / assetInfo->path);
    if (!meshes) {
        return std::unexpected("Failed to load meshes: " + meshes.error());
    }

    result.meshes = meshes.value();

    auto cache = engine::asset::cache::writeMeshCache(projectRoot / "cache/models" / (assetInfo->desc.id.value + ".json"), meshes.value());
    if (!cache) {
        std::string warning = "Failed to write asset cache: " + cache.error();
        if (result.cacheWarning) {
            result.cacheWarning.value() += "\n" + warning;
        } else {
            result.cacheWarning = warning;
        }
    }

    return result;
}

std::expected<TextureLoadResult, std::string> loadTexture(std::filesystem::path projectRoot, const AssetRegistry& reg, AssetID id, bool forceReimport) {
    auto assetInfo = reg.getAsset(id);
    if (!assetInfo) {
        return std::unexpected("Failed to get asset info");
    }

    TextureLoadResult result{};

    if (!forceReimport && engine::asset::cache::isCacheUpToDate(projectRoot / assetInfo->path, projectRoot / "cache/textures" / (assetInfo->desc.id.value + ".json"))) {
        auto texture = engine::asset::cache::readTextureCache(projectRoot / "cache/textures" / (assetInfo->desc.id.value + ".json"));
        if (!texture) {
            result.cacheWarning = "Failed to read cache file: " + texture.error();
        } else {
            result.texture = texture.value();
            return result;
        }
    }

    auto texture = engine::asset::import::loadTexture(projectRoot / assetInfo->path);
    if (!texture) {
        return std::unexpected("Failed to load texture: " + texture.error());
    }

    result.texture = texture.value();

    auto cache = engine::asset::cache::writeTextureCache(projectRoot / "cache/textures" / (assetInfo->desc.id.value + ".json"), texture.value());
    if (!cache) {
        std::string warning = "Failed to write asset cache: " + cache.error();
        if (result.cacheWarning) {
            result.cacheWarning.value() += "\n" + warning;
        } else {
            result.cacheWarning = warning;
        }
    }

    return result;
}

} // namespace engine::asset