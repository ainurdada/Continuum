#pragma once

#include <optional>

#include <engine.h>
#include <ecs/ecs.h>
#include <spin.h>

class SpinSystem : public engine::ecs::ISystem {
    engine::ecs::Stash<engine::scene::Transform>* _transforms;
    engine::ecs::Stash<demo::Spin>* _spins;
    std::optional<engine::ecs::Query> _spinQuery;

    void awake(engine::ecs::World& world) override;
    void update(float deltaTime) override;
    void destroy() override;
};
