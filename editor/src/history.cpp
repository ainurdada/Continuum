#include <history.h>

#include <optional>

namespace editor {

namespace {
std::optional<engine::ecs::Entity> getEntityBySceneID(engine::scene::SceneEntityId id, const engine::ecs::Stash<engine::scene::SceneEntityId>& sceneEntityIdStash, engine::ecs::Query sceneEntityIdQuery) {
    for (auto entity : sceneEntityIdQuery.view()) {
        if (*sceneEntityIdStash.get(entity) == id) {
            return entity;
        }
    }
    return std::nullopt;
}
} // namespace

bool ComponentHistory::canUndo() const {
    return _appliedCount > 0;
}

bool ComponentHistory::canRedo() const {
    return _appliedCount < _entries.size();
}

void ComponentHistory::clear() {
    _entries.clear();
    _appliedCount = 0;
}

std::expected<void, std::string> ComponentHistory::record(const ComponentEditTarget& target, std::unique_ptr<IEditSnapshot>&& before, std::unique_ptr<IEditSnapshot>&& after) {
    if (!before) {
        return std::unexpected("Failed to record edit: no before snapshot");
    }
    if (!after) {
        return std::unexpected("Failed to record edit: no after snapshot");
    }
    if (std::addressof(*before) == std::addressof(*after)) {
        return std::unexpected("Failed to record edit: before and after snapshots are qual");
    }

    if (_appliedCount == _entries.size()) {
        _entries.emplace_back();
    }

    _entries[_appliedCount].target = target;
    _entries[_appliedCount].before = std::move(before);
    _entries[_appliedCount].after = std::move(after);

    _entries.resize(_appliedCount + 1);
    _appliedCount = _entries.size();

    return {};
}

std::expected<void, std::string> ComponentHistory::undo(const ApplySnaphotFn& applySnapshot) {
    if (!canUndo()) {
        return {};
    }

    if (!applySnapshot) {
        return std::unexpected("applySnapshot is null");
    }
    auto applyResult = applySnapshot(_entries[_appliedCount - 1].target, *_entries[_appliedCount - 1].before);

    if (!applyResult) {
        return std::unexpected(applyResult.error());
    }

    _appliedCount--;
    return {};
}

std::expected<void, std::string> ComponentHistory::redo(const ApplySnaphotFn& applySnapshot) {
    if (!canRedo()) {
        return {};
    }

    if (!applySnapshot) {
        return std::unexpected("applySnapshot is null");
    }
    auto applyResult = applySnapshot(_entries[_appliedCount].target, *_entries[_appliedCount].after);

    if (!applyResult) {
        return std::unexpected(applyResult.error());
    }

    _appliedCount++;
    return {};
}

} // namespace editor