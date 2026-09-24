#include <play_session.h>

#include <SDL3/SDL_log.h>

#include <math/public/g_math.h>
#include <reflection_json/public/scene_serializer_json.h>
#include <scene/public/camera.h>
#include <scene/public/hierarchy.h>
#include <scene/public/mesh_renderer.h>
#include <scene/public/transform.h>

std::expected<void, std::string> editor::PlaySession::run(const nlohmann::json& sceneJson, engine::reflection::TypeRegistry& typeReg, engine::reflection::serialization::JsonSerializationRegistry& policies, engine::ecs_reflection::ComponentBindingRegistry& bindings) {
    engine::scene::serialization::json::SceneSerializerJson serializer{typeReg, policies, bindings};

    auto worldParsingResult = serializer.jsonToWorld(_world, sceneJson);
    if (!worldParsingResult) {
        return std::unexpected(worldParsingResult.error());
    }

    try {
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

void editor::PlaySession::renderScene(engine::RenderFrameData& frame) {
    auto& transforms = _world.getStash<engine::scene::Transform>();
    auto& meshRenderers = _world.getStash<engine::scene::MeshRenderer>();
    auto& cameras = _world.getStash<engine::scene::Camera>();

    auto meshQuery = _world.query().with<engine::scene::MeshRenderer>().with<engine::scene::Transform>().build();
    auto cameraQuery = _world.query().with<engine::scene::Camera>().with<engine::scene::Transform>().build();

    for (auto entity : meshQuery.view()) {
        auto transform = transforms.get(entity);
        auto mesh = meshRenderers.get(entity);

        engine::RenderItem item{};
        item.geometryId = engine::GeometryId::Cube;
        auto worldMatrix = engine::scene::worldMatrix(_world, entity);
        if (!worldMatrix) {
            continue;
        }
        item.modelMatrix = worldMatrix.value();

        frame.items.push_back(item);
    }

    for (auto entity : cameraQuery.view()) {
        auto camera = cameras.get(entity);
        auto transform = transforms.get(entity);
        auto cameraMatrix = engine::scene::worldMatrix(_world, entity);
        if (!cameraMatrix) {
            continue;
        }

        engine::RenderCameraData rCamera{};
        rCamera.verticalFovRadians = camera->verticalFov;
        rCamera.nearPlane = camera->nearPlane;
        rCamera.farPlane = camera->farPlane;
        rCamera.viewMatrix = math::inverse(cameraMatrix.value());

        frame.camera = rCamera;
        break;
    }

    frame.drawGlobalGrid = false;
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
