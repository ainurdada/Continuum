#pragma once

#include <optional>
#include <render/public/render_frame_data.h>
#include "depth_target.h"
#include "graphics_context.h"
#include "graphics_pipeline.h"
#include "mesh.h"

namespace engine::graphics {

class Renderer {
  private:
    SDL_Window* _window{};
    GraphicsContext* _graphicsContext{};

    GraphicsPipeline _graphicsPipeline;
    GraphicsPipeline _gridGraphicsPipeline;
    DepthTarget _depthTarget;

    Mesh _cubeMesh;
    Mesh _gridMesh;

  public:
    static std::optional<Renderer> create(SDL_Window* window, GraphicsContext* graphicsContext);

    bool renderFrame(const RenderFrameData& renderFrameData);

    bool recordRenderPass(SDL_GPUCommandBuffer* commandBuffer, SDL_GPUTexture* colorTarget, Uint32 width, Uint32 height, const RenderFrameData& renderFrameData);

  private:
    Renderer(SDL_Window* window, GraphicsContext* graphicsContext, GraphicsPipeline&& graphicsPipeline, GraphicsPipeline&& gridGraphicsPipeline, DepthTarget&& depthTarget, Mesh&& cubeMesh, Mesh&& globalGridMesh);
};

} // namespace engine::graphics
