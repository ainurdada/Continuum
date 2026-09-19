add_library(
    ContinuumEngine_Math
    STATIC

    private/g_math.cpp
)

target_include_directories(
    ContinuumEngine_Math
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
)

target_include_directories(
    ContinuumEngine_Math
    PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/public>
)

target_link_libraries(
    ContinuumEngine_Math
    PUBLIC
    glm::glm
)


target_link_libraries(
    ContinuumEngine
    INTERFACE
    ContinuumEngine_Math
)
