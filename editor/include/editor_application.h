#pragma once

#include <memory>
#include <optional>

#include <imgui.h>

#include <console_panel.h>
#include <content_browser_panel.h>
#include <edit_snapshot_registry.h>
#include <editor_session.h>
#include <hierarchy_panel.h>
#include <inspector_panel.h>
#include <project.h>
#include <scene_document.h>
#include <ui/component_drawer_registry.h>

#include <ecs_reflection/public/component_binding_registry.h>
#include <reflection_json/public/json_serialization_registry.h>
#include <reflection/public/type_registry.h>

namespace editor {

class EditorApplication {
    Project _project;

    engine::reflection::TypeRegistry _typeRegistry;
    engine::ecs_reflection::ComponentBindingRegistry _componentBindings;

    engine::reflection::serialization::JsonSerializationRegistry _jsonSerializationRegistry;
    EditSnapshotRegistry _snapshotRegistry;
    ui::ComponentDrawerRegistry _componentDrawers;

    std::optional<EditorSession> _session;

    HierarchyPanel _hierarchy{};
    InspectorPanel _inspector{};
    ConsolePanel _console{};
    ContentBrowserPanel _contentBrowser;

    bool trySaveScene(engine::ecs_reflection::WorldReflectionContext& ctx);

    void drawMainMenuBar(engine::ecs_reflection::WorldReflectionContext& ctx);

    void buildDefaultDockLayout(ImGuiID dockspaceId);

  public:
    EditorApplication(Project project);

    /// @brief run editor application
    /// @return return code (0 if run didn't have errors)
    int run();

    template <typename TDrawer> bool registerComponentDrawer() {
        return _componentDrawers.registerDrawer<TDrawer>();
    }

    template <typename T, typename Policy> bool registerJsonSerialization() {
        return _jsonSerializationRegistry.registerPolicy<T, Policy>();
    }
};

} // namespace editor
