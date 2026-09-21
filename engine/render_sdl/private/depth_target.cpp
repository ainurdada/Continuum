#include "depth_target.h"

#include <sdl_support/public/validation.h>

namespace engine::graphics {

DepthTarget::DepthTarget(DepthTarget&& other) noexcept {
    _device = other._device;
    _depthTexture = other._depthTexture;
    _width = other._width;
    _height = other._height;

    other._device = nullptr;
    other._depthTexture = nullptr;
    other._width = 0;
    other._height = 0;
}

DepthTarget& DepthTarget::operator=(DepthTarget&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    release();

    _device = other._device;
    _depthTexture = other._depthTexture;
    _width = other._width;
    _height = other._height;

    other._device = nullptr;
    other._depthTexture = nullptr;
    other._width = 0;
    other._height = 0;

    return *this;
}

DepthTarget::~DepthTarget() {
    release();
}

std::optional<DepthTarget> DepthTarget::create(SDL_GPUDevice* device, Uint32 width, Uint32 height) {
    // Create depth texture
    SDL_GPUTextureCreateInfo depthTextureCreateInfo{};
    depthTextureCreateInfo.type = SDL_GPU_TEXTURETYPE_2D;
    depthTextureCreateInfo.format = format;
    depthTextureCreateInfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
    depthTextureCreateInfo.width = width;
    depthTextureCreateInfo.height = height;
    depthTextureCreateInfo.layer_count_or_depth = 1;
    depthTextureCreateInfo.num_levels = 1;
    depthTextureCreateInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;
    depthTextureCreateInfo.props = 0;
    auto depthTexture = SDL_CreateGPUTexture(device, &depthTextureCreateInfo);
    if (!validate(depthTexture, "Create depth texture")) {
        return std::nullopt;
    }

    return DepthTarget(device, depthTexture, width, height);
}

bool DepthTarget::resize(Uint32 width, Uint32 height) {
    // Check new sizes
    if (width == _width && height == _height) {
        return true;
    }

    // Create new depth target
    std::optional<DepthTarget> newDepthTarget = create(_device, width, height);
    if (!newDepthTarget) {
        return false;
    }

    // Apply new depth target
    *this = std::move(newDepthTarget.value());

    return true;
}

SDL_GPUTexture* DepthTarget::handle() const noexcept {
    return _depthTexture;
}

DepthTarget::DepthTarget(SDL_GPUDevice* device, SDL_GPUTexture* depthTexture, Uint32 width, Uint32 height){
    _device = device;
    _depthTexture = depthTexture;
    _width = width;
    _height = height;
}

void DepthTarget::release() {
    if (_device && _depthTexture) {
        SDL_ReleaseGPUTexture(_device, _depthTexture);
    }
    _device = nullptr;
    _depthTexture = nullptr;
    _width = 0;
    _height = 0;
}

} // namespace engine::graphics