#pragma once

#ifndef ECS_H_INCLUDED
#include "ecs/ecs.h"
#endif

namespace engine::ecs {

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