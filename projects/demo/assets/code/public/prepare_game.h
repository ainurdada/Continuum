#pragma once

#include <optional>

#include <SDL3/SDL_log.h>

#include <ecs/ecs.h>
#include <runtime/public/runtime.h>

#include <fly_camera_controller.h>
#include <spin.h>
#include <spin_system.h>

namespace {

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

engine::Result prepareGame(engine::ecs::World& world) {
    using namespace engine;

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

    return Result::Continue;
}