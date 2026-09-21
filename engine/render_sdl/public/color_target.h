#pragma once

#include <SDL3/SDL_gpu.h>
#include <optional>

namespace engine::graphics
{
    
class ColorTarget {
  private:
    SDL_GPUDevice* _device{};
    SDL_GPUTexture* _colorTexture{};
    Uint32 _width{};
    Uint32 _height{};
    SDL_GPUTextureFormat _format{};

  public:

    ColorTarget(const ColorTarget&) = delete;
    ColorTarget& operator=(const ColorTarget&) = delete;

    ColorTarget(ColorTarget&& other) noexcept;
    ColorTarget& operator=(ColorTarget&& other) noexcept;

    ColorTarget() = delete;
    ~ColorTarget();

    static std::optional<ColorTarget> create(SDL_GPUDevice* device, Uint32 width, Uint32 height, SDL_GPUTextureFormat format);

    bool resize(Uint32 width, Uint32 height);

    SDL_GPUTexture* handle() const noexcept;

  private:
    ColorTarget(SDL_GPUDevice* device, SDL_GPUTexture* colorTexture, Uint32 width, Uint32 height, SDL_GPUTextureFormat format);

    void release();
};

} // namespace engine::graphics
