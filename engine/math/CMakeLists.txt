add_library(
    ContinuumEngine_Math
    STATIC

    private/g_math.cpp
)

set_module_defaults(ContinuumEngine_Math STATIC)

target_link_libraries(
    ContinuumEngine_Math
    PUBLIC
    glm::glm
)
