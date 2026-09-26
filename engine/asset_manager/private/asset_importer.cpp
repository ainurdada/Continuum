#include <asset_importer.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>

namespace engine::asset::import {

std::expected<unsigned int, std::string> getMeshCount(std::filesystem::path file) {
    Assimp::Importer imp{};
    auto scene = imp.ReadFile(file.generic_string(), 0);

    if (!scene) {
        return std::unexpected(imp.GetErrorString());
    }

    return scene->mNumMeshes;
}

} // namespace engine::asset::import
