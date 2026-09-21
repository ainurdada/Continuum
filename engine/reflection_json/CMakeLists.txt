add_library(
    ContinuumEngine_ReflectionJson
    STATIC

    private/builtin_json_policies.cpp
    private/json_serialization.cpp
    private/scene_serializer_json.cpp
)

set_module_defaults(ContinuumEngine_ReflectionJson STATIC)

target_link_libraries(
    ContinuumEngine_ReflectionJson
    
    PUBLIC
    ContinuumEngine_Reflection
    ContinuumEngine_Scene
    nlohmann_json::nlohmann_json

    PRIVATE
    ContinuumEngine_EcsReflection
)
