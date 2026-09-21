#include <inspector_panel.h>

#include <string>

#include <SDL3/SDL_log.h>
#include <glm/trigonometric.hpp>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <ecs/ecs.h>
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

void InspectorPanel::draw(EditorSession& session, engine::ecs_reflection::WorldReflectionContext& ctx, const ui::ComponentDrawerRegistry& drawers) {
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

            ctx.visitComponentsMut(entity, [&session, sceneId, &drawers, &activeEditDrawn](engine::ecs::IStash& stash, std::optional<engine::reflection::ObjectView> view) {
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
                ImGui::PopID();
            });

            // add mesh renderer button
            if (transformStash.has(entity) && !meshRendererStash.has(entity)) {
                if (ImGui::Button("Add MeshRenderer")) {
                    engine::scene::MeshRenderer newMeshRenderer{};
                    newMeshRenderer.geometryId = engine::GeometryId::Cube;
                    meshRendererStash.add(entity, newMeshRenderer);
                    if (meshRendererStash.has(entity)) {
                        session.documentMut().markDirty();
                    } else {
                        SDL_Log("failed to add mesh renderer");
                    }
                }
            } else if (meshRendererStash.has(entity)) {
                if (ImGui::Button("Remove MeshRenderer")) {
                    meshRendererStash.remove(entity);
                    if (!meshRendererStash.has(entity)) {
                        session.documentMut().markDirty();
                    } else {
                        SDL_Log("failed to remove mesh renderer");
                    }
                }
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
