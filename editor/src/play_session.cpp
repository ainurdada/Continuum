#include <play_session.h>

#include <SDL3/SDL_log.h>

#include <math/public/g_math.h>
#include <reflection_json/public/scene_serializer_json.h>
#include <scene/public/camera.h>
#include <scene/public/hierarchy.h>
#include <scene/public/mesh_reference.h>
#include <scene/public/mesh_renderer.h>
#include <scene/public/render_scene.h>
#include <scene/public/transform.h>

#include <prepare_game.h>

std::expected<void, std::string> editor::PlaySession::run(const nlohmann::json& sceneJson, engine::reflection::TypeRegistry& typeReg, engine::reflection::serialization::JsonSerializationRegistry& policies, engine::ecs_reflection::ComponentBindingRegistry& bindings) {
    engine::scene::serialization::json::SceneSerializerJson serializer{typeReg, policies, bindings};

    auto worldParsingResult = serializer.jsonToWorld(_world, sceneJson);
    if (!worldParsingResult) {
        return std::unexpected(worldParsingResult.error());
    }

    try {
        prepareGame(_world);
        _worldExecution.awake();
    } catch (std::exception& err) {
        return std::unexpected(err.what());
    }

    return {};
}

void editor::PlaySession::update(float deltaTime) {
    try {
        _worldExecution.update(deltaTime);
    } catch (std::exception& err) {
        SDL_LogError(SDL_LogCategory::SDL_LOG_CATEGORY_ERROR, "%s", err.what());
    }
}

void editor::PlaySession::renderScene(engine::RenderFrameData& frame, const std::unordered_map<engine::asset::AssetID, std::vector<engine::graphics::MeshHandle>, engine::asset::AssetIDHash>& meshHandles) {
    engine::RenderFrameInput frameInput{
        .world = _world,
        .renderEntities = _world.query().with<engine::scene::MeshRenderer>().with<engine::scene::Transform>().build(),
        .cameraEntities = _world.query().with<engine::scene::Camera>().with<engine::scene::Transform>().build(),
        .meshHandles = meshHandles,
    };

    engine::collectRenderFrameData(frameInput, frame);
}

void editor::PlaySession::destroy() {
    _worldExecution.destroy();
}

editor::PlaySession::~PlaySession() {
    if (_worldExecution.state() == engine::ecs::ExecutionLayer::State::Ready || _worldExecution.state() == engine::ecs::ExecutionLayer::State::Failed) {
        try {
            destroy();
        } catch (std::exception& err) {
            SDL_LogError(SDL_LogCategory::SDL_LOG_CATEGORY_ERROR, "%s", err.what());
        }
    }
}
