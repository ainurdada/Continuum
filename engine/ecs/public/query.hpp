#pragma once

#ifndef ECS_H_INCLUDED
#include "ecs.h"
#endif

namespace engine::ecs {

template <typename T> QueryBuilder& QueryBuilder::with() {
    Stash<T>& stash = _world->getStash<T>();
    if (!_filter.require(stash.componentId()) && _filter.none().has(stash.componentId())) {
        throw std::logic_error("failed to add required component to query: it already exists in excluded components");
    }
    return *this;
}

template <typename T> inline QueryBuilder& QueryBuilder::without() {
    Stash<T>& stash = _world->getStash<T>();
    if (!_filter.exclude(stash.componentId()) && _filter.all().has(stash.componentId())) {
        throw std::logic_error("failed to add excluded component to query: it already exists in required components");
    }
    return *this;
}

} // namespace engine::ecs