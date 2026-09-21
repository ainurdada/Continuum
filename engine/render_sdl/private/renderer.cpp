#include "renderer.h"

#include <sdl_support/public/validation.h>
#include "shader.h"
#include <math/public/g_math.h>

namespace engine::graphics {

// clang-format off
const MeshData cubeMeshData{
    .positionVertices = {
        PositionVertex{-0.5, -0.5, -0.5},
        PositionVertex{-0.5, 0.5, -0.5},
        PositionVertex{0.5, 0.5, -0.5},
        PositionVertex{0.5, -0.5, -0.5},
        PositionVertex{-0.5, -0.5, 0.5},
        PositionVertex{-0.5, 0.5, 0.5},
        PositionVertex{0.5, 0.5, 0.5},
        PositionVertex{0.5, -0.5, 0.5}
    },
    .indices = {
        0, 1, 2,  0, 2, 3,
        4, 6, 5,  4, 7, 6,
        0, 4, 5,  0, 5, 1,
        3, 2, 6,  3, 6, 7,
        0, 3, 7,  0, 7, 4,
        1, 5, 6,  1, 6, 2
    }
};
// clang-format on

// clang-format off
struct TransformUniform {
    Mat4f projection{};
    Mat4f view{};
    Mat4f model{};
};
// clang-format on

static_assert(sizeof(TransformUniform) == 192);

MeshData createGlobalGridMeshData(int halfCellCount, float spacingMeters) {
    MeshData grid{};
    for (int i = -halfCellCount; i <= halfCellCount; i++) {
        std::uint16_t currentIndex = static_cast<uint16_t>(grid.positionVertices.size());
        grid.positionVertices.push_back(PositionVertex{-halfCellCount * spacingMeters, 0, i * spacingMeters});
        grid.positionVertices.push_back(PositionVertex{halfCellCount * spacingMeters, 0, i * spacingMeters});
        grid.positionVertices.push_back(PositionVertex{i * spacingMeters, 0, -halfCellCount * spacingMeters});
        grid.positionVertices.push_back(PositionVertex{i * spacingMeters, 0, halfCellCount * spacingMeters});
        grid.indices.push_back(currentIndex);
        grid.indices.push_back(currentIndex + 1);
        grid.indices.push_back(currentIndex + 2);
        grid.indices.push_back(currentIndex + 3);
    }
    return grid;
}

std::optional<Renderer> Renderer::create(SDL_Window* window, GraphicsContext* graphicsContext) {
    // Load cube mesh
    auto mesh = Mesh::load(graphicsContext->device(), cubeMeshData);
    if (!mesh) {
        return std::nullopt;
    }

    // Load global grid
    auto globalGridMesh = Mesh::load(graphicsContext->device(), createGlobalGridMeshData(20, 1));
    if (!globalGridMesh) {
        return std::nullopt;
    }

    // Get window sizes
    int windowWidth{}, windowHeight{};
    if (!validate(SDL_GetWindowSizeInPixels(window, &windowWidth, &windowHeight), "Get window size in pixel")) {
        return std::nullopt;
    }

    // Create depth target
    auto depthTarget = DepthTarget::create(graphicsContext->device(), windowWidth, windowHeight);
    if (!depthTarget) {
        return std::nullopt;
    }

    // Create shaders
    using namespace engine::graphics;
    ShaderLoadInfo vertexShaderInfo{};
    vertexShaderInfo.name = "default.vert";
    vertexShaderInfo.shaderStage = ShaderStage::Vertex;
    vertexShaderInfo.numUniformBuffers = 1;
    auto vertexShader = Shader::load(graphicsContext->device(), vertexShaderInfo);
    if (!vertexShader) {
        return std::nullopt;
    }
    ShaderLoadInfo fragmentShaderInfo{};
    fragmentShaderInfo.name = "default.frag";
    fragmentShaderInfo.shaderStage = ShaderStage::Fragment;
    fragmentShaderInfo.numUniformBuffers = 0;
    auto fragmentShader = Shader::load(graphicsContext->device(), fragmentShaderInfo);
    if (!fragmentShader) {
        return std::nullopt;
    }
    ShaderLoadInfo gridFragmentShaderInfo{};
    gridFragmentShaderInfo.name = "grid.frag";
    gridFragmentShaderInfo.shaderStage = ShaderStage::Fragment;
    gridFragmentShaderInfo.numUniformBuffers = 0;
    auto gridFragmentShader = Shader::load(graphicsContext->device(), gridFragmentShaderInfo);
    if (!gridFragmentShader) {
        return std::nullopt;
    }

    // Get swapchain texture format
    SDL_GPUTextureFormat swapchaintTextureFormat = SDL_GetGPUSwapchainTextureFormat(graphicsContext->device(), window);
    SDL_GPUColorTargetDescription colorTargetDescription{};
    colorTargetDescription.format = swapchaintTextureFormat;
    colorTargetDescription.blend_state = {};

    // Create graphics pipeline
    SDL_GPUVertexBufferDescription vertexBufferDescription{};
    vertexBufferDescription.slot = 0;
    vertexBufferDescription.pitch = sizeof(PositionVertex);
    vertexBufferDescription.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vertexBufferDescription.instance_step_rate = 0;
    SDL_GPUVertexAttribute vertexAttribute{};
    vertexAttribute.location = 0;
    vertexAttribute.buffer_slot = 0;
    vertexAttribute.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    vertexAttribute.offset = 0;
    SDL_GPUGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{};
    graphicsPipelineCreateInfo.vertex_shader = vertexShader->handle();
    graphicsPipelineCreateInfo.fragment_shader = fragmentShader->handle();
    graphicsPipelineCreateInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    graphicsPipelineCreateInfo.vertex_input_state.vertex_buffer_descriptions = &vertexBufferDescription;
    graphicsPipelineCreateInfo.vertex_input_state.num_vertex_buffers = 1;
    graphicsPipelineCreateInfo.vertex_input_state.vertex_attributes = &vertexAttribute;
    graphicsPipelineCreateInfo.vertex_input_state.num_vertex_attributes = 1;
    graphicsPipelineCreateInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    graphicsPipelineCreateInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    graphicsPipelineCreateInfo.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
    graphicsPipelineCreateInfo.rasterizer_state.enable_depth_clip = true;
    graphicsPipelineCreateInfo.multisample_state.sample_count = SDL_GPU_SAMPLECOUNT_1;
    graphicsPipelineCreateInfo.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS;
    graphicsPipelineCreateInfo.depth_stencil_state.enable_depth_test = true;
    graphicsPipelineCreateInfo.depth_stencil_state.enable_depth_write = true;
    graphicsPipelineCreateInfo.depth_stencil_state.enable_stencil_test = false;
    graphicsPipelineCreateInfo.target_info.color_target_descriptions = &colorTargetDescription;
    graphicsPipelineCreateInfo.target_info.num_color_targets = 1;
    graphicsPipelineCreateInfo.target_info.has_depth_stencil_target = true;
    graphicsPipelineCreateInfo.target_info.depth_stencil_format = DepthTarget::format;
    graphicsPipelineCreateInfo.props = 0;
    auto graphicsPipeline = GraphicsPipeline::create(graphicsContext->device(), graphicsPipelineCreateInfo);
    if (!graphicsPipeline) {
        return std::nullopt;
    }

    graphicsPipelineCreateInfo.fragment_shader = gridFragmentShader.value().handle();
    graphicsPipelineCreateInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_LINELIST;
    graphicsPipelineCreateInfo.depth_stencil_state.enable_depth_write = false;
    auto gridGraphicsPipeline = GraphicsPipeline::create(graphicsContext->device(), graphicsPipelineCreateInfo);
    if (!gridGraphicsPipeline) {
        return std::nullopt;
    }

    return Renderer(window, graphicsContext, std::move(graphicsPipeline.value()), std::move(gridGraphicsPipeline.value()), std::move(depthTarget.value()), std::move(mesh.value()), std::move(globalGridMesh.value()));
}

bool Renderer::renderFrame(const RenderFrameData& renderFrameData) {
    // Create command buffer
    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(_graphicsContext->device());
    if (!validate(commandBuffer, "Acquire GPU command buffer error")) {
        return false;
    }

    // get swapchain texture
    SDL_GPUTexture* swapchainTexture{};
    Uint32 swapchainTextureWidth{};
    Uint32 swapchainTextureHeight{};
    if (!validate(SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, _window, &swapchainTexture, &swapchainTextureWidth, &swapchainTextureHeight), "Get swap chain texture error")) {
        if (!validate(SDL_CancelGPUCommandBuffer(commandBuffer), "Cancel GPU command buffer error")) {
            return false;
        }
        return false;
    }
    if (!swapchainTexture) {
        if (!validate(SDL_CancelGPUCommandBuffer(commandBuffer), "Cancel GPU command buffer error")) {
            return false;
        }
        return true;
    }

