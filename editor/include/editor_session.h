#pragma once

#include <memory>
#include <optional>
#include <string_view>

#include <ecs/ecs.h>
#include <ecs_reflection/public/world_reflection_context.h>
#include <scene/public/scene_entity_id.h>

#include <component_edit.h>
#include <edit_snapshot_registry.h>
#include <history.h>
#include <scene_document.h>

namespace editor {

class EditorSession {
  private:
    std::unique_ptr<SceneDocument> _sceneDocument;
    std::optional<engine::ecs::Entity> _selectedEntity{};

    std::optional<ComponentEdit> _currentComponentEdit = std::nullopt;
    const EditSnapshotRegistry& _snapshotRegistry;
    ComponentHistory _componentHistory;

    std::optional<engine::ecs::Entity> findEntityBySceneId(engine::scene::SceneEntityId id);

    std::expected<void, std::string> applyComponentSnapshot(const ComponentEditTarget& target, const IEditSnapshot& snapshot, engine::ecs_reflection::WorldReflectionContext& ctx);

  public:
    EditorSession(std::unique_ptr<SceneDocument>&& sceneDocument, const EditSnapshotRegistry& snapshotRegistry);

    /// @brief Get scene document
    /// @return Constant scene document
    [[nodiscard]] const SceneDocument& document() const noexcept;

    /// @brief Get mutable scene document
    /// @return Mutable scene document
    SceneDocument& documentMut();

    /// @brief Get selected entity
    /// @return Selected entity or nullopt
    [[nodiscard]] std::optional<engine::ecs::Entity> selectedEntity() const noexcept;

    /// @brief Select entity
    /// @param entity Entity to select
    /// @return True if entity is valid and selection completed successfuly
    bool select(engine::ecs::Entity entity, engine::ecs_reflection::WorldReflectionContext& ctx);

    /// @brief Clear selection. If selection is empty then do nothing
    bool clearSelection(engine::ecs_reflection::WorldReflectionContext& ctx);

    /// @return True if undo history is not empty
    [[nodiscard]] bool canUndo() const;

    /// @return True if redo history is not empty
    [[nodiscard]] bool canRedo() const;

    /// @brief Undo last operation in undo history. If history is empty then do noyhing
    /// @return Error of undo operation
    std::expected<void, std::string> undo(engine::ecs_reflection::WorldReflectionContext& ctx);

    /// @brief Redo last operation in redo history. If history is empty then do noyhing
    /// @return Error of redo operation
    std::expected<void, std::string> redo(engine::ecs_reflection::WorldReflectionContext& ctx);

    /// @brief Clear history
    void clearHistory();

    std::expected<void, std::string> beginComponentEdit(const ComponentEditTarget& target, const engine::reflection::ObjectView& rootView, EditInteraction interactionKey);

    std::expected<void, std::string> markComponentEditChanged(const ComponentEditTarget& target, EditInteraction interactionKey);

    std::expected<void, std::string> cancelComponentEdit(const ComponentEditTarget& target, const engine::reflection::ObjectView& rootView, EditInteraction interactionKey);

    std::expected<void, std::string> endComponentEdit(const ComponentEditTarget& target, const engine::reflection::ObjectView& rootView, EditInteraction interactionKey);

    std::expected<void, std::string> finishActiveComponentEdit(engine::ecs_reflection::WorldReflectionContext& ctx);

    std::expected<void, std::string> finishComponentEdit(std::string_view interactionKey, engine::ecs_reflection::WorldReflectionContext& ctx);

    bool isComponentEditActive(const ComponentEditTarget& target, EditInteraction interactionKey) const;
};

} // namespace editor
