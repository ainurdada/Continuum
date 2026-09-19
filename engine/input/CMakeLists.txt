add_library(
    ContinuumEngine_Input
    STATIC

    private/input.cpp
)

target_include_directories(
    ContinuumEngine_Input
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
)

target_include_directories(
    ContinuumEngine_Input
    PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/public>
)

target_link_libraries(
    ContinuumEngine_Input
    PUBLIC
    ContinuumEngine_Math
)


target_link_libraries(
    ContinuumEngine
    INTERFACE
    ContinuumEngine_Input
)
