#pragma once

#ifndef ECS_H_INCLUDED
#include "ecs/ecs.h"
#endif

namespace engine::ecs {

template <typename T> void ExecutionLayer::execute(T&& systemFunction) {
    if (_world._commands) {
        throw std::logic_error("world already has active commands");
    }
    try {
        Commands commands{_world, _commandBuffer};
        _world._commands = &commands;
        std::forward<T>(systemFunction)();
        _world.commit();
        _world._commands = nullptr;
    } catch (...) {
        _world._commands = nullptr;
        _world.releaseReservations(_commandBuffer);
        _commandBuffer.clear();
        throw;
    }
    _world.releaseReservations(_commandBuffer);
    _commandBuffer.clear();
}

} // namespace engine::ecs