#include "graphics_pipeline.h"

#include <sdl_support/public/validation.h>

namespace engine::graphics {

GraphicsPipeline::GraphicsPipeline(GraphicsPipeline&& other) noexcept {
    _device = other._device;
    _graphicsPipeline = other._graphicsPipeline;

    other._device = nullptr;
    other._graphicsPipeline = nullptr;
}

GraphicsPipeline& GraphicsPipeline::operator=(GraphicsPipeline&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    release();

    _device = other._device;
    _graphicsPipeline = other._graphicsPipeline;

    other._device = nullptr;
    other._graphicsPipeline = nullptr;

    return *this;
}

GraphicsPipeline::~GraphicsPipeline() {
    release();
}

std::optional<GraphicsPipeline> GraphicsPipeline::create(SDL_GPUDevice* device, const SDL_GPUGraphicsPipelineCreateInfo& createInfo) {
    // Create graphics pipeline
    auto graphicsPipeline = SDL_CreateGPUGraphicsPipeline(device, &createInfo);
    if (!validate(graphicsPipeline, "Create graphics pipeline error")) {
        return std::nullopt;
    }

    return GraphicsPipeline(device, graphicsPipeline);
}

SDL_GPUGraphicsPipeline* GraphicsPipeline::handle() const noexcept {
    return _graphicsPipeline;
}

GraphicsPipeline::GraphicsPipeline(SDL_GPUDevice* device, SDL_GPUGraphicsPipeline* graphicsPipeline) {
    _device = device;
    _graphicsPipeline = graphicsPipeline;
}

void GraphicsPipeline::release() {
    if (_device && _graphicsPipeline) {
        SDL_ReleaseGPUGraphicsPipeline(_device, _graphicsPipeline);
    }
    _graphicsPipeline = nullptr;
    _device = nullptr;
}

} // namespace engine::graphics