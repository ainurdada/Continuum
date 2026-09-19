add_library(
    ContinuumEngine_ReflectionJson
    STATIC

    private/builtin_json_policies.cpp
    private/json_serialization.cpp
)

target_include_directories(
    ContinuumEngine_ReflectionJson
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/public>
)

target_link_libraries(
    ContinuumEngine_ReflectionJson
    PUBLIC
    ContinuumEngine_Reflection
    nlohmann_json::nlohmann_json
)