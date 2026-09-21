add_library(
    ContinuumEngine_Ecs
    STATIC

    private/archetype.cpp
    private/command_buffer.cpp
    private/component_signature.cpp
    private/entity.cpp
    private/execution_layer.cpp
    private/query.cpp
    private/system.cpp
    private/world.cpp
)

set_module_defaults(ContinuumEngine_Ecs STATIC)
