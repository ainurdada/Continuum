#include <editor_application.h>

#include <iostream>

#include <SDL3/SDL.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlgpu3.h>
#include <imgui_internal.h>

#include <ecs_reflection/public/ecs_reflection_bootstrap.h>
#include <ecs_reflection/public/world_reflection_context.h>
#include <edit_snapshot_policy.h>
#include <reflection/public/bootstrap.h>
#include <reflection_json/public/builtin_json_policies.h>
#include <reflection_json/public/scene_serializer_json.h>
#include <render_sdl/public/graphics_context.h>
#include <render_sdl/public/renderer.h>
#include <scene/public/camera.h>
#include <scene/public/name.h>
#include <scene/public/transform.h>

#include <console_log.h>
#include <scene_viewport.h>
#include <snapshot_bootstrap.h>
#include <styles.h>
#include <ui/drawers/bool_drawer.h>
#include <ui/drawers/double_drawer.h>
#include <ui/drawers/float_drawer.h>
#include <ui/drawers/int_drawer.h>
#include <ui/drawers/string_drawer.h>
#include <ui/drawers/transform_drawer.h>

namespace editor {

EditorApplication::EditorApplication(Project project) : _project(std::move(project)), _assetRegistry(_project.projectRoot), _contentBrowser(_project.projectRoot) {}

bool EditorApplication::trySaveScene(engine::ecs_reflection::WorldReflectionContext& ctx) {
    auto finishEditResult = _session->finishActiveComponentEdit(ctx);
    if (!finishEditResult) {
        SDL_Log("%s", finishEditResult.error().c_str());
        return false;
    }

    engine::scene::serialization::json::SceneSerializerJson serializer(_typeRegistry, _jsonSerializationRegistry, _componentBindings);

    auto saveResult = _session->documentMut().save(serializer);
    if (saveResult) {
        return true;
    } else {
        SDL_Log("%s", saveResult.error().c_str());
        return false;
    }
}

void EditorApplication::drawMainMenuBar(engine::ecs_reflection::WorldReflectionContext& ctx) {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            ImGui::BeginDisabled(_playSession.get());
            if (ImGui::MenuItem("Save scene", nullptr, false, _session->document().isDirty())) {
                trySaveScene(ctx);
            }

            ImGui::EndDisabled();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            ImGui::BeginDisabled(_playSession.get());
            if (ImGui::MenuItem("Undo", "Ctrl/Cmd+Z", false, _session->canUndo())) {
                _session->undo(ctx);
            }
            if (ImGui::MenuItem("Redo", "Ctrl/Cmd+Shift+Z", false, _session->canRedo())) {
                _session->redo(ctx);
            }

            ImGui::EndDisabled();
            ImGui::EndMenu();
        }
        ImGui::BeginDisabled(_playSession.get());
        ImGui::SameLine();
        if (ImGui::Button("Play")) {
            auto preparePlaySession = [this, &ctx]() -> std::unique_ptr<PlaySession> {
                auto finishEdit = _session->finishActiveComponentEdit(ctx);
                if (!finishEdit) {
                    SDL_LogError(SDL_LogCategory::SDL_LOG_CATEGORY_ERROR, "%s", finishEdit.error().c_str());
                    return nullptr;
                }
                std::unique_ptr<PlaySession> play = std::make_unique<PlaySession>();
                engine::scene::serialization::json::SceneSerializerJson serializer{_typeRegistry, _jsonSerializationRegistry, _componentBindings};
                auto worldJson = serializer.worldToJson(_session->documentMut().worldMut());
                if (!worldJson) {
                    SDL_LogError(SDL_LogCategory::SDL_LOG_CATEGORY_ERROR, "%s", worldJson.error().c_str());
                    return nullptr;
                }

                auto runResult = play->run(worldJson.value(), _typeRegistry, _jsonSerializationRegistry, _componentBindings);
                if (!runResult) {
                    SDL_LogError(SDL_LogCategory::SDL_LOG_CATEGORY_ERROR, "%s", runResult.error().c_str());
                    return nullptr;
                }

                return std::move(play);
            };

            auto play = preparePlaySession();
            if (play) {
                _playSession = std::move(play);
            }
        }
        ImGui::EndDisabled();
        ImGui::BeginDisabled(!_playSession.get());
        ImGui::SameLine();
        if (ImGui::Button("Stop")) {
            if (_playSession) {
                _playSession.reset();
            }
        }
        ImGui::EndDisabled();

