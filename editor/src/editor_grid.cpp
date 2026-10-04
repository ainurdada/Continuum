#include <editor_grid.h>

#include <render/public/mesh_data.h>
#include <render_sdl/public/depth_target.h>
#include <render_sdl/public/shader.h>
#include <render_sdl/public/vertex_layout.h>
#include <sdl_support/public/validation.h>

namespace editor {

namespace {

struct TransformUniform {
    Mat4f projection{};
    Mat4f view{};
    Mat4f model{};
};

engine::graphics::MeshData createGlobalGridMeshData(int halfCellCount, float spacingMeters) {
    engine::graphics::MeshData grid{};
    std::vector<Vec3f> positionVertices{};

    for (int i = -halfCellCount; i <= halfCellCount; i++) {
        std::uint32_t currentIndex = static_cast<std::uint32_t>(positionVertices.size());
        positionVertices.push_back(Vec3f{-halfCellCount * spacingMeters, 0, i * spacingMeters});
        positionVertices.push_back(Vec3f{halfCellCount * spacingMeters, 0, i * spacingMeters});
        positionVertices.push_back(Vec3f{i * spacingMeters, 0, -halfCellCount * spacingMeters});
        positionVertices.push_back(Vec3f{i * spacingMeters, 0, halfCellCount * spacingMeters});
        grid.indices.push_back(currentIndex);
        grid.indices.push_back(currentIndex + 1);
        grid.indices.push_back(currentIndex + 2);
        grid.indices.push_back(currentIndex + 3);
    }

    std::vector<engine::graphics::VertexAttributeDescription> descs = {
        engine::graphics::VertexAttributeDescription{.location = 0, .format = engine::graphics::VertexAttributeFormat::Float3, .offset = 0},
    };

    grid.vertices = engine::graphics::makeVerticesData<Vec3f>(positionVertices, descs);
    return grid;
}

} // namespace

EditorGrid::EditorGrid(std::unique_ptr<engine::graphics::Mesh>& mesh, std::unique_ptr<engine::graphics::GraphicsPipeline>& pipeline) : _mesh(std::move(mesh)), _pipeline(std::move(pipeline)) {}

std::optional<EditorGrid> EditorGrid::create(SDL_GPUDevice* device, SDL_GPUTextureFormat colorTergetFormat) {
    // Load global grid
    auto grid = createGlobalGridMeshData(20, 1);
    auto globalGridMesh = engine::graphics::Mesh::load(device, grid);
    if (!globalGridMesh) {
        return std::nullopt;
    }

    // create shaders
    using namespace engine::graphics;
    ShaderLoadInfo vertexShaderInfo{};
    vertexShaderInfo.name = "default.vert";
    vertexShaderInfo.shaderStage = ShaderStage::Vertex;
    vertexShaderInfo.numUniformBuffers = 1;
    auto vertexShader = Shader::load(device, vertexShaderInfo);
    if (!vertexShader) {
        return std::nullopt;
    }
    ShaderLoadInfo gridFragmentShaderInfo{};
    gridFragmentShaderInfo.name = "grid.frag";
    gridFragmentShaderInfo.shaderStage = ShaderStage::Fragment;
    gridFragmentShaderInfo.numUniformBuffers = 0;
    auto gridFragmentShader = engine::graphics::Shader::load(device, gridFragmentShaderInfo);
    if (!gridFragmentShader) {
        return std::nullopt;
    }

    // Get color target format
    SDL_GPUColorTargetDescription colorTargetDescription{};
    colorTargetDescription.format = colorTergetFormat;
    colorTargetDescription.blend_state = {};

    // Create graphics pipeline
    auto attributes = engine::graphics::convertVertexAttributeToSDL(grid.vertices.descs);
    SDL_GPUVertexBufferDescription vertexBufferDescription{};
    vertexBufferDescription.slot = 0;
    vertexBufferDescription.pitch = grid.vertices.pitch;
    vertexBufferDescription.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vertexBufferDescription.instance_step_rate = 0;
    SDL_GPUGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{};
    graphicsPipelineCreateInfo.vertex_shader = vertexShader->handle();
    graphicsPipelineCreateInfo.fragment_shader = gridFragmentShader.value().handle();
    graphicsPipelineCreateInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_LINELIST;
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
    graphicsPipelineCreateInfo.depth_stencil_state.enable_depth_write = false;
    graphicsPipelineCreateInfo.depth_stencil_state.enable_stencil_test = false;
    graphicsPipelineCreateInfo.target_info.color_target_descriptions = &colorTargetDescription;
    graphicsPipelineCreateInfo.target_info.num_color_targets = 1;
    graphicsPipelineCreateInfo.target_info.has_depth_stencil_target = true;
    graphicsPipelineCreateInfo.target_info.depth_stencil_format = engine::graphics::DepthTarget::format;
    graphicsPipelineCreateInfo.props = 0;

    auto gridGraphicsPipeline = GraphicsPipeline::create(device, graphicsPipelineCreateInfo);
    if (!gridGraphicsPipeline) {
        return std::nullopt;
    }

    auto pipeline = std::make_unique<GraphicsPipeline>(std::move(gridGraphicsPipeline.value()));
    auto mesh = std::make_unique<engine::graphics::Mesh>(std::move(globalGridMesh.value()));

    return EditorGrid(mesh, pipeline);
}

bool EditorGrid::recordRenderPass(SDL_GPUCommandBuffer* commandBuffer, SDL_GPUTexture* colorTarget, SDL_GPUTexture* depthTarget, Uint32 width, Uint32 height, const std::optional<engine::RenderCameraData>& camera) {
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

    if (!camera) {
        return true;
    }

    if (!depthTarget) {
        return false;
    }

    // Create color target
    SDL_GPUColorTargetInfo colorTargetInfo{};
    colorTargetInfo.texture = colorTarget;
    colorTargetInfo.clear_color = SDL_FColor{0.00647, 0, 0.0858, 1};
    colorTargetInfo.load_op = SDL_GPU_LOADOP_LOAD;
    colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

    // Create depth stencil target
    SDL_GPUDepthStencilTargetInfo depthStencilTargetInfo{};
    depthStencilTargetInfo.texture = depthTarget;
    depthStencilTargetInfo.clear_depth = 1.0;
    depthStencilTargetInfo.load_op = SDL_GPU_LOADOP_LOAD;
    depthStencilTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
    depthStencilTargetInfo.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
    depthStencilTargetInfo.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;

    // Begin render pass
    SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, &depthStencilTargetInfo);
    if (!validate(renderPass, "Begin render pass error")) {
        return false;
    }

