#include <inspector_panel.h>

#include <set>
#include <string>

#include <SDL3/SDL_log.h>
#include <glm/trigonometric.hpp>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <ecs/ecs.h>
#include <ecs_reflection/public/component_binding_registry.h>
#include <ecs_reflection/public/world_reflection_context.h>
#include <scene/public/mesh_renderer.h>
#include <scene/public/name.h>
#include <scene/public/parent.h>
#include <scene/public/scene_entity_id.h>

#include <editor_session.h>
#include <ui/component_drawer_registry.h>
#include <ui/modifierHelpers.h>

#define INSPECTOR_INTERACTION_KEY "Inspector"

namespace editor {

namespace {
bool drawValue(ui::DrawRequest& data, const ui::ComponentDrawerRegistry& drawers);

bool drawValueContent(ui::DrawRequest& data, const ui::ComponentDrawerRegistry& drawers) {
    auto typeDrawer = drawers.find(data.view.type()->nativeTypeKey);
    if (typeDrawer) {
        typeDrawer->drawUntyped(data);
        return data.session.isComponentEditActive(data.target, data.interactionKey);
    }
    if (data.view.type()->category != engine::reflection::TypeCategory::Object) {
        SDL_Log("Not supported drawer for %s", std::string(data.view.type()->name).c_str());
        return false;
    }
    bool drawSuccess = false;
    for (auto& field : data.view.type()->fields) {
        if (!field.modifiers.has<engine::reflection::modifiers::ShowInInspector>()) {
            continue;
        }

        std::optional<engine::reflection::ObjectView> fieldView;
        if (data.effectiveReadOnly || !data.view.canWrite() || field.modifiers.has<engine::reflection::modifiers::ReadOnly>()) {
            fieldView = data.view.readField(field);
        } else {
            fieldView = data.view.editField(field);
            if (!fieldView) {
                fieldView = data.view.readField(field);
            }
        }
        if (!fieldView) {
            continue;
        }

        ui::DrawRequest fieldData{.session = data.session, .target = data.target, .rootView = data.rootView, .view = fieldView.value(), .interactionKey = data.interactionKey, .fieldDesc = &field, .effectiveReadOnly = data.effectiveReadOnly};
        fieldData.interactionKey.fieldPath.emplace_back(field.key);

        ImGui::PushID(std::string(field.key).c_str());
        drawSuccess |= drawValue(fieldData, drawers);
        ImGui::PopID();
    }
    return drawSuccess;
}

bool drawValue(ui::DrawRequest& data, const ui::ComponentDrawerRegistry& drawers) {
    if (data.view.type()->category == engine::reflection::TypeCategory::Object) {
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImGui::GetStyle().WindowPadding);
        if (ImGui::BeginTable("ObjectSection", 1, ImGuiTableFlags_BordersOuter, ImVec2(0, 0))) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImGuiCol_ChildBg));

            std::string name = getDisplayName(*data.view.type(), data.fieldDesc);
            bool successDraw = false;
            if (ImGui::CollapsingHeader((name + "###ObjectHeader").c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                successDraw = drawValueContent(data, drawers);
            }

            ImGui::EndTable();
            ImGui::PopStyleVar();
            ImGui::Spacing();
            return successDraw;
        }
        ImGui::PopStyleVar();
        ImGui::Spacing();
        return false;
    }
    return drawValueContent(data, drawers);
}

std::expected<void, std::string> createComponent(engine::ecs::Entity entity, EditorSession& session, const engine::ecs_reflection::ComponentBinding& binding, engine::ecs_reflection::WorldReflectionContext& ctx) {
    auto& world = session.documentMut().worldMut();

    if (!world.hasEntity(entity)) {
        return std::unexpected("Not valid entity");
    }

    if (!binding.create) {
        return std::unexpected("component does not have create function");
    }

    auto finishEdit = session.finishActiveComponentEdit(ctx);
    if (!finishEdit) {
        return std::unexpected(finishEdit.error());
    }

    auto createResult = binding.create(world, entity, *binding.type);
    if (!createResult) {
        return std::unexpected(createResult.error());
    }

    session.clearHistory();

    session.documentMut().markDirty();

    return {};
}

std::expected<void, std::string> removeComponent(engine::ecs::Entity entity, EditorSession& session, const engine::ecs_reflection::ComponentBinding& binding, engine::ecs_reflection::WorldReflectionContext& ctx) {
    auto& world = session.documentMut().worldMut();

    if (!world.hasEntity(entity)) {
        return std::unexpected("Not valid entity");
    }

    if (!binding.remove) {
        return std::unexpected("component does not have remove function");
    }

    auto finishEdit = session.finishActiveComponentEdit(ctx);
    if (!finishEdit) {
        return std::unexpected(finishEdit.error());
    }

    auto removeResult = binding.remove(world, entity, *binding.type);
    if (!removeResult) {
        return std::unexpected(removeResult.error());
    }

    session.clearHistory();

    session.documentMut().markDirty();

    return {};
}
} // namespace

