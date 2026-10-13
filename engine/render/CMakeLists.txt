add_library(
    ContinuumEngine_Render
    STATIC
    private/view.cpp
)

set_module_defaults(ContinuumEngine_Render STATIC)

target_link_libraries(
    ContinuumEngine_Render
    PUBLIC
    ContinuumEngine_Math
)