add_executable(
    ContinuumEditor

    main.cpp
    src/project.cpp
    include/scene_document.h
    src/scene_document.cpp
    include/content_browser_panel.h
    src/content_browser_panel.cpp
    include/editor_session.h
    src/editor_session.cpp
    include/history.h
    src/history.cpp
    include/inspector_panel.h
    src/inspector_panel.cpp
    include/hierarchy_panel.h
    src/hierarchy_panel.cpp
    include/console_log.h
    src/console_log.cpp
    include/console_panel.h
    src/console_panel.cpp
    include/scene_viewport.h
    src/scene_viewport.cpp
    include/editor_application.h
    src/editor_application.cpp
    include/ui/draw_context.h
    include/edit_snapshot_policy.h
    include/edit_snapshot.h
    include/edit_snapshot_registry.h
    include/component_edit.h
    include/ui/drawers/transform_drawer.h
    include/ui/component_drawer.h
    include/ui/component_drawer_registry.h
    include/ui/drawers/float_drawer.h
    include/ui/drawers/bool_drawer.h
    include/ui/drawers/int_drawer.h
    include/ui/drawers/double_drawer.h
    include/ui/modifierHelpers.h
    include/ui/drawers/string_drawer.h
    include/snapshot_bootstrap.h
)

continuum_link_modules(
    ContinuumEditor

    MODULES
    ContinuumEngine
    ContinuumEngine_Scene
    nlohmann_json::nlohmann_json
    ContinuumImGui
    imguizmo::imguizmo
)

continuum_finalize_reflection(ContinuumEditor EDITOR)

target_include_directories(
    ContinuumEditor
    PRIVATE
    "${PROJECT_SOURCE_DIR}/editor/include"
)

continuum_configure_runtime(ContinuumEditor)
