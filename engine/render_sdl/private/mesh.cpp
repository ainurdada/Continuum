#include "mesh.h"

#include <sdl_support/public/validation.h>

namespace engine::graphics {

Mesh::Mesh(Mesh&& other) noexcept {
    _device = other._device;
    _vertexBuffer = other._vertexBuffer;
    _indexBuffer = other._indexBuffer;
    _numIndices = other._numIndices;

    other._device = nullptr;
    other._vertexBuffer = nullptr;
    other._indexBuffer = nullptr;
    other._numIndices = 0;
}

Mesh& Mesh::operator=(Mesh&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    release();
    _device = other._device;
    _vertexBuffer = other._vertexBuffer;
    _indexBuffer = other._indexBuffer;
    _numIndices = other._numIndices;

    other._device = nullptr;
    other._vertexBuffer = nullptr;
    other._indexBuffer = nullptr;
    other._numIndices = 0;

    return *this;
}

Mesh::~Mesh() {
    release();
}

std::optional<Mesh> Mesh::load(SDL_GPUDevice* device, const MeshData& meshData) {
    // get data sizes
    Uint32 vertexBytes = meshData.positionVertexBytes();
    Uint32 indexBytes = meshData.indexBytes();
    Uint32 numIndices = meshData.indices.size();

    // Create vertex buffer
    SDL_GPUBufferCreateInfo vertexBufferCreateInfo{};
    vertexBufferCreateInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    vertexBufferCreateInfo.size = vertexBytes;
    vertexBufferCreateInfo.props = 0;
    auto vertexBuffer = SDL_CreateGPUBuffer(device, &vertexBufferCreateInfo);
    if (!validate(vertexBuffer, "Create vertex buffer error")) {
        return std::nullopt;
    }

    // Create index buffer
    SDL_GPUBufferCreateInfo indexBufferCreateInfo{};
    indexBufferCreateInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    indexBufferCreateInfo.size = indexBytes;
    indexBufferCreateInfo.props = 0;
    auto indexBuffer = SDL_CreateGPUBuffer(device, &indexBufferCreateInfo);
    if (!validate(indexBuffer, "Create index buffer error")) {
        SDL_ReleaseGPUBuffer(device, vertexBuffer);
        return std::nullopt;
    }

    // Create vertex upload buffer
    SDL_GPUTransferBufferCreateInfo vertexUploadBufferCreateInfo{};
    vertexUploadBufferCreateInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    vertexUploadBufferCreateInfo.size = vertexBytes;
    vertexUploadBufferCreateInfo.props = 0;
    auto vertexUploadBuffer = SDL_CreateGPUTransferBuffer(device, &vertexUploadBufferCreateInfo);
    if (!validate(vertexUploadBuffer, "Create vertex upload buffer error")) {
        SDL_ReleaseGPUBuffer(device, vertexBuffer);
        SDL_ReleaseGPUBuffer(device, indexBuffer);
        return std::nullopt;
    }

    // Create index upload buffer
    SDL_GPUTransferBufferCreateInfo indexUploadBufferCreateInfo{};
    indexUploadBufferCreateInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    indexUploadBufferCreateInfo.size = indexBytes;
    indexUploadBufferCreateInfo.props = 0;
    auto indexUploadBuffer = SDL_CreateGPUTransferBuffer(device, &indexUploadBufferCreateInfo);
    if (!validate(indexUploadBuffer, "Create index upload buffer error")) {
        SDL_ReleaseGPUBuffer(device, vertexBuffer);
        SDL_ReleaseGPUBuffer(device, indexBuffer);
        SDL_ReleaseGPUTransferBuffer(device, vertexUploadBuffer);
        return std::nullopt;
    }

    // Map vertex data
    void* mappedVertexBuffer = SDL_MapGPUTransferBuffer(device, vertexUploadBuffer, false);
    if (!validate(mappedVertexBuffer, "Map vertex data error")) {
        SDL_ReleaseGPUBuffer(device, vertexBuffer);
        SDL_ReleaseGPUBuffer(device, indexBuffer);
        SDL_ReleaseGPUTransferBuffer(device, vertexUploadBuffer);
        SDL_ReleaseGPUTransferBuffer(device, indexUploadBuffer);
        return std::nullopt;
    }
    SDL_memcpy(mappedVertexBuffer, meshData.positionVertices.data(), vertexBytes);
    SDL_UnmapGPUTransferBuffer(device, vertexUploadBuffer);

    // Map index data
    void* mappedIndexBuffer = SDL_MapGPUTransferBuffer(device, indexUploadBuffer, false);
    if (!validate(mappedIndexBuffer, "Map index data error")) {
        SDL_ReleaseGPUBuffer(device, vertexBuffer);
        SDL_ReleaseGPUBuffer(device, indexBuffer);
        SDL_ReleaseGPUTransferBuffer(device, vertexUploadBuffer);
        SDL_ReleaseGPUTransferBuffer(device, indexUploadBuffer);
        return std::nullopt;
    }
    SDL_memcpy(mappedIndexBuffer, meshData.indices.data(), indexBytes);
    SDL_UnmapGPUTransferBuffer(device, indexUploadBuffer);

    // Start copy pass
    SDL_GPUCommandBuffer* gpuCopyCommand = SDL_AcquireGPUCommandBuffer(device);
    if (!validate(gpuCopyCommand, "Start send vertex data error")) {
        SDL_ReleaseGPUBuffer(device, vertexBuffer);
        SDL_ReleaseGPUBuffer(device, indexBuffer);
        SDL_ReleaseGPUTransferBuffer(device, vertexUploadBuffer);
        SDL_ReleaseGPUTransferBuffer(device, indexUploadBuffer);
        return std::nullopt;
    }
    SDL_GPUCopyPass* gpuCopyPass = SDL_BeginGPUCopyPass(gpuCopyCommand);
    if (!validate(gpuCopyPass, "Begin copy pass error")) {
        validate(SDL_CancelGPUCommandBuffer(gpuCopyCommand), "Cancel command buffer error");
        SDL_ReleaseGPUBuffer(device, vertexBuffer);
        SDL_ReleaseGPUBuffer(device, indexBuffer);
        SDL_ReleaseGPUTransferBuffer(device, vertexUploadBuffer);
        SDL_ReleaseGPUTransferBuffer(device, indexUploadBuffer);
        return std::nullopt;
    }

    // Send vertex data
    SDL_GPUTransferBufferLocation vertexTransferLocation{};
    vertexTransferLocation.transfer_buffer = vertexUploadBuffer;
    vertexTransferLocation.offset = 0;
    SDL_GPUBufferRegion vertexBufferRegion{};
    vertexBufferRegion.buffer = vertexBuffer;
    vertexBufferRegion.offset = 0;
    vertexBufferRegion.size = vertexBytes;
    SDL_UploadToGPUBuffer(gpuCopyPass, &vertexTransferLocation, &vertexBufferRegion, false);

    // Send index data
    SDL_GPUTransferBufferLocation indexTransferLocation{};
    indexTransferLocation.transfer_buffer = indexUploadBuffer;
    indexTransferLocation.offset = 0;
    SDL_GPUBufferRegion indexBufferRegion{};
    indexBufferRegion.buffer = indexBuffer;
    indexBufferRegion.offset = 0;
    indexBufferRegion.size = indexBytes;
    SDL_UploadToGPUBuffer(gpuCopyPass, &indexTransferLocation, &indexBufferRegion, false);

    // End copy pass
    SDL_EndGPUCopyPass(gpuCopyPass);
    if (!validate(SDL_SubmitGPUCommandBuffer(gpuCopyCommand), "Finish send index data")) {
        SDL_ReleaseGPUBuffer(device, vertexBuffer);
        SDL_ReleaseGPUBuffer(device, indexBuffer);
        SDL_ReleaseGPUTransferBuffer(device, vertexUploadBuffer);
        SDL_ReleaseGPUTransferBuffer(device, indexUploadBuffer);
        return std::nullopt;
    }
    SDL_ReleaseGPUTransferBuffer(device, vertexUploadBuffer);
    vertexUploadBuffer = nullptr;
    SDL_ReleaseGPUTransferBuffer(device, indexUploadBuffer);
    indexUploadBuffer = nullptr;

    return Mesh(device, vertexBuffer, indexBuffer, numIndices);
}

SDL_GPUBuffer* Mesh::vertexBufferHandle() const noexcept {
    return _vertexBuffer;
}

SDL_GPUBuffer* Mesh::indexBufferHandle() const noexcept {
    return _indexBuffer;
}

Uint32 Mesh::indexCount() const noexcept {
    return _numIndices;
}

Mesh::Mesh(SDL_GPUDevice* device, SDL_GPUBuffer* vertexBuffer, SDL_GPUBuffer* indexBuffer, Uint32 numIndices) {
    _device = device;
    _vertexBuffer = vertexBuffer;
    _indexBuffer = indexBuffer;
    _numIndices = numIndices;
}

void Mesh::release() {
    if (_device) {
        if (_vertexBuffer) {
            SDL_ReleaseGPUBuffer(_device, _vertexBuffer);
        }
        if (_indexBuffer) {
            SDL_ReleaseGPUBuffer(_device, _indexBuffer);
        }
    }
    _vertexBuffer = nullptr;
    _indexBuffer = nullptr;
    _device = nullptr;
    _numIndices = 0;
}

} // namespace engine::graphics