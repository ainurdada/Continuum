#pragma once

#define INSPECTOR_WINDOW_NAME "Inspector"

namespace engine::ecs_reflection {

class WorldReflectionContext;
class ComponentBindingRegistry;

} // namespace engine::ecs_reflection

namespace editor::ui {

class ComponentDrawerRegistry;

} // namespace editor::ui

namespace editor {

class EditorSession;

struct InspectorDrawData {
    EditorSession& session;
    engine::ecs_reflection::WorldReflectionContext& ctx;
    const ui::ComponentDrawerRegistry& drawers;
    const engine::ecs_reflection::ComponentBindingRegistry& bindings;
};

class InspectorPanel {
  public:
    bool deleteChildrenMode = false;

    void draw(InspectorDrawData& data);
};

} // namespace editor
