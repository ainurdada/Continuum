#pragma once

#include <SDL3/SDL_gpu.h>
#include <optional>

namespace engine::graphics {

class GraphicsContext {
  private:
    SDL_GPUDevice* _device{};
    SDL_Window* _window{};

  public:
    GraphicsContext(const GraphicsContext&) = delete;
    GraphicsContext& operator=(const GraphicsContext&) = delete;

    GraphicsContext(GraphicsContext&& other) noexcept;
    GraphicsContext& operator=(GraphicsContext&& other) noexcept;

    GraphicsContext() = delete;
    ~GraphicsContext();

    static std::optional<GraphicsContext> create(SDL_Window* window);

    SDL_GPUDevice* device() const noexcept;

  private:
    GraphicsContext(SDL_GPUDevice* device, SDL_Window* window);

    void release();
};

} // namespace engine::graphics