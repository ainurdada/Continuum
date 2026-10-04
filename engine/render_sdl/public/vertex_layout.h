#pragma once

#include <vector>

#include <SDL3/SDL_gpu.h>
#include <render/public/mesh_data.h>

namespace engine::graphics {

std::vector<SDL_GPUVertexAttribute> convertVertexAttributeToSDL(const std::vector<VertexAttributeDescription>& descs);

}