#pragma once

#ifndef ECS_H_INCLUDED
#include "ecs/ecs.h"
#endif

namespace engine::ecs {

template <typename T> void Commands::remove(Entity entity) {
    _world.validateCommitState();
    Stash<T>& stash = _world.getStash<T>();
    CommandBuffer::CommandData data{.commandKind = CommandBuffer::CommandKind::RemoveEntityComponent, .entity = entity, .componentId = stash.componentId()};
    _commandBuffer.requestCommand(data);
}

template <typename T> inline void Commands::add(Entity entity, T component) {
    _world.validateCommitState();
    auto& stash = _world.getStash<T>();
    ComponentID componentId = stash.componentId();
    CommandPayloadStorage<T>& payloadStorage = _commandBuffer.getOrCreateCommandPayloadStorage<T>(componentId);
    auto index = payloadStorage.add(std::move(component));
    try {
        _commandBuffer.requestCommand(CommandBuffer::CommandData{.commandKind = CommandBuffer::CommandKind::AddEntityComponent, .entity = entity, .componentId = componentId, .payloadIndex = index});
    } catch (...) {
        payloadStorage.pop();
        throw;
    }
}

} // namespace engine::ecs