        ImGui::EndMainMenuBar();
    }
}

void EditorApplication::buildDefaultDockLayout(ImGuiID dockspaceId) {
    const auto viewport = ImGui::GetMainViewport();

    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->WorkSize);

    ImGuiID leftId = 0;
    ImGuiID remainderId = 0;
    ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Left, 0.25f, &leftId, &remainderId);

    ImGuiID rightId = 0;
    ImGuiID centerId = 0;
    ImGui::DockBuilderSplitNode(remainderId, ImGuiDir_Right, 0.3f, &rightId, &centerId);

    ImGuiID centerDownId = 0;
    ImGuiID centerUpId = 0;
    ImGui::DockBuilderSplitNode(centerId, ImGuiDir_Down, 0.33f, &centerDownId, &centerUpId);

    ImGui::DockBuilderDockWindow(HIERARCHY_WINDOW_NAME, leftId);
    ImGui::DockBuilderDockWindow(INSPECTOR_WINDOW_NAME, rightId);
    ImGui::DockBuilderDockWindow(CONTENT_BROWSER_WINDOW_NAME, centerDownId);
    ImGui::DockBuilderDockWindow(CONSOLE_WINDOW_NAME, centerDownId);
    ImGui::DockBuilderDockWindow(SCENE_VIEW_WINDOW_NAME, centerUpId);
    ImGui::DockBuilderFinish(dockspaceId);
}

