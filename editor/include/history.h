#pragma once

#include <cstddef>
#include <expected>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <ecs/ecs.h>
#include <scene/public/scene_entity_id.h>
#include <scene/public/transform.h>

#include <component_edit.h>
#include <edit_snapshot.h>

namespace editor {

using ApplySnaphotFn = std::function<std::expected<void, std::string>(const ComponentEditTarget&, const IEditSnapshot&)>;

/// @brief Component history entry should have before and after snapshots.
/// Ownership is transferred by std::move
struct ComponentHistoryEntry {
    ComponentEditTarget target;
    std::unique_ptr<IEditSnapshot> before;
    std::unique_ptr<IEditSnapshot> after;
};

class ComponentHistory {
    std::vector<ComponentHistoryEntry> _entries{};
    std::size_t _appliedCount = 0;

  public:
    /// @return True if undo history is not empty
    [[nodiscard]] bool canUndo() const;

    /// @return True if redo history is not empty
    [[nodiscard]] bool canRedo() const;

    /// @brief Clear history
    void clear();

    std::expected<void, std::string> record(const ComponentEditTarget& target, std::unique_ptr<IEditSnapshot>&& before, std::unique_ptr<IEditSnapshot>&& after);

    std::expected<void, std::string> undo(const ApplySnaphotFn& applySnapshot);

    std::expected<void, std::string> redo(const ApplySnaphotFn& applySnapshot);
};

}