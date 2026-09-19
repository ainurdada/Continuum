add_library(
    ContinuumEngine_EcsReflection
    INTERFACE
)

target_link_libraries(
    ContinuumEngine_EcsReflection
    INTERFACE
    ContinuumEngine_Reflection
    ContinuumEngine_Ecs
)

target_include_directories(
    ContinuumEngine_EcsReflection
    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
)


target_link_libraries(
    ContinuumEngine
    INTERFACE
    ContinuumEngine_EcsReflection
)
