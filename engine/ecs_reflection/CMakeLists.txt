add_library(
    ContinuumEngine_EcsReflection
    INTERFACE
)

target_link_libraries(
    ContinuumEngine_EcsReflection
    INTERFACE
    ContinuumEngine_Reflection
    ContinuumEngine_Ecs
)

set_module_defaults(ContinuumEngine_EcsReflection INTERFACE)
