add_library(
    ContinuumEngine_Render
    INTERFACE
)

set_module_defaults(ContinuumEngine_Render INTERFACE)

target_link_libraries(
    ContinuumEngine_Render
    INTERFACE
    ContinuumEngine_Math
)