#include <ecs/ecs.h>

#include <filesystem>
#include <optional>

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_log.h>

#include <ecs_reflection/public/component_binding_registry.h>
#include <ecs_reflection/public/ecs_reflection_bootstrap.h>
#include <engine.h>
#include <reflection/public/bootstrap.h>
#include <reflection/public/type_registry.h>
#include <reflection_json/public/builtin_json_policies.h>
#include <reflection_json/public/json_serialization_registry.h>
#include <reflection_json/public/scene_serializer_json.h>
#include <runtime/public/runtime.h>
#include <scene/public/camera.h>
#include <scene/public/mesh_renderer.h>
#include <scene/public/scene_entity_id.h>

#include "tools/time.h"
#include <fly_camera_controller.h>
#include <spin.h>
#include <spin_system.h>

#include <reflection/public/builtin_types.h>

namespace {

struct GameState {
    Time time{};

    engine::reflection::TypeRegistry typeRegistry;
    engine::ecs_reflection::ComponentBindingRegistry componentBindings;

    engine::ecs::World world{};
    std::optional<engine::ecs::WorldExecution> worldExecution = std::nullopt;
    engine::ecs::Query renderQuery = world.query().with<engine::scene::Transform>().with<engine::scene::MeshRenderer>().build();
    engine::ecs::Query cameraQuery = world.query().with<engine::scene::Transform>().with<engine::scene::Camera>().build();

    engine::reflection::serialization::JsonSerializationRegistry jsonSerializationRegistry;
};

std::optional<engine::ecs::Entity> findEntityBySceneId(engine::ecs::World& world, const engine::scene::SceneEntityId sceneId) {
    auto& sceneIdStash = world.getStash<engine::scene::SceneEntityId>();

    auto sceneIdQuery = world.query().with<engine::scene::SceneEntityId>().build();
    for (auto entity : sceneIdQuery.view()) {
        if (*sceneIdStash.get(entity) == sceneId) {
            return entity;
        }
    }
    return std::nullopt;
}

} // namespace

