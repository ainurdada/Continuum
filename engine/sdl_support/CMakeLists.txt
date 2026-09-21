add_library(
    ContinuumEngine_SdlSupport
    INTERFACE
)

set_module_defaults(ContinuumEngine_SdlSupport INTERFACE)

target_link_libraries(
    ContinuumEngine_SdlSupport
    INTERFACE
    SDL3::SDL3
)