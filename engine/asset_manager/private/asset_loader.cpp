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

} // namespace engine::asset