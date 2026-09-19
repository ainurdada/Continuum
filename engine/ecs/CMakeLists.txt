add_library(
    ContinuumEngine_Ecs
    STATIC

    private/archetype.cpp
    private/command_buffer.cpp
    private/component_signature.cpp
    private/entity.cpp
    private/execution_layer.cpp
    private/query.cpp
    private/system.cpp
    private/world.cpp
)

target_include_directories(
    ContinuumEngine_Ecs
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
)

target_include_directories(
    ContinuumEngine_Ecs
    PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/public>
)


target_link_libraries(
    ContinuumEngine
    INTERFACE
    ContinuumEngine_Ecs
)