namespace engine {

GameDescription createGame() {
    return {.title = "Continuum", .windowWidth = 1280, .windowHeight = 720};
}

Result startGame(void** appState) {
    // Create game state
    GameState* newGameState = new GameState();
    *appState = newGameState;
    ecs::World& world = newGameState->world;

    auto basePath = SDL_GetBasePath();
    if (!basePath) {
        return Result::Failure;
    }

    if (!engine::reflection::generated::initializeTypeRegistry(newGameState->typeRegistry)) {
        SDL_Log("Failed to initialize type registry");
        return Result::Failure;
    }

    if (!engine::ecs_reflection::generated::initializeComponentBindings(newGameState->componentBindings, newGameState->typeRegistry)) {
        SDL_Log("Failed to initialize component bindings");
        return Result::Failure;
    }

    const std::filesystem::path scenePath = std::filesystem::path{basePath} / "scenes" / "main.cscn";

    if (!reflection::serialization::registerBuiltinJsonPolicies(newGameState->jsonSerializationRegistry)) {
        SDL_Log("Failed to register builtin policies");
        return Result::Failure;
    }

    engine::scene::serialization::json::SceneSerializerJson serializer(newGameState->typeRegistry, newGameState->jsonSerializationRegistry, newGameState->componentBindings);

    const auto& sceneLoadResult = serializer.deserialize(world, scenePath);
    if (!sceneLoadResult.has_value()) {
        SDL_Log("%s", sceneLoadResult.error().c_str());
        return Result::Failure;
    }

    auto& flyCameraStash = world.getStash<demo::FlyCameraController>();
    auto& spinStash = world.getStash<demo::Spin>();

    std::optional<ecs::Entity> camera = findEntityBySceneId(world, scene::SceneEntityId{1});
    if (!camera.has_value()) {
        return Result::Failure;
    }
    flyCameraStash.add(camera.value(), {});
    if (!flyCameraStash.has(camera.value())) {
        return Result::Failure;
    }

    std::optional<ecs::Entity> cube1 = findEntityBySceneId(world, scene::SceneEntityId{2});
    if (!cube1.has_value()) {
        return Result::Failure;
    }
    spinStash.add(cube1.value(), demo::Spin{.rotationRate = {1, 1, 0}, .timeSource = demo::Spin::TimeSource::Simulation});
    if (!spinStash.has(cube1.value())) {
        return Result::Failure;
    }

    std::optional<ecs::Entity> cube2 = findEntityBySceneId(world, scene::SceneEntityId{3});
    if (!cube2.has_value()) {
        return Result::Failure;
    }
    spinStash.add(cube2.value(), demo::Spin{.rotationRate = {0, 0.25, 0}, .timeSource = demo::Spin::TimeSource::Real});
    if (!spinStash.has(cube2.value())) {
        return Result::Failure;
    }

    auto& systemGroup = world.createSystemGroup();

    systemGroup.registerSystem<SpinSystem>();

    newGameState->worldExecution.emplace(world);
    try {
        newGameState->worldExecution->awake();
    } catch (...) {
        SDL_Log("failed to awake ecs world systems");
        return Result::Failure;
    }

    return Result::Continue;
}

Result updateGame(void* appState, const Engine& engine, const FrameInfo& frameInfo) {
    // Get Game State
    auto& gameState = *static_cast<GameState*>(appState);

    // Set time
    auto& time = gameState.time;
    if (engine.input().isPressed(engine::input::Key::SPACE)) {
        time.pause(!time.pause());
    }
    time.addTime(frameInfo.realDeltaSeconds);

    if (gameState.worldExecution) {
        try {
            gameState.worldExecution->update(time.simulationDeltaSeconds());
        } catch (...) {
            SDL_Log("failed to update ecs world systems");
            return Result::Failure;
        }
    }

    auto& world = gameState.world;
    auto& stashTransform = world.getStash<scene::Transform>();
    auto& stashSpin = world.getStash<demo::Spin>();

    Vec3f inputDirection{0, 0, 0};
    if (engine.input().isHold(engine::input::Key::D)) {
        inputDirection.x += 1;
    }
    if (engine.input().isHold(engine::input::Key::A)) {
        inputDirection.x -= 1;
    }
    if (engine.input().isHold(engine::input::Key::W)) {
        inputDirection.z += 1;
    }
    if (engine.input().isHold(engine::input::Key::S)) {
        inputDirection.z -= 1;
    }

    auto& flyCameraControllerStash = world.getStash<demo::FlyCameraController>();
    engine::ecs::Query flyCameraQuery = world.query().with<engine::scene::Transform>().with<demo::FlyCameraController>().build();
    for (const auto& entity : flyCameraQuery.view()) {
        auto* transform = stashTransform.getMut(entity);
        const auto* flyCameraController = flyCameraControllerStash.get(entity);
        float deltaSeconds = static_cast<float>(time.realDeltaSeconds());

        transform->position += math::worldRight * deltaSeconds * inputDirection.x * flyCameraController->moveSpeed;
        transform->position += math::worldUp * deltaSeconds * inputDirection.y * flyCameraController->moveSpeed;
        transform->position += math::worldForward * deltaSeconds * inputDirection.z * flyCameraController->moveSpeed;

        transform->rotation.y += engine.input().mouseDeltaPosition().x * flyCameraController->lookSensitivity;
        transform->rotation.x += engine.input().mouseDeltaPosition().y * flyCameraController->lookSensitivity;
    }

    return Result::Continue;
}

Result renderGame(const void* appState, RenderFrameData& data) {
    // Get Game State
    const GameState& gameState = *static_cast<const GameState*>(appState);
    const engine::ecs::World& world = gameState.world;
    const auto* meshRendererStash = world.findStash<scene::MeshRenderer>();
    const auto* cameraStash = world.findStash<scene::Camera>();

    if (!meshRendererStash || !cameraStash) {
        return Result::Failure;
    }

    // Set camera data
    int foundCameraCount = 0;
    for (const auto& entity : gameState.cameraQuery.view()) {
        auto getWorldMatrixResult = scene::worldMatrix(world, entity);
        if (!getWorldMatrixResult.has_value()) {
            return Result::Failure;
        }
        const auto* camera = cameraStash->get(entity);
        data.camera.verticalFovRadians = camera->verticalFov;
        data.camera.nearPlane = camera->nearPlane;
        data.camera.farPlane = camera->farPlane;
        data.camera.viewMatrix = math::inverse(getWorldMatrixResult.value());
        foundCameraCount++;
    }
    if (foundCameraCount != 1) {
        return Result::Failure;
    }

    for (const auto& entity : gameState.renderQuery.view()) {
        // Set item data
        auto getWorldMatrixResult = scene::worldMatrix(world, entity);
        if (!getWorldMatrixResult.has_value()) {
            return Result::Failure;
        }
        Mat4f modelMatrix = getWorldMatrixResult.value();

        const auto* meshRenderer = meshRendererStash->get(entity);
        if (!meshRenderer) {
            return Result::Failure;
        }

        // Add item to render data
        data.items.push_back(RenderItem{.geometryId = meshRenderer->geometryId, .modelMatrix = modelMatrix});
    }

    return Result::Continue;
}

Result processEvent(void* appState, const Event& event) {
    switch (event.type) {
    case EventType::CloseRequested:
        return Result::Success;

    default:
        return Result::Continue;
    }
}

void stopGame(void* appState, Result result) {
    // Get Game State
    GameState* gameState = static_cast<GameState*>(appState);

    // Destroy game state
    if (gameState) {
        if (gameState->worldExecution) {
            try {
                gameState->worldExecution->destroy();
            } catch (...) {
                SDL_Log("failed to destroy ecs world systems");
            }
        }
        delete gameState;
    }
}

} // namespace engine