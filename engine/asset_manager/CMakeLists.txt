add_library(
    ContinuumEngine_AssetManager
    STATIC
    private/asset.cpp
    private/asset_registry.cpp
    private/asset_importer.cpp
    private/asset_cache.cpp
)

set_module_defaults(ContinuumEngine_AssetManager STATIC)

target_link_libraries(
    ContinuumEngine_AssetManager
    PRIVATE
    nlohmann_json::nlohmann_json
    assimp::assimp
)

target_link_libraries(
    ContinuumEngine_AssetManager
    PUBLIC
    ContinuumEngine_Render
)
