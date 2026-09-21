#pragma once

#include <SDL3/SDL_gpu.h>

namespace engine::graphics {

#ifdef _WIN32
constexpr char const* fileExtension = ".dxil";
constexpr char const* entryPoint = "main";
constexpr SDL_GPUShaderFormat shaderFormat = SDL_GPU_SHADERFORMAT_DXIL;
#elifdef __APPLE__
constexpr char const* fileExtension = ".msl";
constexpr char const* entryPoint = "main0";
constexpr SDL_GPUShaderFormat shaderFormat = SDL_GPU_SHADERFORMAT_MSL;
#else
#error
#endif

} // namespace engine::graphics