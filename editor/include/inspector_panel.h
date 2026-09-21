#pragma once

#define INSPECTOR_WINDOW_NAME "Inspector"

namespace engine::ecs_reflection {

class WorldReflectionContext;

} // namespace engine::ecs_reflection

namespace editor::ui {

class ComponentDrawerRegistry;

} // namespace editor::ui

namespace editor {

class EditorSession;

class InspectorPanel {
  public:
    bool deleteChildrenMode = false;

    void draw(EditorSession& session, engine::ecs_reflection::WorldReflectionContext& ctx, const ui::ComponentDrawerRegistry& drawers);
};

} // namespace editor
