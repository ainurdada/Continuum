add_library(
    ContinuumEngine_RenderSdl
    STATIC

    private/color_target.cpp
    private/depth_target.cpp
    private/graphics_context.cpp
    private/graphics_pipeline.cpp
    private/mesh.cpp
    private/renderer.cpp
    private/shader.cpp
)

set_module_defaults(ContinuumEngine_RenderSdl STATIC)

target_link_libraries(
    ContinuumEngine_RenderSdl
    PUBLIC
    SDL3::SDL3
    ContinuumEngine_Render

    PRIVATE
    ContinuumEngine_Math
    ContinuumEngine_SdlSupport
)