    // Bind pipeline
    SDL_BindGPUGraphicsPipeline(renderPass, _pipeline->handle());

    // Send uniform data
    TransformUniform transformUniform{};
    transformUniform.projection = math::perspective(camera->verticalFovRadians, static_cast<float>(width) / static_cast<float>(height), camera->nearPlane, camera->farPlane);
    transformUniform.view = camera->viewMatrix;
    transformUniform.model = math::identity();
    SDL_PushGPUVertexUniformData(commandBuffer, 0, &transformUniform, sizeof(transformUniform));

    // Bind vertex buffer
    SDL_GPUBufferBinding vertexBufferBinding{};
    vertexBufferBinding.buffer = _mesh->vertexBufferHandle();
    vertexBufferBinding.offset = 0;
    SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBufferBinding, 1);

    // Bind index buffer
    SDL_GPUBufferBinding indexBufferBinding{};
    indexBufferBinding.buffer = _mesh->indexBufferHandle();
    indexBufferBinding.offset = 0;
    SDL_BindGPUIndexBuffer(renderPass, &indexBufferBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

    // Draw
    SDL_DrawGPUIndexedPrimitives(renderPass, _mesh->indexCount(), 1, 0, 0, 0);

    // End render pass
    SDL_EndGPURenderPass(renderPass);
    return true;
}

} // namespace editor