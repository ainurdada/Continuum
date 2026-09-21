add_library(
    ContinuumEngine_Input
    STATIC

    private/input.cpp
)

set_module_defaults(ContinuumEngine_Input STATIC)

target_link_libraries(
    ContinuumEngine_Input
    PUBLIC
    ContinuumEngine_Math
)
