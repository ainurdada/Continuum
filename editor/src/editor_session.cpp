#include <editor_session.h>

#include <stdexcept>

namespace editor {
std::optional<engine::ecs::Entity> EditorSession::findEntityBySceneId(engine::scene::SceneEntityId id) {
    auto& world = _sceneDocument->worldMut();
    auto stash = world.findStash<engine::scene::SceneEntityId>();
    if (!stash) {
        return std::nullopt;
    }
    auto query = world.query().with<engine::scene::SceneEntityId>().build();
    for (auto entity : query.view()) {
        if (id == *stash->get(entity)) {
            return entity;
        }
    }
    return std::nullopt;
}

std::expected<void, std::string> EditorSession::applyComponentSnapshot(const ComponentEditTarget& target, const IEditSnapshot& snapshot, engine::ecs_reflection::WorldReflectionContext& ctx) {
    auto entity = findEntityBySceneId(target.entityId);
    if (!entity) {
        return std::unexpected("Failed to apply component snapshot: failed to find entity");
    }

    std::expected<void, std::string> result = std::unexpected("Component was not found");
    if (!ctx.visitComponentsMut(entity.value(), [&result, &target, &snapshot](engine::ecs::IStash& stash, std::optional<engine::reflection::ObjectView> view) {
            if (stash.nativeTypeKey() != target.componentType) {
                return;
            }
            if (!view) {
                result = std::unexpected("No reflection view");
                return;
            }
            result = snapshot.restore(view.value());
        })) {
        return std::unexpected("Failed to visit entity");
    }

    return result;
}

EditorSession::EditorSession(std::unique_ptr<SceneDocument>&& sceneDocument, const EditSnapshotRegistry& snapshotRegistry) : _snapshotRegistry(snapshotRegistry) {
    if (!sceneDocument) {
        throw std::invalid_argument("sceneDocument is null");
    }
    _sceneDocument = std::move(sceneDocument);
}

const SceneDocument& EditorSession::document() const noexcept {
    return *_sceneDocument;
}

SceneDocument& EditorSession::documentMut() {
    return *_sceneDocument;
}

std::optional<engine::ecs::Entity> EditorSession::selectedEntity() const noexcept {
    return _selectedEntity;
}

bool EditorSession::select(engine::ecs::Entity entity, engine::ecs_reflection::WorldReflectionContext& ctx) {
    if (!_sceneDocument->world().hasEntity(entity)) {
        return false;
    }
    if (_selectedEntity && _selectedEntity.value() == entity) {
        return true;
    }
    if (!finishActiveComponentEdit(ctx)) {
        return false;
    }
    _selectedEntity = entity;
    return true;
}

bool EditorSession::clearSelection(engine::ecs_reflection::WorldReflectionContext& ctx) {
    if (!finishActiveComponentEdit(ctx)) {
        return false;
    }
    _selectedEntity = std::nullopt;
    return true;
}

bool EditorSession::canUndo() const {
    return _componentHistory.canUndo() && !_currentComponentEdit;
}

bool EditorSession::canRedo() const {
    return _componentHistory.canRedo() && !_currentComponentEdit;
}

std::expected<void, std::string> EditorSession::undo(engine::ecs_reflection::WorldReflectionContext& ctx) {
    if (canUndo()) {
        auto result = _componentHistory.undo([this, &ctx](const ComponentEditTarget& target, const IEditSnapshot& snapshot) { return applyComponentSnapshot(target, snapshot, ctx); });
        if (result) {
            _sceneDocument->markDirty();
            return {};
        }
        return std::unexpected(result.error());
    }
    return {};
}

std::expected<void, std::string> EditorSession::redo(engine::ecs_reflection::WorldReflectionContext& ctx) {
    if (canRedo()) {
        auto result = _componentHistory.redo([this, &ctx](const ComponentEditTarget& target, const IEditSnapshot& snapshot) { return applyComponentSnapshot(target, snapshot, ctx); });
        if (result) {
            _sceneDocument->markDirty();
            return {};
        }
        return std::unexpected(result.error());
    }
    return {};
}

void EditorSession::clearHistory() {
    _currentComponentEdit = std::nullopt;
    _componentHistory.clear();
}

std::expected<void, std::string> EditorSession::beginComponentEdit(const ComponentEditTarget& target, const engine::reflection::ObjectView& rootView, EditInteraction interactionKey) {
    if (interactionKey.source.empty()) {
        return std::unexpected("Empty interaction key");
    }

    if (!rootView.canWrite()) {
        return std::unexpected("Not mutable component");
    }

    if (rootView.type()->nativeTypeKey != target.componentType) {
        return std::unexpected("Type keys are not equal");
    }

    if (_currentComponentEdit) {
        if (_currentComponentEdit->target == target && _currentComponentEdit->interactionKey == interactionKey) {
            return {};
        }
        return std::unexpected("Another edit action is not finished yet");
    }

    auto capture = _snapshotRegistry.findCapture(target.componentType);
    if (!capture) {
        return std::unexpected("Type does not have snapshot policy");
    }

    auto snapshot = capture(rootView);
    if (!snapshot) {
        return std::unexpected("Failed to create edit snapshot");
    }

    ComponentEdit edit{};
    edit.target = target;
    edit.interactionKey = interactionKey;
    edit.before = std::move(snapshot);
    edit.changed = false;

    _currentComponentEdit = std::move(edit);
    return {};
}

std::expected<void, std::string> EditorSession::markComponentEditChanged(const ComponentEditTarget& target, EditInteraction interactionKey) {
    if (!_currentComponentEdit) {
        return std::unexpected("Failed to marked as changed: edit operation is not started");
    }

    if (_currentComponentEdit->target != target || _currentComponentEdit->interactionKey != interactionKey) {
        return std::unexpected("Failed to marked as changed: another edit operation is not finished yet");
    }

    _currentComponentEdit->changed = true;
    _sceneDocument->markDirty();
    return {};
}

std::expected<void, std::string> EditorSession::cancelComponentEdit(const ComponentEditTarget& target, const engine::reflection::ObjectView& rootView, EditInteraction interactionKey) {
    if (!_currentComponentEdit) {
        return std::unexpected("Failed to cancel changes: edit operation is not started");
    }

    if (_currentComponentEdit->target != target || _currentComponentEdit->interactionKey != interactionKey) {
        return std::unexpected("Failed to cancel changes: another edit operation is not finished yet");
    }

    if (!rootView.canWrite()) {
        return std::unexpected("Not mutable component");
    }

    if (rootView.type()->nativeTypeKey != target.componentType) {
        return std::unexpected("Type keys are not equal");
    }

    auto restoreResult = _currentComponentEdit->before->restore(rootView);
    if (!restoreResult) {
        return std::unexpected(restoreResult.error());
    }

    _currentComponentEdit = std::nullopt;
    return {};
}

std::expected<void, std::string> EditorSession::endComponentEdit(const ComponentEditTarget& target, const engine::reflection::ObjectView& rootView, EditInteraction interactionKey) {
    if (!_currentComponentEdit) {
        return std::unexpected("Failed to apply changes: edit operation is not started");
    }

    if (_currentComponentEdit->target != target || _currentComponentEdit->interactionKey != interactionKey) {
        return std::unexpected("Failed to apply changes: another edit operation is not finished yet");
    }

    if (!rootView.canWrite()) {
        return std::unexpected("Not mutable component");
    }

    if (rootView.type()->nativeTypeKey != target.componentType) {
        return std::unexpected("Type keys are not equal");
    }

    if (!_currentComponentEdit->changed) {
        _currentComponentEdit = std::nullopt;
        return {};
    }

    auto capture = _snapshotRegistry.findCapture(target.componentType);
    if (!capture) {
        return std::unexpected("Type does not have snapshot policy");
    }

    auto snapshot = capture(rootView);
    if (!snapshot) {
        return std::unexpected("Failed to create edit snapshot");
    }

    auto recordResult = _componentHistory.record(target, std::move(_currentComponentEdit->before), std::move(snapshot));
    if (!recordResult) {
        return std::unexpected(recordResult.error());
    }

    _currentComponentEdit = std::nullopt;
    return {};
}

std::expected<void, std::string> EditorSession::finishActiveComponentEdit(engine::ecs_reflection::WorldReflectionContext& ctx) {
    if (!_currentComponentEdit) {
        return {};
    }
    auto target = _currentComponentEdit->target;
    auto key = _currentComponentEdit->interactionKey;

    auto entity = findEntityBySceneId(target.entityId);
    if (!entity) {
        return std::unexpected("Failed to finis active component edot: failed to find entity");
    }

    std::expected<void, std::string> result = std::unexpected("Component was not fount");
    if (!ctx.visitComponentsMut(entity.value(), [&result, &target, &key, this](engine::ecs::IStash& stash, std::optional<engine::reflection::ObjectView> view) {
            if (stash.nativeTypeKey() != target.componentType) {
                return;
            }
            if (!view) {
                result = std::unexpected("No reflection view");
                return;
            }
            result = endComponentEdit(target, view.value(), key);
        })) {
        return std::unexpected("Failed to visit entity");
    }

    return result;
}

std::expected<void, std::string> EditorSession::finishComponentEdit(std::string_view interactionKey, engine::ecs_reflection::WorldReflectionContext& ctx) {
    if (interactionKey.empty()) {
        return std::unexpected("Interaction key is empy");
    }
    if (!_currentComponentEdit) {
        return {};
    }
    if (_currentComponentEdit->interactionKey.source != interactionKey) {
        return {};
    }

    return finishActiveComponentEdit(ctx);
}

bool EditorSession::isComponentEditActive(const ComponentEditTarget& target, EditInteraction interactionKey) const {
    return _currentComponentEdit && _currentComponentEdit->target == target && _currentComponentEdit->interactionKey == interactionKey;
}

} // namespace editor
