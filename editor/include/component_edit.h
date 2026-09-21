#pragma once

#include <typeindex>
#include <string>
#include <memory>
#include <vector>

#include <scene/public/scene_entity_id.h>

#include <edit_snapshot.h>

namespace editor {

struct EditInteraction {
    std::string source;
    std::vector<std::string> fieldPath;

    bool operator==(const EditInteraction& other) const {
        return source == other.source && fieldPath == other.fieldPath;
    }
};

struct ComponentEditTarget {
    engine::scene::SceneEntityId entityId;
    std::type_index componentType = typeid(void);

    bool operator==(const ComponentEditTarget& other) const {
        return entityId == other.entityId && componentType == other.componentType;
    }
};

struct ComponentEdit {
    ComponentEditTarget target;
    EditInteraction interactionKey;
    std::unique_ptr<IEditSnapshot> before;
    bool changed = false;
};

}