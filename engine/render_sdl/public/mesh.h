#pragma once

#include <SDL3/SDL_gpu.h>
#include <optional>

#include <render/public/mesh_data.h>

namespace engine::graphics {

class Mesh {
  private:
    SDL_GPUDevice* _device{};
    SDL_GPUBuffer* _vertexBuffer{};
    SDL_GPUBuffer* _indexBuffer{};
    Uint32 _numIndices{};

  public:
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    Mesh() = delete;
    ~Mesh();

    static std::optional<Mesh> load(SDL_GPUDevice* device, const MeshData& meshData);

    SDL_GPUBuffer* vertexBufferHandle() const noexcept;
    SDL_GPUBuffer* indexBufferHandle() const noexcept;
    Uint32 indexCount() const noexcept;

  private:
    Mesh(SDL_GPUDevice* device, SDL_GPUBuffer* vertexBuffer, SDL_GPUBuffer* indexBuffer, Uint32 numIndices);

    void release();
};

} // namespace engine::graphics