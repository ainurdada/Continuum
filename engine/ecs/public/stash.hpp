#pragma once

#ifndef ECS_H_INCLUDED
#include "ecs/ecs.h"
#endif

#include <cassert>
#include <stdexcept>
#include <utility>

namespace engine::ecs {

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

template <typename T> inline void* Stash<T>::addPreparedStorage(Entity entity, void* component) noexcept {
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

    return &_denseComponents.back();
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

} // namespace engine::ecs