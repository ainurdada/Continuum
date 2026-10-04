#include <vertex_layout.h>

#include <unordered_map>

namespace engine::graphics {

namespace {
const std::unordered_map<VertexAttributeFormat, SDL_GPUVertexElementFormat> sdlFormats = {
    {VertexAttributeFormat::Float, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT},
    {VertexAttributeFormat::Float2, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2},
    {VertexAttributeFormat::Float3, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3},
    {VertexAttributeFormat::Float4, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4},
};
}

std::vector<SDL_GPUVertexAttribute> convertVertexAttributeToSDL(const std::vector<VertexAttributeDescription>& descs) {
    std::vector<SDL_GPUVertexAttribute> res{};

    for (auto& desc : descs) {
        SDL_GPUVertexAttribute atb;
        atb.location = desc.location;
        atb.offset = desc.offset;
        atb.buffer_slot = 0;
        atb.format = sdlFormats.at(desc.format);

        res.push_back(atb);
    }

    return res;
}

} // namespace engine::graphics