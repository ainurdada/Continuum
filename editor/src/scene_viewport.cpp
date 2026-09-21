#include <scene_viewport.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/trigonometric.hpp>

#include <ecs_reflection/public/world_reflection_context.h>
#include <scene/public/hierarchy.h>
#include <scene/public/mesh_renderer.h>
#include <scene/public/scene_entity_id.h>

#include <editor_session.h>
#include <ui/draw_context.h>

#define GIZMO_INTERACTION_KEY "Gizmo"

namespace editor {

SceneViewport::SceneViewport(SDL_Window* window, SDL_GPUDevice* device, SDL_GPUTextureFormat format) : _window(window), _device(device), _format(format) {
    _editorCameraTransform.position.z = -10;
    _editorCameraTransform.position.y = 5;
    _editorCameraTransform.rotation.x = glm::radians(30.0f);
}

std::expected<std::optional<SceneViewportFrame>, std::string> SceneViewport::draw(EditorSession& session, engine::ecs_reflection::WorldReflectionContext& ctx) {
    std::string error{};
    ImGuiIO& io = ImGui::GetIO();
    bool gizmoIsActive = false;
    SceneViewportFrame frame{};
    frame.rfd.drawGlobalGrid = true;
    frame.rfd.camera.verticalFovRadians = _editorCamera.verticalFov;
    frame.rfd.camera.nearPlane = _editorCamera.nearPlane;
    frame.rfd.camera.farPlane = _editorCamera.farPlane;
    bool sceneViewRenderable = false;
    ImVec2 viewportSize{};
    ImVec2 rectMin{};
    ImVec2 rectSize{};
    if (ImGui::Begin(SCENE_VIEW_WINDOW_NAME, nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_MenuBar)) {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::RadioButton("Move (E)", _gizmoOperation == ImGuizmo::TRANSLATE)) {
                _gizmoOperation = ImGuizmo::TRANSLATE;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Rotate (R)", _gizmoOperation == ImGuizmo::ROTATE)) {
                _gizmoOperation = ImGuizmo::ROTATE;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Scale (T)", _gizmoOperation == ImGuizmo::SCALE)) {
                _gizmoOperation = ImGuizmo::SCALE;
            }
            ImGui::SameLine();
            ImGui::Checkbox("Snap", &_gizmoSnapped);
            ImGuizmo::MODE effectiveGizmoMode = _gizmoMode;
            if (_gizmoOperation == ImGuizmo::SCALE) {
                effectiveGizmoMode = ImGuizmo::LOCAL;
            }
            ImGui::BeginDisabled(_gizmoOperation == ImGuizmo::SCALE);
            ImGui::SameLine();
            if (ImGui::RadioButton("World", effectiveGizmoMode == ImGuizmo::WORLD)) {
                _gizmoMode = ImGuizmo::WORLD;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Local", effectiveGizmoMode == ImGuizmo::LOCAL)) {
                _gizmoMode = ImGuizmo::LOCAL;
            }
            ImGui::EndDisabled();
            ImGui::EndMenuBar();
        }
        viewportSize = ImGui::GetContentRegionAvail();
        if (viewportSize.x > 0 && viewportSize.y > 0 && io.DisplayFramebufferScale.x > 0 && io.DisplayFramebufferScale.y > 0) {
            float width = viewportSize.x * io.DisplayFramebufferScale.x;
            float height = viewportSize.y * io.DisplayFramebufferScale.y;
            frame.widthInt = static_cast<Uint32>(width);
            frame.heightInt = static_cast<Uint32>(height);
            if (frame.widthInt >= 1 && frame.heightInt >= 1) {
                if (!_sceneViewColorTarget.has_value()) {
                    auto newColorTarget = engine::graphics::ColorTarget::create(_device, frame.widthInt, frame.heightInt, _format);
                    if (newColorTarget.has_value()) {
                        _sceneViewColorTarget = std::move(newColorTarget);
                        sceneViewRenderable = true;
                    } else {
                        error = "failed to create color target";
                    }
                } else {
                    if (!_sceneViewColorTarget.value().resize(frame.widthInt, frame.heightInt)) {
                        error = "failed to resize color target";
                    } else {
                        sceneViewRenderable = true;
                    }
                }
            }
        }
        if (sceneViewRenderable && _sceneViewColorTarget.has_value()) {
            ImGui::Image(_sceneViewColorTarget.value().handle(), viewportSize);
            rectMin = ImGui::GetItemRectMin();
            rectSize = ImGui::GetItemRectSize();
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                if (SDL_SetWindowRelativeMouseMode(_window, true)) {
                    _isActiveMouseLook = true;
                } else {
                    SDL_Log("%s", SDL_GetError());
                }
            }
        }
        if (_isActiveMouseLook && !ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
            stopMouseLook();
        }
        if (_isActiveMouseLook) {
            _editorCameraTransform.rotation.y += io.MouseDelta.x * mouseSensitivity;
            _editorCameraTransform.rotation.x += io.MouseDelta.y * mouseSensitivity;
            _editorCameraTransform.rotation.x = glm::clamp(_editorCameraTransform.rotation.x, glm::radians(-89.0f), glm::radians(89.0f));
            Vec3f cameraMoveDirection{};
            if (ImGui::IsKeyDown(ImGuiKey_A)) {
                cameraMoveDirection.x -= 1.0;
            }
            if (ImGui::IsKeyDown(ImGuiKey_D)) {
                cameraMoveDirection.x += 1.0;
            }
            if (ImGui::IsKeyDown(ImGuiKey_W)) {
                cameraMoveDirection.z += 1.0;
            }
            if (ImGui::IsKeyDown(ImGuiKey_S)) {
                cameraMoveDirection.z -= 1.0;
            }
            float heightDelta = 0;
            if (ImGui::IsKeyDown(ImGuiKey_E)) {
                heightDelta += 1.0;
            }
            if (ImGui::IsKeyDown(ImGuiKey_Q)) {
                heightDelta -= 1.0;
            }
            Vec3f worldCameraMoveDirection = math::transformDirection(engine::scene::localMatrix(_editorCameraTransform), cameraMoveDirection);
            Vec3f desiredTranslation = worldCameraMoveDirection + math::worldUp * heightDelta;
            if (math::length(desiredTranslation) > 0) {
                desiredTranslation = math::normalize(desiredTranslation);
            }
            _editorCameraTransform.position += desiredTranslation * cameraMoveSpeed * io.DeltaTime;
        }
        frame.rfd.camera.viewMatrix = math::inverse(engine::scene::localMatrix(_editorCameraTransform));
    }

    auto& transformStash = session.documentMut().worldMut().getStash<engine::scene::Transform>();
    auto& meshRendererStash = session.documentMut().worldMut().getStash<engine::scene::MeshRenderer>();
    auto& sceneEntityIdStash = session.documentMut().worldMut().getStash<engine::scene::SceneEntityId>();

    auto renderableEntityQuery = session.documentMut().worldMut().query().with<engine::scene::MeshRenderer>().with<engine::scene::Transform>().build();

    if (sceneViewRenderable) {
        // gizmo
        if (session.selectedEntity() && transformStash.has(session.selectedEntity().value())) {
            auto worldMatrix = engine::scene::worldMatrix(session.document().world(), session.selectedEntity().value());
            if (worldMatrix.has_value()) {
                Mat4f gizmoMatrix = worldMatrix.value();

                auto projection = math::perspective(_editorCamera.verticalFov, static_cast<float>(frame.widthInt) / static_cast<float>(frame.heightInt), _editorCamera.nearPlane, _editorCamera.farPlane);

                ImGuizmo::SetDrawlist();
                ImGuizmo::SetRect(rectMin.x, rectMin.y, rectSize.x, rectSize.y);
                ImGuizmo::SetOrthographic(false);
                if (!_isActiveMouseLook && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !io.WantTextInput) {
                    if (ImGui::IsKeyPressed(ImGuiKey_E)) {
                        _gizmoOperation = ImGuizmo::TRANSLATE;
                    }
                    if (ImGui::IsKeyPressed(ImGuiKey_R)) {
                        _gizmoOperation = ImGuizmo::ROTATE;
                    }
                    if (ImGui::IsKeyPressed(ImGuiKey_T)) {
                        _gizmoOperation = ImGuizmo::SCALE;
                    }
                }
                float* snap = nullptr;
                if (_gizmoSnapped) {
                    switch (_gizmoOperation) {
                    case ImGuizmo::TRANSLATE:
                        snap = glm::value_ptr(translationSnap);
                        break;
                    case ImGuizmo::ROTATE:
                        snap = glm::value_ptr(rotationSnap);
                        break;
                    case ImGuizmo::SCALE:
                        snap = glm::value_ptr(scaleSnap);
                        break;

                    default:
                        break;
                    }
                }
                ImGuizmo::MODE effectiveGizmoMode = _gizmoMode;
                if (_gizmoOperation == ImGuizmo::SCALE) {
                    effectiveGizmoMode = ImGuizmo::LOCAL;
                }
                auto selectedSceneId = *sceneEntityIdStash.get(session.selectedEntity().value());
                bool gizmoManipulated = ImGuizmo::Manipulate(glm::value_ptr(frame.rfd.camera.viewMatrix), glm::value_ptr(projection), _gizmoOperation, effectiveGizmoMode, glm::value_ptr(gizmoMatrix), nullptr, snap);
                gizmoIsActive = ImGuizmo::IsUsing();
                if (gizmoManipulated) {
                    engine::ecs::Entity manipulatedEntity = session.selectedEntity().value();
                    auto newLocalMatrix = engine::scene::worldToLocal(session.document().world(), manipulatedEntity, gizmoMatrix);
                    if (newLocalMatrix) {
                        if (!ctx.visitComponentsMut(manipulatedEntity, [&session, selectedSceneId, &newLocalMatrix](engine::ecs::IStash& stash, std::optional<engine::reflection::ObjectView> view) {
                                if (!view) {
                                    return;
                                }
                                auto transform = view->tryAsMut<engine::scene::Transform>();
                                if (transform) {
                                    ui::DrawContext<engine::scene::Transform> editContext(session, ComponentEditTarget{.entityId = selectedSceneId, .componentType = view->type()->nativeTypeKey}, *view, *view, EditInteraction{GIZMO_INTERACTION_KEY, {}}, nullptr, false);

                                    if (!editContext.beginEdit()) {
                                        SDL_Log("Failed to begin transform editint in gizmo");
                                        return;
                                    }

                                    editContext.getValueMut() = engine::scene::fromMat4(newLocalMatrix.value());
                                    editContext.markChanged();
                                }
                            })) {
                            error += "Failed to visit entity components\n";
                        }
                    }
                }
            }
        }
        for (auto entity : renderableEntityQuery.view()) {
            auto worldMatrix = engine::scene::worldMatrix(session.document().world(), entity);
            if (!worldMatrix.has_value()) {
                SDL_Log("fail to get transform for entity(id: %u, generation: %u)", entity.index, entity.generation);
                continue;
            }
            auto meshRenderer = meshRendererStash.get(entity);
            frame.rfd.items.push_back(engine::RenderItem{.geometryId = meshRenderer->geometryId, .modelMatrix = worldMatrix.value()});
        }
    }
    ImGui::End();
    if (!gizmoIsActive) {
        if (!session.finishComponentEdit(GIZMO_INTERACTION_KEY, ctx)) {
            SDL_Log("Failed to finish gizmo interaction editing");
        }
    }
    if (!error.empty()) {
        return std::unexpected(error);
    }
    if (!sceneViewRenderable) {
        return {};
    }
    frame.texture = _sceneViewColorTarget->handle();
    return frame;
}

void SceneViewport::stopMouseLook() {
    if (SDL_SetWindowRelativeMouseMode(_window, false)) {
        _isActiveMouseLook = false;
    } else {
        SDL_Log("%s", SDL_GetError());
    }
}

} // namespace editor
