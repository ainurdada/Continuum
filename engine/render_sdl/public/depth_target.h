#pragma once

#include <SDL3/SDL_gpu.h>
#include <optional>

namespace engine::graphics {

class DepthTarget {
  private:
    SDL_GPUDevice* _device{};
    SDL_GPUTexture* _depthTexture{};
    Uint32 _width{};
    Uint32 _height{};

  public:
    static const SDL_GPUTextureFormat format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;

    DepthTarget(const DepthTarget&) = delete;
    DepthTarget& operator=(const DepthTarget&) = delete;

    DepthTarget(DepthTarget&& other) noexcept;
    DepthTarget& operator=(DepthTarget&& other) noexcept;

    DepthTarget() = delete;
    ~DepthTarget();

    static std::optional<DepthTarget> create(SDL_GPUDevice* device, Uint32 width, Uint32 height);

    bool resize(Uint32 width, Uint32 height);

    SDL_GPUTexture* handle() const noexcept;

  private:
    DepthTarget(SDL_GPUDevice* device, SDL_GPUTexture* depthTexture, Uint32 width, Uint32 height);

    void release();
};

} // namespace engine::graphics