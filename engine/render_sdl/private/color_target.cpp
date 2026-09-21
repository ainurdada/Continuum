#include "color_target.h"

#include <utility>

#include <sdl_support/public/validation.h>

namespace engine::graphics {

ColorTarget::ColorTarget(ColorTarget&& other) noexcept {
    _device = other._device;
    _colorTexture = other._colorTexture;
    _width = other._width;
    _height = other._height;
    _format = other._format;

    other._device = nullptr;
    other._colorTexture = nullptr;
    other._width = 0;
    other._height = 0;
    other._format = {};
}

ColorTarget& ColorTarget::operator=(ColorTarget&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    release();

    _device = other._device;
    _colorTexture = other._colorTexture;
    _width = other._width;
    _height = other._height;
    _format = other._format;

    other._device = nullptr;
    other._colorTexture = nullptr;
    other._width = 0;
    other._height = 0;
    other._format = {};

    return *this;
}

ColorTarget::~ColorTarget() {
    release();
}

std::optional<ColorTarget> ColorTarget::create(SDL_GPUDevice* device, Uint32 width, Uint32 height, SDL_GPUTextureFormat format) {
    if (!device) {
        SDL_Log("null device");
        return std::nullopt;
    }
    if (width == 0) {
        SDL_Log("width = 0");
        return std::nullopt;
    }
    if (height == 0) {
        SDL_Log("height = 0");
        return std::nullopt;
    }
    if (format == SDL_GPU_TEXTUREFORMAT_INVALID) {
        SDL_Log("not valid format");
        return std::nullopt;
    }
    // Create color texture
    SDL_GPUTextureCreateInfo colorTextureCreateInfo{};
    colorTextureCreateInfo.type = SDL_GPU_TEXTURETYPE_2D;
    colorTextureCreateInfo.format = format;
    colorTextureCreateInfo.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
    colorTextureCreateInfo.width = width;
    colorTextureCreateInfo.height = height;
    colorTextureCreateInfo.layer_count_or_depth = 1;
    colorTextureCreateInfo.num_levels = 1;
    colorTextureCreateInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;
    colorTextureCreateInfo.props = 0;
    auto colorTexture = SDL_CreateGPUTexture(device, &colorTextureCreateInfo);
    if (!validate(colorTexture, "Create color texture")) {
        return std::nullopt;
    }

    return ColorTarget(device, colorTexture, width, height, format);
}

bool ColorTarget::resize(Uint32 width, Uint32 height) {
    // Check new sizes
    if (width == _width && height == _height) {
        return true;
    }

    // Create new color target
    std::optional<ColorTarget> newColorTarget = create(_device, width, height, _format);
    if (!newColorTarget) {
        return false;
    }

    // Apply new color target
    *this = std::move(newColorTarget.value());

    return true;
}

SDL_GPUTexture* ColorTarget::handle() const noexcept {
    return _colorTexture;
}

ColorTarget::ColorTarget(SDL_GPUDevice* device, SDL_GPUTexture* colorTexture, Uint32 width, Uint32 height, SDL_GPUTextureFormat format) {
    _device = device;
    _colorTexture = colorTexture;
    _width = width;
    _height = height;
    _format = format;
}

void ColorTarget::release() {
    if (_device && _colorTexture) {
        SDL_ReleaseGPUTexture(_device, _colorTexture);
    }
    _device = nullptr;
    _colorTexture = nullptr;
    _width = 0;
    _height = 0;
    _format = {};
}

} // namespace engine::graphics
