#include "renderer.h"

#include <unordered_map>

#include "shader.h"
#include <math/public/g_math.h>
#include <sdl_support/public/validation.h>
#include <vertex_layout.h>

namespace engine::graphics {

namespace {
struct CubeVertex {
    Vec3f position;
};

// clang-format off
MeshData cubeMeshData() {
    std::vector<CubeVertex> vertices = {
        CubeVertex{ Vec3f{-0.5, -0.5, -0.5} },
        CubeVertex{ Vec3f{-0.5, 0.5, -0.5} },
        CubeVertex{ Vec3f{0.5, 0.5, -0.5} },
        CubeVertex{ Vec3f{0.5, -0.5, -0.5} },
        CubeVertex{ Vec3f{-0.5, -0.5, 0.5} },
        CubeVertex{ Vec3f{-0.5, 0.5, 0.5} },
        CubeVertex{ Vec3f{0.5, 0.5, 0.5} },
        CubeVertex{ Vec3f{0.5, -0.5, 0.5} },
    };

    std::vector<VertexAttributeDescription> descs = {
        VertexAttributeDescription{0, VertexAttributeFormat::Float3, 0}
    };

    MeshData data;
    data.vertices = makeVerticesData<CubeVertex>(vertices, descs);
    data.indices = {
        0, 1, 2,  0, 2, 3,
        4, 6, 5,  4, 7, 6,
        0, 4, 5,  0, 5, 1,
        3, 2, 6,  3, 6, 7,
        0, 3, 7,  0, 7, 4,
        1, 5, 6,  1, 6, 2
    };

    return data;
}
// clang-format on

// clang-format off
struct TransformUniform {
    Mat4f projection{};
    Mat4f view{};
    Mat4f model{};
};
// clang-format on

static_assert(sizeof(TransformUniform) == 192);

} // namespace

std::optional<Renderer> Renderer::create(SDL_Window* window, GraphicsContext* graphicsContext) {
    // Load cube mesh
    auto cube = cubeMeshData();
    auto mesh = Mesh::load(graphicsContext->device(), cube);
    if (!mesh) {
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
    fragmentShaderInfo.numUniformBuffers = 1;
    auto fragmentShader = Shader::load(graphicsContext->device(), fragmentShaderInfo);
    if (!fragmentShader) {
        return std::nullopt;
    }

    // Get swapchain texture format
    SDL_GPUTextureFormat swapchaintTextureFormat = SDL_GetGPUSwapchainTextureFormat(graphicsContext->device(), window);
    SDL_GPUColorTargetDescription colorTargetDescription{};
    colorTargetDescription.format = swapchaintTextureFormat;
    colorTargetDescription.blend_state = {};

    // Create graphics pipeline
    auto attributes = convertVertexAttributeToSDL(cube.vertices.descs);
    SDL_GPUVertexBufferDescription vertexBufferDescription{};
    vertexBufferDescription.slot = 0;
    vertexBufferDescription.pitch = cube.vertices.pitch;
    vertexBufferDescription.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vertexBufferDescription.instance_step_rate = 0;
    SDL_GPUGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{};
    graphicsPipelineCreateInfo.vertex_shader = vertexShader->handle();
    graphicsPipelineCreateInfo.fragment_shader = fragmentShader->handle();
    graphicsPipelineCreateInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    graphicsPipelineCreateInfo.vertex_input_state.vertex_buffer_descriptions = &vertexBufferDescription;
    graphicsPipelineCreateInfo.vertex_input_state.num_vertex_buffers = 1;
    graphicsPipelineCreateInfo.vertex_input_state.vertex_attributes = attributes.data();
    graphicsPipelineCreateInfo.vertex_input_state.num_vertex_attributes = attributes.size();
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

    return Renderer(window, graphicsContext, std::move(graphicsPipeline.value()), std::move(depthTarget.value()), std::move(mesh.value()));
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

    // Resize depth target
    if (!_depthTarget.resize(swapchainTextureWidth, swapchainTextureHeight)) {
        validate(SDL_SubmitGPUCommandBuffer(commandBuffer), "Submit GPU command buffer error");
        return false;
    }

    // Record render pass
    if (!recordRenderPass(commandBuffer, swapchainTexture, _depthTarget.handle(), swapchainTextureWidth, swapchainTextureHeight, renderFrameData)) {
        validate(SDL_SubmitGPUCommandBuffer(commandBuffer), "Submit GPU command buffer error");
        return false;
    }

    // Submit command buffer
    if (!validate(SDL_SubmitGPUCommandBuffer(commandBuffer), "Submit GPU command buffer error")) {
        return false;
    }

    return true;
}

bool Renderer::recordRenderPass(SDL_GPUCommandBuffer* commandBuffer, SDL_GPUTexture* colorTarget, SDL_GPUTexture* depthTarget, Uint32 width, Uint32 height, const RenderFrameData& renderFrameData) {
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

    if (!renderFrameData.camera) {
        return true;
    }

    if (!depthTarget) {
        return false;
    }

    // Create color target
    SDL_GPUColorTargetInfo colorTargetInfo{};
    colorTargetInfo.texture = colorTarget;
    colorTargetInfo.clear_color = SDL_FColor{9.0f / 255.0f, 15.0f / 255.0f, 24.0f / 255.0f, 1.0f};
    colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

    // Create depth stencil target
    SDL_GPUDepthStencilTargetInfo depthStencilTargetInfo{};
    depthStencilTargetInfo.texture = depthTarget;
    depthStencilTargetInfo.clear_depth = 1.0;
    depthStencilTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
    depthStencilTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
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
    transformUniform.projection = math::perspective(renderFrameData.camera->verticalFovRadians, static_cast<float>(width) / static_cast<float>(height), renderFrameData.camera->nearPlane, renderFrameData.camera->farPlane);
    transformUniform.view = renderFrameData.camera->viewMatrix;
    for (const auto& item : renderFrameData.items) {
        transformUniform.model = item.modelMatrix;
        SDL_PushGPUVertexUniformData(commandBuffer, 0, &transformUniform, sizeof(TransformUniform));
        SDL_PushGPUFragmentUniformData(commandBuffer, 0, &item.baseColor, sizeof(Vec3f));

        const Mesh* mesh = nullptr;
        switch (item.geometryId) {

        case GeometryId::Cube: {
            mesh = &_cubeMesh;
            break;
        }

        case GeometryId::UploadedMesh: {
            if (item.meshHandle == 0 || item.meshHandle > _meshes.size()) {
                continue;
            }
            mesh = &_meshes.at(item.meshHandle - 1);
            break;
        }

        default:
            continue;
        }

        // Bind vertex buffer
        SDL_GPUBufferBinding vertexBufferBinding{};
        vertexBufferBinding.buffer = mesh->vertexBufferHandle();
        vertexBufferBinding.offset = 0;
        SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBufferBinding, 1);

        // Bind index buffer
        SDL_GPUBufferBinding indexBufferBinding{};
        indexBufferBinding.buffer = mesh->indexBufferHandle();
        indexBufferBinding.offset = 0;
        SDL_BindGPUIndexBuffer(renderPass, &indexBufferBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

        // Draw
        SDL_DrawGPUIndexedPrimitives(renderPass, mesh->indexCount(), 1, 0, 0, 0);
    }

    // End render pass
    SDL_EndGPURenderPass(renderPass);
    return true;
}

std::optional<MeshHandle> Renderer::uploadMesh(const MeshData& data) {
    auto mesh = Mesh::load(_graphicsContext->device(), data);
    if (!mesh) {
        return std::nullopt;
    }

    _meshes.push_back(std::move(mesh.value()));
    return _meshes.size();
}

Renderer::Renderer(SDL_Window* window, GraphicsContext* graphicsContext, GraphicsPipeline&& graphicsPipeline, DepthTarget&& depthTarget, Mesh&& cubeMesh) : _graphicsPipeline(std::move(graphicsPipeline)), _depthTarget(std::move(depthTarget)), _cubeMesh(std::move(cubeMesh)) {
    _window = window;
    _graphicsContext = graphicsContext;
}

} // namespace engine::graphics