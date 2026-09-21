#pragma once

#define HIERARCHY_WINDOW_NAME "Hierarchy"

namespace engine::ecs_reflection {

class WorldReflectionContext;

} // namespace engine::ecs_reflection

namespace editor {

class EditorSession;

class HierarchyPanel {
  public:
    void draw(EditorSession& session, engine::ecs_reflection::WorldReflectionContext& ctx);
};

} // namespace editor
