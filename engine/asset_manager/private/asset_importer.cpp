#include <asset_importer.h>

#include <memory>

#include <SDL3/SDL.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <math/public/g_math.h>

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

        std::vector<Vec3f> positions{};
        std::vector<engine::graphics::VertexAttributeDescription> descs = {
            engine::graphics::VertexAttributeDescription{.location = 0, .format = engine::graphics::VertexAttributeFormat::Float3, .offset = 0},
        };
        for (unsigned int j = 0; j < mesh->mNumVertices; j++) {
            auto vertex = mesh->mVertices + j;
            positions.push_back({vertex->x, vertex->y, vertex->z});
        }

        data.vertices = engine::graphics::makeVerticesData<Vec3f>(positions, descs);

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

std::expected<engine::graphics::TextureData, std::string> loadTexture(std::filesystem::path file) {
    std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> surface(SDL_LoadPNG(file.generic_string().c_str()), &SDL_DestroySurface);
    if (!surface) {
        return std::unexpected(SDL_GetError());
    }

    std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> converted(SDL_ConvertSurface(surface.get(), SDL_PIXELFORMAT_RGBA32), &SDL_DestroySurface);
    if (!converted) {
        return std::unexpected(SDL_GetError());
    }

    engine::graphics::TextureData tex{};
    tex.width = converted->w;
    tex.height = converted->h;
    tex.bytes.reserve(tex.width * tex.height * 4);

    for (int h = 0; h < tex.height; h++) {
        for (int w = 0; w < tex.width; w++) {
            tex.bytes.push_back(*((std::uint8_t*)converted->pixels + converted->pitch * h + 4 * w));
            tex.bytes.push_back(*((std::uint8_t*)converted->pixels + converted->pitch * h + 4 * w + 1));
            tex.bytes.push_back(*((std::uint8_t*)converted->pixels + converted->pitch * h + 4 * w + 2));
            tex.bytes.push_back(*((std::uint8_t*)converted->pixels + converted->pitch * h + 4 * w + 3));
        }
    }

    return tex;
}

} // namespace engine::asset::import
