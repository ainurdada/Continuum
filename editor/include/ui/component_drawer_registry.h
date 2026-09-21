#pragma once

#include <memory>
#include <typeindex>
#include <unordered_map>

#include <ui/component_drawer.h>

namespace editor::ui {

class ComponentDrawerRegistry {
    std::unordered_map<std::type_index, std::unique_ptr<IComponentDrawer>> _drawers;

  public:
    template <typename TDrawer> bool registerDrawer() {
        std::type_index key = typeid(typename TDrawer::ValueType);
        if (_drawers.contains(key)) {
            return false;
        }

        _drawers.emplace(key, std::make_unique<TDrawer>());
        return true;
    }

    IComponentDrawer* find(std::type_index typeIndex) const {
        auto drawer = _drawers.find(typeIndex);
        if (drawer == _drawers.end()) {
            return nullptr;
        }
        return drawer->second.get();
    }
};

} // namespace editor::ui