void InspectorPanel::draw(InspectorDrawData& data) {
    auto& session = data.session;
    auto& ctx = data.ctx;
    auto& drawers = data.drawers;
    auto& bindings = data.bindings;

    auto& parentStash = session.documentMut().worldMut().getStash<engine::scene::Parent>();
    auto& transformStash = session.documentMut().worldMut().getStash<engine::scene::Transform>();
    auto& meshRendererStash = session.documentMut().worldMut().getStash<engine::scene::MeshRenderer>();
    auto& sceneEntityIdStash = session.documentMut().worldMut().getStash<engine::scene::SceneEntityId>();

    auto parentQuery = session.documentMut().worldMut().query().with<engine::scene::Parent>().build();

    bool activeEditDrawn = false;
    if (ImGui::Begin(INSPECTOR_WINDOW_NAME)) {
        if (session.selectedEntity().has_value()) {
            engine::ecs::Entity entity = session.selectedEntity().value();
            engine::scene::SceneEntityId sceneId = *sceneEntityIdStash.get(entity);

            std::set<std::type_index> entityComponents{};
            std::optional<engine::ecs::IStash*> stashToRemove = std::nullopt;
            ctx.visitComponentsMut(entity, [&session, sceneId, &drawers, &activeEditDrawn, &entityComponents, &stashToRemove](engine::ecs::IStash& stash, std::optional<engine::reflection::ObjectView> view) {
                if (!view) {
                    return;
                }

                ui::DrawRequest data{
                    .session = session,
                    .target = ComponentEditTarget{.entityId = sceneId, .componentType = view->type()->nativeTypeKey},
                    .rootView = view.value(),
                    .view = view.value(),
                    .interactionKey = EditInteraction{INSPECTOR_INTERACTION_KEY, {}},
                    .effectiveReadOnly = false,
                };
                std::string id = std::to_string(sceneId.value) + "::" + std::string(view->type()->key);
                ImGui::PushID(id.c_str());
                activeEditDrawn |= drawValue(data, drawers);
                if (ImGui::Button("remove")) {
                    stashToRemove = &stash;
                }
                ImGui::PopID();

                entityComponents.emplace(stash.nativeTypeKey());
            });

            // remove component
            if (stashToRemove) {
                auto binding = bindings.findBinding(stashToRemove.value()->nativeTypeKey());
                if (binding) {
                    auto removeResult = removeComponent(entity, session, *binding, ctx);
                    if (!removeResult) {
                        SDL_LogError(SDL_LogCategory::SDL_LOG_CATEGORY_ERROR, "%s", removeResult.error().c_str());
                    }
                } else {
                    SDL_LogError(SDL_LogCategory::SDL_LOG_CATEGORY_ERROR, "Failed to find binding");
                }
            }

            // add component button
            if (ImGui::Button("add component")) {
                ImGui::OpenPopup("Available Component");
            }
            bool hasComponentsToAdd = false;
            if (ImGui::BeginPopup("Available Component")) {
                bindings.visitRegisteredComponents([&entityComponents, &hasComponentsToAdd, &entity, &session, &ctx](const engine::ecs_reflection::ComponentBinding& binding) {
                    if (binding.create && !entityComponents.contains(binding.nativeTypeIndex)) {
                        std::string label = getDisplayName(*binding.type) + "###component_" + std::string(binding.type->key);
                        if (ImGui::Button(label.c_str())) {
                            auto createResult = createComponent(entity, session, binding, ctx);
                            if (!createResult) {
                                SDL_LogError(SDL_LogCategory::SDL_LOG_CATEGORY_ERROR, "%s", createResult.error().c_str());
                            } else {
                                ImGui::CloseCurrentPopup();
                            }
                        }
                        hasComponentsToAdd = true;
                    }
                });

                if (!hasComponentsToAdd) {
                    ImGui::TextUnformatted("No components to add");
                }

                ImGui::EndPopup();
            }

            // destroy entity
            bool hasChildren = false;
            for (auto childrenEntity : parentQuery.view()) {
                auto parent = parentStash.get(childrenEntity);
                if (parent->entity == entity) {
                    hasChildren = true;
                    break;
                }
            }
            bool wantToDeleteEntity = ImGui::Button("Delete entity");
            if (hasChildren) {
                ImGui::SameLine();
                ImGui::Checkbox("Delete children", &deleteChildrenMode);
            }
            if (wantToDeleteEntity) {
                auto deleteResult = session.documentMut().deleteEntity(entity, deleteChildrenMode);
                if (deleteResult) {
                    session.clearHistory();
                    session.clearSelection(ctx);
                } else {
                    SDL_Log("%s", deleteResult.error().c_str());
                }
            }

            // unsaved changes text
            if (session.documentMut().isDirty()) {
                ImGui::TextDisabled("Unsaved changes");
            }
        } else {
            ImGui::TextUnformatted("No entity selected");
        }
    }
    ImGui::End();
    if (!activeEditDrawn) {
        auto finish = session.finishComponentEdit(INSPECTOR_INTERACTION_KEY, ctx);
        if (!finish) {
            SDL_Log("%s", finish.error().c_str());
        }
    }
}

} // namespace editor
