#include "shader.h"

#include <sdl_support/public/validation.h>
#include <graphics_platform.h>
#include <string>

namespace engine::graphics {

inline SDL_GPUShaderStage translateShaderStageToSDL(ShaderStage shaderStage) {
    switch (shaderStage) {
    case ShaderStage::Vertex:
        return SDL_GPU_SHADERSTAGE_VERTEX;
    case ShaderStage::Fragment:
        return SDL_GPU_SHADERSTAGE_FRAGMENT;
    default:
        return {};
    }
}

Shader::Shader(Shader&& other) noexcept {
    _device = other._device;
    _gpuShader = other._gpuShader;

    other._device = nullptr;
    other._gpuShader = nullptr;
}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    release();

    _device = other._device;
    _gpuShader = other._gpuShader;

    other._device = nullptr;
    other._gpuShader = nullptr;

    return *this;
}

Shader::~Shader() {
    release();
}

std::optional<Shader> Shader::load(SDL_GPUDevice* device, ShaderLoadInfo loadInfo) {
    // Get base path
    auto basePath = SDL_GetBasePath();
    if (!validate(basePath, "Get base path error:")) {
        return std::nullopt;
    }

    // Load File
    std::string fullPath = std::string(basePath) + "shaders/" + loadInfo.name + fileExtension;
    size_t fileSize{};
    void* file = SDL_LoadFile(fullPath.c_str(), &fileSize);
    std::string loadFileErrorMessage = "Load file error: " + fullPath;
    if (!validate(file, loadFileErrorMessage.c_str())) {
        return std::nullopt;
    }

    // GPU create info
    SDL_GPUShaderCreateInfo shaderCreateInfo{};
    shaderCreateInfo.code = static_cast<const Uint8*>(file);
    shaderCreateInfo.code_size = fileSize;
    shaderCreateInfo.entrypoint = entryPoint;
    shaderCreateInfo.format = shaderFormat;
    shaderCreateInfo.stage = translateShaderStageToSDL(loadInfo.shaderStage);
    shaderCreateInfo.num_samplers = 0;
    shaderCreateInfo.num_storage_textures = 0;
    shaderCreateInfo.num_storage_buffers = 0;
    shaderCreateInfo.num_uniform_buffers = loadInfo.numUniformBuffers;
    shaderCreateInfo.props = 0;

    // Create shader
    SDL_GPUShader* shader = SDL_CreateGPUShader(device, &shaderCreateInfo);
    std::string createShaderErrorMessage = "Create shader error: " + fullPath;
    validate(shader, createShaderErrorMessage.c_str());

    // free loaded bytes
    SDL_free(file);

    if (!shader) {
        return std::nullopt;
    }

    return Shader(device, shader);
}

SDL_GPUShader* Shader::handle() const noexcept {
    return _gpuShader;
}

Shader::Shader(SDL_GPUDevice* device, SDL_GPUShader* gpuShader) {
    _device = device;
    _gpuShader = gpuShader;
}

void Shader::release() {
    if (_device && _gpuShader) {
        SDL_ReleaseGPUShader(_device, _gpuShader);
        _gpuShader = nullptr;
    }
}

} // namespace engine::graphics
