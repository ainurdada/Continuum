add_library(
    ContinuumEngine_RuntimeSdl
    OBJECT

    private/sdl_entry.cpp
)

target_link_libraries(
    ContinuumEngine_RuntimeSdl
    PRIVATE
    ContinuumEngine_Runtime
    ContinuumEngine_RenderSdl
    ContinuumEngine_InputSdl
    ContinuumEngine_SdlSupport
    SDL3::SDL3
)
