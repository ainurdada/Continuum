add_library(
    ContinuumEngine_InputSdl
    STATIC

    private/input_backend.cpp
)

set_module_defaults(ContinuumEngine_InputSdl STATIC)

target_link_libraries(
    ContinuumEngine_InputSdl
    PUBLIC
    ContinuumEngine_Input
    SDL3::SDL3
)
