#pragma once

#ifndef ECS_H_INCLUDED
#include "ecs.h"
#endif
#include <cassert>
namespace engine::ecs {

// STASH
template <typename T> inline std::size_t Stash<T>::findDenseIndex(Entity entity) const {
    if (!_world.hasEntity(entity)) {
        return invalidDenseIndex;
    }

    if (static_cast<std::size_t>(entity.index) >= _sparse.size()) {
        return invalidDenseIndex;
    }

    std::size_t denseIndex = _sparse[static_cast<std::size_t>(entity.index)];
    if (denseIndex == invalidDenseIndex) {
        return invalidDenseIndex;
    }

    if (denseIndex >= _denseEntities.size()) {
        return invalidDenseIndex;
    }

    if (entity != _denseEntities[denseIndex]) {
        return invalidDenseIndex;
    }

    return denseIndex;
}

template <typename T> inline bool Stash<T>::addStorage(Entity entity, const T& component) {
    std::size_t entityIndex = static_cast<std::size_t>(entity.index);

    if (_sparse.size() < entityIndex + 1) {
        _sparse.resize(entityIndex + 1, invalidDenseIndex);
    }

    std::size_t denseIndex = _denseEntities.size();

    _denseEntities.push_back(entity);
    try {
        _denseComponents.push_back(component);
    } catch (...) {
        _denseEntities.pop_back();
        throw;
    }

    if (_denseEntities.size() != _denseComponents.size()) {
        throw std::logic_error("dense vectors in Stash are not equal");
    }

    _sparse[entityIndex] = denseIndex;

    if (_denseEntities[_sparse[entityIndex]] != entity) {
        throw std::logic_error("didn't add entity to stash correctly");
    }

    return true;
}
template <typename T> inline bool Stash<T>::removeStorage(Entity entity) noexcept {
    std::size_t targetDenseIndex = findDenseIndex(entity);
    if (targetDenseIndex == invalidDenseIndex) {
        return false;
    }

    if (targetDenseIndex != _denseEntities.size() - 1) {
        Entity lastEntity = _denseEntities.back();
        _denseEntities[targetDenseIndex] = std::move(_denseEntities.back());
        _denseComponents[targetDenseIndex] = std::move(_denseComponents.back());
        _sparse[lastEntity.index] = targetDenseIndex;
    }

    _denseEntities.pop_back();
    _denseComponents.pop_back();
    _sparse[entity.index] = invalidDenseIndex;

    return true;
}

template <typename T> inline void Stash<T>::prepareAddStorage(Entity entity) {
    std::size_t entityIndex = static_cast<std::size_t>(entity.index);
    if (_sparse.size() < entityIndex + 1) {
        _sparse.resize(entityIndex + 1, invalidDenseIndex);
    }
    if (_sparse[entityIndex] == invalidDenseIndex) {
        if (_denseEntities.size() == _denseEntities.capacity()) {
            auto newCapacity = 2 * _denseEntities.capacity();
            if (newCapacity == 0) {
                newCapacity = 1;
            }
            _denseEntities.reserve(newCapacity);
        }
        if (_denseComponents.size() == _denseComponents.capacity()) {
            auto newCapacity = 2 * _denseComponents.capacity();
            if (newCapacity == 0) {
                newCapacity = 1;
            }
            _denseComponents.reserve(newCapacity);
        }
    }
}

template <typename T> inline void Stash<T>::addPreparedStorage(Entity entity, void* component) noexcept {
    assert(_world.hasEntity(entity) || _world.hasReservedEntity(entity));
    assert(component);
    assert(_denseEntities.size() == _denseComponents.size());
    assert(_denseEntities.size() < _denseEntities.capacity());
    assert(_denseComponents.size() < _denseComponents.capacity());
    assert(entity.index < _sparse.size());
    assert(_sparse[entity.index] == invalidDenseIndex);

    auto newIndex = _denseEntities.size();
    T* typedComponent = static_cast<T*>(component);
    _denseEntities.push_back(entity);
    _denseComponents.push_back(std::move(*typedComponent));
    _sparse[entity.index] = newIndex;
}

template <typename T> inline ComponentID Stash<T>::componentId() const noexcept {
    return _componentId;
}

template <typename T> inline std::type_index Stash<T>::nativeTypeKey() const noexcept {
    return typeid(T);
}

template <typename T> inline bool Stash<T>::has(Entity entity) const {
    return findDenseIndex(entity) != invalidDenseIndex;
}

template <typename T> inline void Stash<T>::add(Entity entity, const T& component) {
    _world.addEntityComponent(entity, *this, component);
}

template <typename T> inline const T* Stash<T>::get(Entity entity) const {
    std::size_t denseIndex = findDenseIndex(entity);
    if (denseIndex == invalidDenseIndex) {
        return nullptr;
    }

    return &_denseComponents[denseIndex];
}

template <typename T> inline T* Stash<T>::getMut(Entity entity) {
    std::size_t denseIndex = findDenseIndex(entity);
    if (denseIndex == invalidDenseIndex) {
        return nullptr;
    }

    return &_denseComponents[denseIndex];
}

template <typename T> inline void Stash<T>::remove(Entity entity) {
    _world.removeEntityComponent(entity, *this);
}

template <typename T> inline std::size_t Stash<T>::size() const noexcept {
    return _denseEntities.size();
}
// ~STASH

// COMMANDS
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

// ~COMMANDS

// EXECUTION
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
// ~EXECUTION

// WORLD
template <typename T> inline void World::addEntityComponent(Entity entity, Stash<T>& stash, const T& component) {
    validateCommitState();

    if (_commands) {
        _commands->add(entity, component);
        return;
    }

    validateNoActiveIteration();

    if (!hasEntity(entity)) {
        return;
    }
    if (stash.has(entity)) {
        return;
    }

    const Archetype& oldArchetype = _archetypeRegistry.get(_slots[entity.index].archetypeId);
    ComponentSignature signature = oldArchetype.signature();
    if (!signature.add(stash.componentId())) {
        return;
    }

    _queryCache.prepareAddArchetype(signature);

    ArchetypeLookupResult lookupResult = _archetypeRegistry.getOrCreate(signature);
    if (lookupResult.created) {
        _queryCache.onArchetypeCreated(_archetypeRegistry.get(lookupResult.id));
    }

    if (!stash.addStorage(entity, component)) {
        return;
    }
    try {
        setEntityArchetype(entity, lookupResult.id);
    } catch (...) {
        stash.removeStorage(entity);
        throw;
    }

    return;
}

template <typename T> inline void World::removeEntityComponent(Entity entity, Stash<T>& stash) {
    validateCommitState();

    if (_commands) {
        _commands->remove<T>(entity);
        return;
    }

    validateNoActiveIteration();

    removeEntityComponentImmediate(entity, stash.componentId());
}

template <typename Visitor> inline bool World::visitComponents(Entity entity, Visitor&& visitor) const {
    validateCommitState();

    if (!hasEntity(entity)) {
        return false;
    }

    IterationScope scope(_activeIterationCount);

    const auto& archetype = _archetypeRegistry.get(_slots[entity.index].archetypeId);
    for (auto id : archetype.signature().get()) {
        const IStash& stash = *_stashes[id];
        visitor(stash);
    }

    return true;
}

template <typename Visitor> inline bool World::visitComponentsMut(Entity entity, Visitor&& visitor) {
    validateCommitState();

    if (!hasEntity(entity)) {
        return false;
    }

    IterationScope scope(_activeIterationCount);

    const auto& archetype = _archetypeRegistry.get(_slots[entity.index].archetypeId);
    for (auto id : archetype.signature().get()) {
        IStash& stash = *_stashes[id];
        visitor(stash);
    }

    return true;
}

template <typename T> Stash<T>& World::getStash() {
    std::type_index typeIndex = typeid(T);
    if (_componentRegistryMap.contains(typeIndex)) {
        IStash* result = _stashes[_componentRegistryMap[typeIndex]].get();
        return *static_cast<Stash<T>*>(result);
    }
    if (_stashes.size() > std::numeric_limits<ComponentID>::max()) {
        throw std::length_error("overload stash max lenght");
    }
    ComponentID newId = static_cast<ComponentID>(_stashes.size());
    std::unique_ptr<Stash<T>> newStash{new Stash<T>(*this, newId)};
    auto result = newStash.get();
    _stashes.push_back(std::move(newStash));

    try {
        auto [iterator, inserted] = _componentRegistryMap.emplace(typeIndex, newId);

        if (!inserted) {
            _stashes.pop_back();

            IStash* existing = _stashes[iterator->second].get();
            return *static_cast<Stash<T>*>(existing);
        }
    } catch (...) {
        _stashes.pop_back();
        throw;
    }

    return *result;
}
// ~WORLD

template <typename T> inline const Stash<T>* World::findStash() const {
    std::type_index typeIndex = typeid(T);
    const auto& it = _componentRegistryMap.find(typeIndex);
    if (it == _componentRegistryMap.end()) {
        return nullptr;
    }
    const IStash* result = _stashes[it->second].get();
    return static_cast<const Stash<T>*>(result);
}

} // namespace engine::ecs
