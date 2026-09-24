#include <world.h>

#include "execution_layer.h"
#include <cassert>
#include <limits>
#include <stdexcept>

namespace engine::ecs {

void WorldExecution::awake() {
    _world._executionLayer.awake();
}

void WorldExecution::update(float deltaTime) {
    _world._executionLayer.update(deltaTime);
}

void WorldExecution::destroy() {
    _world._executionLayer.destroy();
}

ExecutionLayer::State WorldExecution::state() const {
    return _world._executionLayer._state;
}
World::IterationScope::IterationScope(std::size_t& counter) : _counter(counter) {
    _counter++;
}

World::IterationScope::~IterationScope() noexcept {
    assert(_counter > 0);
    _counter--;
}

void World::setEntityArchetype(Entity entity, ArchetypeID archetypeID) {
    if (!hasEntity(entity)) {
        return;
    }

    if (_slots[entity.index].archetypeId == archetypeID) {
        return;
    }

    _archetypeRegistry.get(archetypeID).prepareAddEntity();
    setEntityArchetypePrepared(entity, archetypeID);
}

void World::validateNoActiveIteration() {
    if (_activeIterationCount != 0) {
        throw std::logic_error("trying get access to editing world while there is existing active iteration");
    }
}

void World::validateCommitState() const {
    if (_isCommitState) {
        throw std::runtime_error("commiting is active");
    }
}

Entity World::createEntity() {
    if (_commands) {
        return _commands->create();
    }

    validateCommitState();
    validateNoActiveIteration();

    Entity newEntity{0, 0};
    bool fromFreeList = false;
    if (_freeList.size() > 0) {
        std::uint32_t freeIndex = _freeList.back();
        _freeList.pop_back();
        _slots[freeIndex].generation++;
        _slots[freeIndex].state = SlotState::Alive;
        newEntity = Entity(freeIndex, _slots[freeIndex].generation);
        fromFreeList = true;
    } else {
        auto newIndex = _slots.size();
        if (newIndex > std::numeric_limits<std::uint32_t>::max()) {
            throw std::length_error("overload ecs world slot limit");
        }

        prepareFreeListForNewSlot();

        WorldSlot newWorldSlot = WorldSlot{.generation = 0, .state = SlotState::Alive};
        _slots.push_back(newWorldSlot);
        newEntity = Entity(static_cast<std::uint32_t>(newIndex), 0);
    }
    Archetype& zeroArchetype = _archetypeRegistry.get(0);
    try {
        _slots[newEntity.index].rowInArchetype = zeroArchetype.addEntity(newEntity);
        _slots[newEntity.index].archetypeId = 0;
    } catch (...) {
        if (fromFreeList) {
            _slots[newEntity.index].generation--;
            _slots[newEntity.index].state = SlotState::Free;
            _freeList.push_back(newEntity.index);
        } else {
            _slots.pop_back();
        }
        throw;
    }
    return newEntity;
}

bool World::hasEntity(Entity entity) const {
    if (entity.index < _slots.size() && _slots[entity.index].generation == entity.generation && _slots[entity.index].state == SlotState::Alive) {
        return true;
    }
    return false;
}

void World::destroyEntity(Entity entity) {
    validateCommitState();
    if (_commands) {
        _commands->destroy(entity);
        return;
    }
    validateNoActiveIteration();
    destroyEntityImmediate(entity);
}

const IStash* World::findStash(ComponentID id) const noexcept {
    if (id >= _stashes.size()) {
        return nullptr;
    }
    return _stashes[id].get();
}

std::size_t World::stashCount() const noexcept {
    return _stashes.size();
}

const QueryCacheEntry& World::getQueryEntry(QueryID id) const {
    return _queryCache.getEntry(id);
}

Query World::createQuery(const QueryFilter& filter) {
    validateNoActiveIteration();

    QueryID id = _queryCache.getOrCreateEntry(_archetypeRegistry, filter);
    return Query(this, id);
}

bool World::destroyEntityImmediate(Entity entity) {
    if (!hasEntity(entity)) {
        return false;
    }

    if (_slots[entity.index].generation != std::numeric_limits<std::uint32_t>::max()) {
        _freeList.push_back(entity.index);
    }

    Archetype& archetype = _archetypeRegistry.get(_slots[entity.index].archetypeId);

    for (auto componentId : archetype.signature().get()) {
        bool removeResult = _stashes[componentId]->removeStorage(entity);
        assert(removeResult);
        notifyComponentChanged(entity, componentId, false, nullptr);
    }

    std::size_t row = _slots[entity.index].rowInArchetype;
    std::optional<Entity> swappedEntity = archetype.removeEntity(row);
    if (swappedEntity) {
        _slots[swappedEntity.value().index].rowInArchetype = row;
    }

    _slots[entity.index].state = SlotState::Free;

    return true;
}

struct CommandGroup {
    Entity entity;
    std::vector<std::size_t> commandIndices{};
    ComponentSignature finalSignature;
    std::unordered_map<ComponentID, std::size_t> pendingAdds{};
    bool destroyRequested = false;
    bool createRequested = false;
};

void World::applyCommandBuffer(const CommandBuffer& commandBuffer) {
    validateNoActiveIteration();

    std::vector<CommandGroup> groups{};
    std::unordered_map<Entity, std::size_t, EntityHash> entityGroupIndexMap{};

    // combine commandss by entities
    for (std::size_t i = 0; i < commandBuffer._requestedCommands.size(); i++) {
        auto& command = commandBuffer._requestedCommands[i];
        if (!entityGroupIndexMap.contains(command.entity)) {
            groups.push_back(CommandGroup{.entity = command.entity});
            entityGroupIndexMap.emplace(command.entity, groups.size() - 1);
        }
        groups[entityGroupIndexMap.at(command.entity)].commandIndices.push_back(i);
    }

    // build new signature
    for (auto& group : groups) {
        if (hasEntity(group.entity)) {
            group.finalSignature = _archetypeRegistry.get(_slots[group.entity.index].archetypeId).signature();
        } else if (!hasReservedEntity(group.entity)) {
            continue;
        }
        for (auto& commandIndex : group.commandIndices) {
            auto& command = commandBuffer._requestedCommands[commandIndex];

            if (hasReservedEntity(group.entity) && !group.createRequested && command.commandKind != CommandBuffer::CommandKind::CreateEntity) {
                continue;
            }
            switch (command.commandKind) {
            case CommandBuffer::CommandKind::DestroyEntity:
                group.destroyRequested = true;
                group.pendingAdds.clear();
                break;
            case CommandBuffer::CommandKind::RemoveEntityComponent:
                group.finalSignature.remove(command.componentId);
                if (group.pendingAdds.contains(command.componentId)) {
                    group.pendingAdds.erase(command.componentId);
                }
                break;
            case CommandBuffer::CommandKind::AddEntityComponent:
                if (group.finalSignature.add(command.componentId)) {
                    group.pendingAdds[command.componentId] = commandIndex;
                }
                break;
            case CommandBuffer::CommandKind::CreateEntity:
                group.createRequested = true;
                break;
            }

            if (group.destroyRequested) {
                break;
            }
        }
    }

    for (auto& group : groups) {
        bool isEntityReserved = false;
        if (hasEntity(group.entity)) {
            if (group.destroyRequested) {
                destroyEntityImmediate(group.entity);
                continue;
            }
        } else if (hasReservedEntity(group.entity) && group.createRequested) {
            if (group.destroyRequested) {
                releaseReservedEntity(group.entity);
                continue;
            }
            isEntityReserved = true;
        } else {
            continue;
        }
        for (auto& [componentId, commandIndex] : group.pendingAdds) {
            if (componentId >= _stashes.size()) {
                throw std::logic_error("component id is out of stashes array");
            }
            if (!_stashes[componentId]) {
                throw std::logic_error("stash is not exist");
            }
            if (componentId >= commandBuffer._commandPayloadStorages.size()) {
                throw std::logic_error("component id is out of command payloads storage array");
            }
            if (!commandBuffer._commandPayloadStorages[componentId]) {
                throw std::logic_error("array of value is not exist");
            }
            if (commandBuffer._requestedCommands[commandIndex].payloadIndex >= commandBuffer._commandPayloadStorages[componentId]->size()) {
                throw std::logic_error("payload index is out of values array");
            }
        }

        ArchetypeID originAtchetypeId = 0;
        if (!isEntityReserved) {
            originAtchetypeId = _slots[group.entity.index].archetypeId;
        }

        _queryCache.prepareAddArchetype(group.finalSignature);
        ArchetypeLookupResult lookupResult = _archetypeRegistry.getOrCreate(group.finalSignature);
        if (lookupResult.created) {
            _queryCache.onArchetypeCreated(_archetypeRegistry.get(lookupResult.id));
        }

        Archetype& targetArchetype = _archetypeRegistry.get(lookupResult.id);
        targetArchetype.prepareAddEntity();

        for (auto& [componentId, _] : group.pendingAdds) {
            _stashes[componentId]->prepareAddStorage(group.entity);
        }

        if (!isEntityReserved) {
            for (auto& componentId : _archetypeRegistry.get(originAtchetypeId).signature().get()) {
                if (!targetArchetype.signature().has(componentId) || group.pendingAdds.contains(componentId)) {
                    _stashes[componentId]->removeStorage(group.entity);
                    notifyComponentChanged(group.entity, componentId, false, nullptr);
                }
            }
        }

        for (auto& [componentId, index] : group.pendingAdds) {
            void* component = commandBuffer._commandPayloadStorages[componentId]->getValue(commandBuffer._requestedCommands[index].payloadIndex);
            void* componentAddress = _stashes[componentId]->addPreparedStorage(group.entity, component);
            notifyComponentChanged(group.entity, componentId, true, componentAddress);
        }

        if (!isEntityReserved) {
            setEntityArchetypePrepared(group.entity, targetArchetype.id());
        } else {
            commitReservedEntityPrepared(group.entity, targetArchetype.id());
        }
    }
}

bool World::removeEntityComponentImmediate(Entity entity, ComponentID componentId) {
    if (!hasEntity(entity)) {
        return false;
    }
    if (componentId >= _stashes.size()) {
        return false;
    }

    const Archetype& oldArchetype = _archetypeRegistry.get(_slots[entity.index].archetypeId);
    ComponentSignature signature = oldArchetype.signature();
    if (!signature.remove(componentId)) {
        return false;
    }

    _queryCache.prepareAddArchetype(signature);

    ArchetypeLookupResult lookupResult = _archetypeRegistry.getOrCreate(signature);
    if (lookupResult.created) {
        _queryCache.onArchetypeCreated(_archetypeRegistry.get(lookupResult.id));
    }
    setEntityArchetype(entity, lookupResult.id);

    bool removeStorageResult = _stashes[componentId]->removeStorage(entity);
    assert(removeStorageResult);

    return removeStorageResult;
}

void World::setEntityArchetypePrepared(Entity entity, ArchetypeID desiredArchetypeId) noexcept {
    if (!hasEntity(entity)) {
        return;
    }

    ArchetypeID oldId = _slots[entity.index].archetypeId;

    if (oldId == desiredArchetypeId) {
        return;
    }

    std::size_t oldRow = _slots[entity.index].rowInArchetype;
    std::size_t newRow = _archetypeRegistry.get(desiredArchetypeId).addPreparedEntity(entity);

    Archetype& oldArchetype = _archetypeRegistry.get(oldId);
    std::optional<Entity> swappedEntity = oldArchetype.removeEntity(oldRow);
    if (swappedEntity) {
        _slots[swappedEntity.value().index].rowInArchetype = oldRow;
    }

    _slots[entity.index].archetypeId = desiredArchetypeId;
    _slots[entity.index].rowInArchetype = newRow;
}

Entity World::reserveEntity() {
    if (_freeList.size() > 0) {
        std::uint32_t freeIndex = _freeList.back();

        assert(freeIndex < _slots.size());
        assert(_slots[freeIndex].state == SlotState::Free);
        assert(_slots[freeIndex].generation < std::numeric_limits<std::uint32_t>::max());

        _freeList.pop_back();
        _slots[freeIndex].generation++;
        _slots[freeIndex].state = SlotState::Reserved;
        return Entity(freeIndex, _slots[freeIndex].generation);
    }

    auto newIndex = _slots.size();
    if (newIndex > std::numeric_limits<std::uint32_t>::max()) {
        throw std::length_error("overload ecs world slot limit");
    }

    prepareFreeListForNewSlot();

    WorldSlot newWorldSlot = WorldSlot{.generation = 0, .state = SlotState::Reserved};
    _slots.push_back(newWorldSlot);
    return Entity(static_cast<std::uint32_t>(newIndex), 0);
}

void World::prepareFreeListForNewSlot() {
    if (_freeList.capacity() < _slots.size() + 1) {
        auto newCapacity = 2 * _freeList.capacity();
        if (newCapacity == 0) {
            newCapacity = 1;
        }
        _freeList.reserve(newCapacity);
    }
}

bool World::releaseReservedEntity(Entity entity) noexcept {
    if (hasReservedEntity(entity)) {
        assert(_freeList.size() != _freeList.capacity());
        if (_slots[entity.index].generation != std::numeric_limits<std::uint32_t>::max()) {
            _freeList.push_back(entity.index);
        }
        _slots[entity.index].state = SlotState::Free;
        return true;
    }
    return false;
}

void World::releaseReservations(const CommandBuffer& commandBuffer) noexcept {
    for (auto command : commandBuffer._requestedCommands) {
        if (command.commandKind == CommandBuffer::CommandKind::CreateEntity) {
            releaseReservedEntity(command.entity);
        }
    }
}

bool World::hasReservedEntity(Entity entity) const noexcept {
    return entity.index < _slots.size() && entity.generation == _slots[entity.index].generation && _slots[entity.index].state == SlotState::Reserved;
}

void World::commitReservedEntityPrepared(Entity entity, ArchetypeID archetypeId) noexcept {
    assert(hasReservedEntity(entity));
    assert(archetypeId < _archetypeRegistry.size());

    _slots[entity.index].rowInArchetype = _archetypeRegistry.get(archetypeId).addPreparedEntity(entity);
    _slots[entity.index].archetypeId = archetypeId;
    _slots[entity.index].state = SlotState::Alive;
}

QueryBuilder World::query() {
    return QueryBuilder(*this);
}

void World::commit() {
    validateCommitState();
    validateNoActiveIteration();
    if (!_commands) {
        return;
    }
    _isCommitState = true;
    try {
        applyCommandBuffer(_commands->_commandBuffer);
        releaseReservations(_commands->_commandBuffer);
        _commands->_commandBuffer.clear();
        _isCommitState = false;
    } catch (...) {
        releaseReservations(_commands->_commandBuffer);
        _commands->_commandBuffer.clear();
        _isCommitState = false;
        throw;
    }
}

SystemGroup& World::createSystemGroup() {
    return _executionLayer.createGroup();
}

void World::notifyComponentChanged(Entity entity, ComponentID componentId, bool added, void* componentData) {
    ComponentAddInfo info{
        .entity = entity,
        .componentId = componentId,
        .componentData = componentData,
        .added = added,
    };

    for (auto& fn : _bindedOnComponentAdd) {
        fn(info);
    }
}

std::size_t World::bindOnComponentChanged(OnComponentAddFn func) {
    auto newIndex = _bindedOnComponentAdd.size();
    _bindedOnComponentAdd.push_back(func);
    _componentAddBindingsHandlers.emplace(_componentAddBinderCounter, newIndex);
    _componentAddBinderCounter++;
    return _componentAddBinderCounter - 1;
}

void World::unbindOnComponentChanged(std::size_t handle) {
    auto currentIndex = _componentAddBindingsHandlers.at(handle);
    _componentAddBindingsHandlers.erase(handle);
    auto lastIndex = _bindedOnComponentAdd.size() - 1;
    for (auto& [k, v] : _componentAddBindingsHandlers) {
        if (v == lastIndex) {
            v = currentIndex;
        }
    }
    _bindedOnComponentAdd[currentIndex] = std::move(_bindedOnComponentAdd[lastIndex]);
    _bindedOnComponentAdd.pop_back();
}

} // namespace engine::ecs
