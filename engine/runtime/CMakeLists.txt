add_library(
    ContinuumEngine_Runtime
    INTERFACE
)

set_module_defaults(ContinuumEngine_Runtime INTERFACE)

target_link_libraries(
    ContinuumEngine_Runtime
    INTERFACE
    ContinuumEngine_Input
    ContinuumEngine_Render
)