int EditorApplication::run() {
    if (!engine::reflection::serialization::registerBuiltinJsonPolicies(_jsonSerializationRegistry)) {
        std::cerr << "Failed to register builtin json policy" << std::endl;
        return 1;
    }

    if (!engine::reflection::generated::initializeTypeRegistry(_typeRegistry)) {
        std::cerr << "Failed to initialize type registry" << std::endl;
        return 1;
    }

    if (!engine::ecs_reflection::generated::initializeComponentBindings(_componentBindings, _typeRegistry)) {
        std::cerr << "Failed to initialize component bindings" << std::endl;
        return 1;
    }

    if (!editor::generated::initializeSnapshotPolicies(_snapshotRegistry)) {
        std::cerr << "Failed to register edit policy" << std::endl;
        return 1;
    }

    if (!_componentDrawers.registerDrawer<editor::ui::TransformDrawer>()) {
        std::cerr << "Failed to register transform drawer";
        return 1;
    }

    if (!_componentDrawers.registerDrawer<editor::ui::FloatDrawer>()) {
        std::cerr << "Failed to register float drawer";
        return 1;
    }

    if (!_componentDrawers.registerDrawer<editor::ui::BoolDrawer>()) {
        std::cerr << "Failed to register bool drawer";
        return 1;
    }

    if (!_componentDrawers.registerDrawer<editor::ui::IntDrawer>()) {
        std::cerr << "Failed to register int drawer";
        return 1;
    }

    if (!_componentDrawers.registerDrawer<editor::ui::DoubleDrawer>()) {
        std::cerr << "Failed to register double drawer";
        return 1;
    }

    if (!_componentDrawers.registerDrawer<editor::ui::StringDrawer>()) {
        std::cerr << "Failed to register string drawer";
        return 1;
    }

    engine::scene::serialization::json::SceneSerializerJson serializer(_typeRegistry, _jsonSerializationRegistry, _componentBindings);

    // load scene and create session
    auto openSceneDocumentResult = editor::SceneDocument::open(_project.projectRoot / "scenes" / "main.cscn", serializer);
    if (!openSceneDocumentResult.has_value()) {
        std::cerr << "error: " << openSceneDocumentResult.error() << std::endl;
        return 1;
    }

    _session.emplace(std::move(openSceneDocumentResult.value()), _snapshotRegistry);

    auto worldfReflectionContext = engine::ecs_reflection::WorldReflectionContext(_session->documentMut().worldMut(), _componentBindings);

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("%s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(_project.name.c_str(), 1280, 800, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!window) {
        SDL_Log("%s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    int returnCode = 0;
    {
        // init console log
        editor::ConsoleLog log{};

        // scan project assets
        auto assetScanResult = _assetRegistry.registerProjectFiles();
        if (assetScanResult) {
            SDL_Log("Assets was loaded");
        } else {
            SDL_LogError(SDL_LogCategory::SDL_LOG_CATEGORY_ERROR, "Failed to scan assets: %s", assetScanResult.error().c_str());
        }

        auto graphicsContext = engine::graphics::GraphicsContext::create(window);
        if (graphicsContext) {

            IMGUI_CHECKVERSION();
            ImGuiContext* imguiCtx = ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
            io.IniFilename = nullptr;
            ImGui::StyleColorsDark();

            editor::styles::setDarkPastelImGuiStyle();

            if (ImGui_ImplSDL3_InitForSDLGPU(window)) {
                ImGui_ImplSDLGPU3_InitInfo initInfo{};
                initInfo.Device = graphicsContext->device();
                initInfo.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(graphicsContext->device(), window);

                if (ImGui_ImplSDLGPU3_Init(&initInfo)) {
                    bool dockLayoutInitialized = false;
                    bool running = true;

                    // Viewport
                    editor::SceneViewport viewport{window, graphicsContext->device(), initInfo.ColorTargetFormat};

                    // Scene variables
                    auto renderer = engine::graphics::Renderer::create(window, &graphicsContext.value());
                    if (!renderer) {
                        returnCode = 1;
                        running = false;
                    }

                    Uint64 previousFrameTime = SDL_GetTicksNS();
                    while (running) {
                        SDL_Event event{};
                        bool closeRequested = false;

                        while (SDL_PollEvent(&event)) {
                            ImGui_ImplSDL3_ProcessEvent(&event);

                            if (event.type == SDL_EVENT_QUIT || (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))) {
                                if (_session->document().isDirty()) {
                                    closeRequested = true;
                                } else {
                                    running = false;
                                }
                            }
                            if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST) {
                                auto finishEditresult = _session->finishActiveComponentEdit(worldfReflectionContext);
                                viewport.stopMouseLook();
                                if (!finishEditresult) {
                                    SDL_Log("%s", finishEditresult.error().c_str());
                                }
                            }
                        }

                        if (!running) {
                            break;
                        }

                        // Update time
                        const Uint64 currentTime = SDL_GetTicksNS();
                        float deltaSeconds = static_cast<float>(currentTime - previousFrameTime) / 1'000'000'000.0;
                        previousFrameTime = currentTime;

                        ImGui_ImplSDLGPU3_NewFrame();
                        ImGui_ImplSDL3_NewFrame();
                        ImGui::NewFrame();
                        ImGuizmo::BeginFrame();

                        if (closeRequested) {
                            ImGui::OpenPopup("Unsaved changes");
                        }
                        if (ImGui::BeginPopupModal("Unsaved changes")) {
                            ImGui::TextUnformatted("You have unsaved changes");
                            if (ImGui::Button("Save")) {
                                if (trySaveScene(worldfReflectionContext)) {
                                    running = false;
                                }
                            }
                            if (ImGui::Button("Discard")) {
                                running = false;
                                ImGui::CloseCurrentPopup();
                            }
                            if (ImGui::Button("Cancel")) {
                                ImGui::CloseCurrentPopup();
                            }
                            ImGui::EndPopup();
                        }

                        // Main menu bar
                        drawMainMenuBar(worldfReflectionContext);

                        ImGuiID dockspaceId = ImGui::DockSpaceOverViewport();
                        if (!dockLayoutInitialized) {
                            buildDefaultDockLayout(dockspaceId);
                            dockLayoutInitialized = true;
                        }

                        ImGui::BeginDisabled(_playSession.get());
                        // Hierarchy
                        _hierarchy.draw(*_session, worldfReflectionContext);

                        // Inspector
                        _inspector.draw(*_session, worldfReflectionContext, _componentDrawers);

                        // Content Browser
                        ContentBrowserDrawInfo contentBrowserDrawInfo {
                            .project = _project,
                            .assetRegistry = _assetRegistry,
                        };
                        _contentBrowser.draw(contentBrowserDrawInfo);
                        ImGui::EndDisabled();

                        // Console
                        _console.draw(log);

                        // Scene View
                        std::optional<editor::SceneViewportFrame> viewportFrame = std::nullopt;
                        engine::RenderFrameData frame{};
                        if (_playSession.get()) {
                            _playSession->update(deltaSeconds);
                            _playSession->renderScene(frame);
                        }
                        auto drawViewportResult = viewport.draw(*_session, worldfReflectionContext, frame, _playSession.get());
                        if (!drawViewportResult) {
                            SDL_Log("%s", drawViewportResult.error().c_str());
                            returnCode = 1;
                            running = false;
                        } else {
                            viewportFrame = drawViewportResult.value();
                        }

                        // editor
                        if (!_playSession.get() && ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Z, ImGuiInputFlags_RouteGlobal)) {
                            _session->undo(worldfReflectionContext);
                        }
                        if (!_playSession.get() && (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z, ImGuiInputFlags_RouteGlobal) || ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Y, ImGuiInputFlags_RouteGlobal))) {
                            _session->redo(worldfReflectionContext);
                        }

                        ImGui::Render();

                        ImDrawData* imDrawData = ImGui::GetDrawData();

                        auto commandBuffer = SDL_AcquireGPUCommandBuffer(graphicsContext->device());
                        if (!commandBuffer) {
                            SDL_Log("%s", SDL_GetError());
                            returnCode = 1;
                            running = false;
                            break;
                        }

                        SDL_GPUTexture* swapchainTexture{};
                        if (!SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, window, &swapchainTexture, nullptr, nullptr)) {
                            SDL_Log("%s", SDL_GetError());
                            SDL_CancelGPUCommandBuffer(commandBuffer);
                            returnCode = 1;
                            running = false;
                            break;
                        }

                        if (swapchainTexture) {
                            if (viewportFrame.has_value()) {
                                if (!renderer->recordRenderPass(commandBuffer, viewportFrame->texture, viewportFrame->widthInt, viewportFrame->heightInt, viewportFrame->rfd)) {
                                    SDL_CancelGPUCommandBuffer(commandBuffer);
                                    returnCode = 1;
                                    running = false;
                                    break;
                                }
                            }
                            ImGui_ImplSDLGPU3_PrepareDrawData(imDrawData, commandBuffer);

                            SDL_GPUColorTargetInfo colorTargetInfo{};
                            colorTargetInfo.texture = swapchainTexture;
                            colorTargetInfo.clear_color = SDL_FColor{0.09435f, 0.0f, 0.102234f, 1.0f};
                            colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
                            colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

                            SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, nullptr);
                            if (renderPass) {
                                ImGui_ImplSDLGPU3_RenderDrawData(imDrawData, commandBuffer, renderPass);

                                SDL_EndGPURenderPass(renderPass);
                            } else {
                                SDL_Log("%s", SDL_GetError());
                                returnCode = 1;
                                running = false;
                            }
                        }

                        if (!SDL_SubmitGPUCommandBuffer(commandBuffer)) {
                            SDL_Log("%s", SDL_GetError());
                            returnCode = 1;
                            running = false;
                            break;
                        }

                        SDL_Delay(16);
                    }

                    if (!SDL_WaitForGPUIdle(graphicsContext->device())) {
                        SDL_Log("%s", SDL_GetError());
                        returnCode = 1;
                    }
                    ImGui_ImplSDLGPU3_Shutdown();
                } else {
                    returnCode = 1;
                }

                ImGui_ImplSDL3_Shutdown();
            } else {
                returnCode = 1;
            }

            ImGui::DestroyContext(imguiCtx);
        } else {
            returnCode = 1;
        }
    }

    SDL_DestroyWindow(window);

    SDL_Quit();
    return returnCode;
}

} // namespace editor
