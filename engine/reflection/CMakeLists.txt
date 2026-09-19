add_library(
    ContinuumEngine_Reflection
    INTERFACE
)

target_include_directories(
    ContinuumEngine_Reflection
    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/public>
)
