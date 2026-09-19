#pragma once

#include <typeindex>
#include <unordered_map>

#include "component_binding.h"

namespace engine::ecs_reflection {

class ComponentBindingRegistry {
    std::unordered_map<std::type_index, ComponentBinding> _bindings{};
    bool _frozen = false;

  public:
    void freeze() noexcept {
        _frozen = true;
    }

    bool isFrozen() const noexcept {
        return _frozen;
    }

    template <typename T> bool registerComponent(const reflection::TypeRegistry& reg) {
        if (isFrozen()) {
            return false;
        }
        auto binding = makeComponentBinding<T>(reg);
        if (!binding) {
            return false;
        }
        return _bindings.emplace(binding->nativeTypeIndex, *binding).second;
    }

    const ComponentBinding* findBinding(std::type_index typeIndex) const {
        auto binging = _bindings.find(typeIndex);
        if (binging == _bindings.end()) {
            return nullptr;
        }
        return &binging->second;
    }
};

} // namespace engine::ecs_reflection
