#pragma once

#include <SDL3/SDL.h>
#include <memory>
#include <optional>

namespace engine::graphics {

enum class ShaderStage {
    Vertex,
    Fragment
};

struct ShaderLoadInfo {
    const char* name = "";
    ShaderStage shaderStage{};
    Uint32 numUniformBuffers{};
};

class Shader {
  private:
    SDL_GPUDevice* _device{};
    SDL_GPUShader* _gpuShader{};

  public:
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    Shader() = delete;
    ~Shader();

    static std::optional<Shader> load(SDL_GPUDevice* device, ShaderLoadInfo loadInfo);

    SDL_GPUShader* handle() const noexcept;

  private:
    Shader(SDL_GPUDevice* device, SDL_GPUShader* gpuShader);

    void release();
};

} // namespace engine::graphics
