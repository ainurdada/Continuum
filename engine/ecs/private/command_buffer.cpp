#include <public/command_buffer.h>

#include <public/world.h>

namespace engine::ecs {

void CommandBuffer::requestCommand(CommandData data) {
    _requestedCommands.push_back(data);
}

void CommandBuffer::clear() noexcept {
    _requestedCommands.clear();
    for (auto& c : _commandPayloadStorages) {
        if (c) {
            c->clear();
        }
    }
}

void Commands::destroy(Entity entity) {
    _world.validateCommitState();
    CommandBuffer::CommandData data{.commandKind = CommandBuffer::CommandKind::DestroyEntity, .entity = entity};
    _commandBuffer.requestCommand(data);
}

Entity Commands::create() {
    _world.validateCommitState();
    Entity entity = _world.reserveEntity();
    CommandBuffer::CommandData data{.commandKind = CommandBuffer::CommandKind::CreateEntity, .entity = entity};
    try {
        _commandBuffer.requestCommand(data);
    } catch (...) {
        _world.releaseReservedEntity(entity);
        throw;
    }
    return entity;
}

} // namespace engine::ecs
