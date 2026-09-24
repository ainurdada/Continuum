#pragma once

#include <memory>
#include <vector>

#include "command_buffer.h"
#include "system.h"

namespace engine::ecs {

class World;

class ExecutionLayer {
  private:
    friend World;
    friend class WorldExecution;

  public:
    enum class State {
        Configuring,
        Initializing,
        Ready,
        Failed,
        Destroying,
        Destroyed
    };

  private:
    State _state = State::Configuring;

    World& _world;
    std::vector<std::unique_ptr<SystemGroup>> _systemGroups{};

    CommandBuffer _commandBuffer{};

    explicit ExecutionLayer(World& world) : _world(world) {}

    template <typename T> void execute(T&& systemFunction);
    SystemGroup& createGroup();

    void awake();
    void update(float deltaTime);
    void destroy();

  public:
    ExecutionLayer() = delete;

    ExecutionLayer(const ExecutionLayer&) = delete;
    ExecutionLayer& operator=(const ExecutionLayer&) = delete;
    ExecutionLayer(ExecutionLayer&&) = delete;
    ExecutionLayer& operator=(ExecutionLayer&&) = delete;
};

class WorldExecution {
    World& _world;

  public:
    WorldExecution(World& world) : _world(world) {}

    void awake();
    void update(float deltaTime);
    void destroy();
    ExecutionLayer::State state() const;
};

} // namespace engine::ecs