    // Record render pass
    if (!recordRenderPass(commandBuffer, swapchainTexture, swapchainTextureWidth, swapchainTextureHeight, renderFrameData)) {
        validate(SDL_CancelGPUCommandBuffer(commandBuffer), "Cancel GPU command buffer error");
        return false;
    }

    // Submit command buffer
    if (!validate(SDL_SubmitGPUCommandBuffer(commandBuffer), "Submit GPU command buffer error")) {
        return false;
    }

    return true;
}

bool Renderer::recordRenderPass(SDL_GPUCommandBuffer* commandBuffer, SDL_GPUTexture* colorTarget, Uint32 width, Uint32 height, const RenderFrameData& renderFrameData) {
    // initial validation
    if (!commandBuffer) {
        SDL_Log("command buffer is null");
        return false;
    }
    if (!colorTarget) {
        SDL_Log("color target is null");
        return false;
    }
    if (width == 0) {
        SDL_Log("width is not greater than 0");
        return false;
    }
    if (height == 0) {
        SDL_Log("height is not greater than 0");
        return false;
    }

    // Resize depth target
    if (!_depthTarget.resize(width, height)) {
        return false;
    }

    // Create color target
    SDL_GPUColorTargetInfo colorTargetInfo{};
    colorTargetInfo.texture = colorTarget;
    colorTargetInfo.clear_color = SDL_FColor{0.00647, 0, 0.0858, 1};
    colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

    // Create depth stencil target
    SDL_GPUDepthStencilTargetInfo depthStencilTargetInfo{};
    depthStencilTargetInfo.texture = _depthTarget.handle();
    depthStencilTargetInfo.clear_depth = 1.0;
    depthStencilTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
    depthStencilTargetInfo.store_op = SDL_GPU_STOREOP_DONT_CARE;
    depthStencilTargetInfo.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
    depthStencilTargetInfo.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;

    // Begin render pass
    SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, &depthStencilTargetInfo);
    if (!validate(renderPass, "Begin render pass error")) {
        return false;
    }

    // Bind pipeline
    SDL_BindGPUGraphicsPipeline(renderPass, _graphicsPipeline.handle());

    // Send uniform data
    TransformUniform transformUniform{};
    transformUniform.projection = math::perspective(renderFrameData.camera.verticalFovRadians, static_cast<float>(width) / static_cast<float>(height), renderFrameData.camera.nearPlane, renderFrameData.camera.farPlane);
    transformUniform.view = renderFrameData.camera.viewMatrix;
    for (const auto& item : renderFrameData.items) {
        transformUniform.model = item.modelMatrix;
        SDL_PushGPUVertexUniformData(commandBuffer, 0, &transformUniform, sizeof(TransformUniform));

        Uint32 indexCount{};
        switch (item.geometryId) {
        case GeometryId::Cube:
            // Bind vertex buffer
            SDL_GPUBufferBinding vertexBufferBinding{};
            vertexBufferBinding.buffer = _cubeMesh.vertexBufferHandle();
            vertexBufferBinding.offset = 0;
            SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBufferBinding, 1);

            // Bind index buffer
            SDL_GPUBufferBinding indexBufferBinding{};
            indexBufferBinding.buffer = _cubeMesh.indexBufferHandle();
            indexBufferBinding.offset = 0;
            SDL_BindGPUIndexBuffer(renderPass, &indexBufferBinding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

            indexCount = _cubeMesh.indexCount();
            break;
        }

        // Draw
        SDL_DrawGPUIndexedPrimitives(renderPass, indexCount, 1, 0, 0, 0);
    }

    // draw global grid
    if (renderFrameData.drawGlobalGrid) {
        // Bind grid pipeline
        SDL_BindGPUGraphicsPipeline(renderPass, _gridGraphicsPipeline.handle());
        transformUniform.model = math::identity();
        SDL_PushGPUVertexUniformData(commandBuffer, 0, &transformUniform, sizeof(TransformUniform));
        // Bind vertex buffer
        SDL_GPUBufferBinding vertexBufferBinding{};
        vertexBufferBinding.buffer = _gridMesh.vertexBufferHandle();
        vertexBufferBinding.offset = 0;
        SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBufferBinding, 1);
        // Bind index buffer
        SDL_GPUBufferBinding indexBufferBinding{};
        indexBufferBinding.buffer = _gridMesh.indexBufferHandle();
        indexBufferBinding.offset = 0;
        SDL_BindGPUIndexBuffer(renderPass, &indexBufferBinding, SDL_GPU_INDEXELEMENTSIZE_16BIT);
        // Draw
        SDL_DrawGPUIndexedPrimitives(renderPass, _gridMesh.indexCount(), 1, 0, 0, 0);
    }

    // End render pass
    SDL_EndGPURenderPass(renderPass);
    return true;
}

Renderer::Renderer(SDL_Window* window, GraphicsContext* graphicsContext, GraphicsPipeline&& graphicsPipeline, GraphicsPipeline&& gridGraphicsPipeline, DepthTarget&& depthTarget, Mesh&& cubeMesh, Mesh&& globalGridMesh)
    : _graphicsPipeline(std::move(graphicsPipeline)), _gridGraphicsPipeline(std::move(gridGraphicsPipeline)), _depthTarget(std::move(depthTarget)), _cubeMesh(std::move(cubeMesh)), _gridMesh(std::move(globalGridMesh)) {
    _window = window;
    _graphicsContext = graphicsContext;
}

} // namespace engine::graphics