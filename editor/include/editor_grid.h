#pragma once

#include <memory>
#include <optional>

#include <SDL3/SDL_gpu.h>

#include <render/public/render_frame_data.h>
#include <render_sdl/public/graphics_pipeline.h>
#include <render_sdl/public/mesh.h>

namespace editor {

class EditorGrid {
    std::unique_ptr<engine::graphics::Mesh> _mesh = nullptr;
    std::unique_ptr<engine::graphics::GraphicsPipeline> _pipeline = nullptr;

    EditorGrid(std::unique_ptr<engine::graphics::Mesh>& mesh, std::unique_ptr<engine::graphics::GraphicsPipeline>& pipeline);

  public:
    static std::optional<EditorGrid> create(SDL_GPUDevice* device, SDL_GPUTextureFormat colorTergetFormat);

    bool recordRenderPass(SDL_GPUCommandBuffer* commandBuffer, SDL_GPUTexture* colorTarget, SDL_GPUTexture* depthTarget, Uint32 width, Uint32 height, const std::optional<engine::RenderCameraData>& camera);
};

} // namespace editor