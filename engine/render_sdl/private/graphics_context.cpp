#include "graphics_context.h"

#include <sdl_support/public/validation.h>
#include <graphics_platform.h>

namespace engine::graphics {

GraphicsContext::GraphicsContext(GraphicsContext&& other) noexcept {
    _window = other._window;
    _device = other._device;

    other._window = nullptr;
    other._device = nullptr;
}

GraphicsContext& GraphicsContext::operator=(GraphicsContext&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    release();

    _window = other._window;
    _device = other._device;

    other._window = nullptr;
    other._device = nullptr;

    return *this;
}

GraphicsContext::~GraphicsContext() {
    release();
}

std::optional<GraphicsContext> GraphicsContext::create(SDL_Window* window) {
    // Create GPU device
    auto device = SDL_CreateGPUDevice(shaderFormat, true, nullptr);
    if (!validate(device, "Create GPU device error")) {
        return std::nullopt;
    }
    SDL_Log("SDL device created: %s", SDL_GetGPUDeviceDriver(device));

    // Connect window and GPU device
    if (!validate(SDL_ClaimWindowForGPUDevice(device, window), "Connect window and GPU device error")) {
        SDL_DestroyGPUDevice(device);
        return std::nullopt;
    }

    return GraphicsContext(device, window);
}

SDL_GPUDevice* GraphicsContext::device() const noexcept {
    return _device;
}

GraphicsContext::GraphicsContext(SDL_GPUDevice* device, SDL_Window* window){
    _window = window;
    _device = device;
}

void GraphicsContext::release() {
    if (_device) {
        if (_window) {
            SDL_ReleaseWindowFromGPUDevice(_device, _window);
        }
        SDL_DestroyGPUDevice(_device);
    }
    _window = nullptr;
    _device = nullptr;
}

} // namespace engine::graphics