#pragma once

#include <memory>
#include <typeindex>
#include <unordered_map>

#include <edit_snapshot.h>

namespace editor {

class EditSnapshotRegistry {
  public:
    using CaptureFn = std::unique_ptr<IEditSnapshot> (*)(const engine::reflection::ObjectView&);

  private:
    std::unordered_map<std::type_index, CaptureFn> _capturesMap{};

    template <typename T, typename Policy> static std::unique_ptr<IEditSnapshot> captureTyped(const engine::reflection::ObjectView& view) {
        auto value = view.tryAs<T>();
        if (!value) {
            return nullptr;
        }

        return std::move(std::make_unique<EditSnapshot<T, Policy>>(*value));
    }

  public:
    template <typename T, typename Policy> bool registerPolicy() {
        if (_capturesMap.contains(typeid(T))) {
            return false;
        }

        _capturesMap.emplace(typeid(T), &captureTyped<T, Policy>);
        return true;
    }

    template <typename T> bool registerDefaultCopyPolicy() {
        if (_capturesMap.contains(typeid(T))) {
            return true;
        }
        if constexpr (std::is_copy_assignable_v<T> && std::is_copy_constructible_v<T>) {
            return registerPolicy<T, CopySnapshotPolicy<T>>();
        } else {
            return true;
        }
    }

    CaptureFn findCapture(std::type_index typeIndex) const {
        if (_capturesMap.contains(typeIndex)) {
            return _capturesMap.at(typeIndex);
        }
        return nullptr;
    }
};

} // namespace editor