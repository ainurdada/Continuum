#pragma once

#include <SDL3/SDL_gpu.h>
#include <optional>

namespace engine::graphics {

class GraphicsPipeline {
  private:
    SDL_GPUDevice* _device{};
    SDL_GPUGraphicsPipeline* _graphicsPipeline{};

  public:
    GraphicsPipeline(const GraphicsPipeline&) = delete;
    GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;

    GraphicsPipeline(GraphicsPipeline&& other) noexcept;
    GraphicsPipeline& operator=(GraphicsPipeline&& other) noexcept;

    GraphicsPipeline() = delete;
    ~GraphicsPipeline();

    static std::optional<GraphicsPipeline> create(SDL_GPUDevice* device, const SDL_GPUGraphicsPipelineCreateInfo& createInfo);

    SDL_GPUGraphicsPipeline* handle() const noexcept;

  private:
    GraphicsPipeline(SDL_GPUDevice* device, SDL_GPUGraphicsPipeline* graphicsPipeline);

    void release();
};

} // namespace engine::graphics