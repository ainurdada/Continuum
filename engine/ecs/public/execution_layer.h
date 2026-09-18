#pragma once

#include <vector>
#include <memory>

#include <public/system.h>
#include <public/command_buffer.h>

namespace engine::ecs {

class World;

class ExecutionLayer {
  private:
    friend World;
    friend class WorldExecution;

    enum class State {
        Configuring,
        Initializing,
        Ready,
        Failed,
        Destroying,
        Destroyed
    };

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
};

}