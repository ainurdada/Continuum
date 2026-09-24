#pragma once

#include <expected>
#include <optional>
#include <string>

#include <SDL3/SDL.h>
#include <imgui.h>

#include <ImGuizmo.h>

#include <render_sdl/public/color_target.h>
#include <render/public/render_frame_data.h>
#include <scene/public/camera.h>
#include <scene/public/transform.h>


#define SCENE_VIEW_WINDOW_NAME "Scene"

namespace engine::ecs_reflection {

class WorldReflectionContext;

} // namespace engine::ecs_reflection

namespace editor {

class EditorSession;

struct SceneViewportFrame {
    engine::RenderFrameData rfd{};
    SDL_GPUTexture* texture;
    Uint32 widthInt{};
    Uint32 heightInt{};
};

class SceneViewport {
  private:
    SDL_Window* _window;
    SDL_GPUDevice* _device;
    SDL_GPUTextureFormat _format;

    engine::scene::Camera _editorCamera{};
    engine::scene::Transform _editorCameraTransform{};
    bool _isActiveMouseLook = false;
    bool _gizmoSnapped = true;
    ImGuizmo::OPERATION _gizmoOperation = ImGuizmo::TRANSLATE;
    ImGuizmo::MODE _gizmoMode = ImGuizmo::WORLD;
    std::optional<engine::graphics::ColorTarget> _sceneViewColorTarget = std::nullopt;

  public:
    // radians per one mouse motion unit
    float mouseSensitivity = 0.005f;
    // m/s
    float cameraMoveSpeed = 4.0f;
    Vec3f translationSnap = Vec3f{0.5f};
    Vec3f rotationSnap = Vec3f{1.0f};
    Vec3f scaleSnap = Vec3f{0.1f};

    SceneViewport(SDL_Window* window, SDL_GPUDevice* device, SDL_GPUTextureFormat format);

    std::expected<std::optional<SceneViewportFrame>, std::string> draw(EditorSession& session, engine::ecs_reflection::WorldReflectionContext& ctx, engine::RenderFrameData& rfd, bool play);

    void stopMouseLook();
};

} // namespace editor
