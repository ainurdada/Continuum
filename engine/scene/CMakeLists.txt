add_library(
    ContinuumEngine_Scene
    STATIC

    private/builtin_component_metadata.cpp
    private/hierarchy.cpp
    private/transform.cpp
)

set_module_defaults(ContinuumEngine_Scene STATIC)

target_link_libraries(
    ContinuumEngine_Scene
    PUBLIC
    ContinuumEngine_Ecs
    ContinuumEngine_Math
    ContinuumEngine_Reflection
    ContinuumEngine_Render
)

continuum_enable_reflection(ContinuumEngine_Scene MODULE Scene ROOTS ${CMAKE_CURRENT_SOURCE_DIR}/public)
