#pragma once

#include <expected>
#include <string>

#include <reflection/public/object_view.h>

#include <edit_snapshot_policy.h>

namespace editor {

class IEditSnapshot {
    public:
    virtual ~IEditSnapshot() = default;

    virtual std::expected<void, std::string> restore(const engine::reflection::ObjectView& view) const = 0;
};

template<typename T, typename Policy> class EditSnapshot : public IEditSnapshot {
    using Snapshot = Policy::Snapshot;

    static_assert(std::is_object_v<T>);
    static_assert(!std::is_const_v<T>);
    static_assert(!std::is_volatile_v<T>);
    static_assert(std::is_object_v<Snapshot>);

    Snapshot _snaphop;

    public:
    EditSnapshot(const T& value) : _snaphop(Policy::capture(value)) {}

    std::expected<void, std::string> restore(const engine::reflection::ObjectView& view) const override {
        auto value = view.tryAsMut<T>();
        if (!value) {
            return std::unexpected("Failed to get mutable value of type:" + std::string(view.type()->name));
        }
        Policy::restore(*value, _snaphop);
        return {};
    }
};

} // namespace editor