#pragma once

#include <optional>
#include <vector>

#include "depth_target.h"
#include "graphics_context.h"
#include "graphics_pipeline.h"
#include "mesh.h"
#include <render/public/mesh_data.h>
#include <render/public/render_frame_data.h>

namespace engine::graphics {

class Renderer {
  private:
    SDL_Window* _window{};
    GraphicsContext* _graphicsContext{};

    GraphicsPipeline _graphicsPipeline;
    DepthTarget _depthTarget;

    Mesh _cubeMesh;

    std::vector<Mesh> _meshes{};

  public:
    static std::optional<Renderer> create(SDL_Window* window, GraphicsContext* graphicsContext);

    bool renderFrame(const RenderFrameData& renderFrameData);

    bool recordRenderPass(SDL_GPUCommandBuffer* commandBuffer, SDL_GPUTexture* colorTarget, SDL_GPUTexture* depthTarget, Uint32 width, Uint32 height, const RenderFrameData& renderFrameData);

    std::optional<MeshHandle> uploadMesh(const MeshData& data);

  private:
    Renderer(SDL_Window* window, GraphicsContext* graphicsContext, GraphicsPipeline&& graphicsPipeline, DepthTarget&& depthTarget, Mesh&& cubeMesh);
};

} // namespace engine::graphics
