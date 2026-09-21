#include <hierarchy_panel.h>

#include <SDL3/SDL_log.h>
#include <imgui.h>

#include <editor_session.h>
#include <ecs/ecs.h>
#include <scene/public/name.h>
#include <scene/public/parent.h>
#include <scene/public/scene_entity_id.h>

namespace editor {

namespace {

void showOriginEntityNode(engine::ecs::Entity originEntity, const engine::ecs::Stash<engine::scene::Name>& nameStash, const engine::ecs::Stash<engine::scene::Parent>& parentStash, const engine::ecs::Stash<engine::scene::SceneEntityId>& sceneEntityIdStash, const engine::ecs::Query& sceneEntityQuery,
                          editor::EditorSession& session, engine::ecs_reflection::WorldReflectionContext& ctx) {
    std::string label = nameStash.get(originEntity)->value + "###entity_" + std::to_string(sceneEntityIdStash.get(originEntity)->value);
    std::vector<engine::ecs::Entity> children{};
    for (auto sceneEntity : sceneEntityQuery.view()) {
        if (parentStash.has(sceneEntity)) {
            auto parent = parentStash.get(sceneEntity);
            if (parent->entity == originEntity) {
                children.push_back(sceneEntity);
            }
        }
    }
    ImGuiTreeNodeFlags treeFlags = ImGuiTreeNodeFlags_OpenOnArrow;
    if (session.selectedEntity().has_value() && session.selectedEntity().value() == originEntity) {
        treeFlags |= ImGuiTreeNodeFlags_Selected;
    }
    if (children.size() == 0) {
        treeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }
    bool nodeOpen = ImGui::TreeNodeEx(label.c_str(), treeFlags);
    if (ImGui::IsItemClicked()) {
        session.select(originEntity, ctx);
    }
    if (nodeOpen) {
        for (auto child : children) {
            showOriginEntityNode(child, nameStash, parentStash, sceneEntityIdStash, sceneEntityQuery, session, ctx);
        }
        if (children.size() != 0) {
            ImGui::TreePop();
        }
    }
}

} // namespace

void HierarchyPanel::draw(EditorSession& session, engine::ecs_reflection::WorldReflectionContext& ctx) {
    auto& nameStash = session.documentMut().worldMut().getStash<engine::scene::Name>();
    auto& parentStash = session.documentMut().worldMut().getStash<engine::scene::Parent>();
    auto& sceneEntityIdStash = session.documentMut().worldMut().getStash<engine::scene::SceneEntityId>();

    auto sceneEntityQuery = session.documentMut().worldMut().query().with<engine::scene::SceneEntityId>().with<engine::scene::Name>().build();
    auto originEntityQuery = session.documentMut().worldMut().query().with<engine::scene::SceneEntityId>().with<engine::scene::Name>().without<engine::scene::Parent>().build();

    if (ImGui::Begin(HIERARCHY_WINDOW_NAME)) {
        if (ImGui::Button("Create Empty")) {
            auto newEntity = session.documentMut().createEmptyEntity();
            if (!newEntity.has_value()) {
                SDL_Log("%s", newEntity.error().c_str());
            } else {
                session.select(newEntity.value(), ctx);
            }
        }
        for (auto entity : originEntityQuery.view()) {
            showOriginEntityNode(entity, nameStash, parentStash, sceneEntityIdStash, sceneEntityQuery, session, ctx);
        }
    }
    ImGui::End();
}

} // namespace editor
