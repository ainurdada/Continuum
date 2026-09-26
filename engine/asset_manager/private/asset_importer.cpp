#include <asset_importer.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

namespace engine::asset::import {

std::expected<std::vector<engine::graphics::MeshData>, std::string> loadMeshes(std::filesystem::path file) {
    Assimp::Importer imp{};
    auto scene = imp.ReadFile(file.generic_string(), aiProcess_Triangulate | aiProcess_PreTransformVertices);

    if (!scene) {
        return std::unexpected(imp.GetErrorString());
    }

    std::vector<engine::graphics::MeshData> result;
    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        auto mesh = *(scene->mMeshes + i);

        engine::graphics::MeshData data;

        for (unsigned int j = 0; j < mesh->mNumVertices; j++) {
            auto vertex = mesh->mVertices + j;
            engine::graphics::PositionVertex pos{.x = vertex->x, .y = vertex->y, .z = vertex->z};
            data.positionVertices.push_back(pos);
        }

        for (unsigned int j = 0; j < mesh->mNumFaces; j++) {
            auto face = mesh->mFaces + j;
            if (face->mNumIndices != 3) {
                return std::unexpected("Mesh has non-triangle faces");
            }
            for (unsigned int k = 0; k < 3; k++) {
                data.indices.push_back(*(face->mIndices + k));
            }
        }

        result.push_back(data);
    }

    return result;
}

} // namespace engine::asset::import
