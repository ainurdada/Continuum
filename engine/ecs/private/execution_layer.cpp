#include <execution_layer.h>
#include <execution_layer.hpp>

#include <stdexcept>

#include <world.h>

namespace engine::ecs {

SystemGroup& ExecutionLayer::createGroup() {
    if (_state != State::Configuring) {
        throw std::logic_error("creating groups are able unly at configuring");
    }
    _systemGroups.push_back(std::move(std::make_unique<SystemGroup>()));
    return *_systemGroups.back().get();
}

void ExecutionLayer::awake() {
    if (_state != State::Configuring) {
        throw std::logic_error("systems are not able for configuring");
    }
    _world.validateNoActiveIteration();
    _state = State::Initializing;
    try {
        for (auto& systemGroup : _systemGroups) {
            systemGroup.get()->_isConfigured = true;
        }
        for (auto& systemGroup : _systemGroups) {
            execute([&systemGroup, this]() { systemGroup->awake(_world); });
        }
    } catch (...) {
        _state = State::Failed;
        throw;
    }
    _state = State::Ready;
}

void ExecutionLayer::update(float deltaTime) {
    if (_state != State::Ready) {
        throw std::logic_error("systems are not ready");
    }
    _world.validateNoActiveIteration();
    for (auto& systemGroup : _systemGroups) {
        execute([&systemGroup, deltaTime]() { systemGroup->update(deltaTime); });
    }
}

void ExecutionLayer::destroy() {
    if (_state == State::Destroyed) {
        return;
    }
    if (_state != State::Ready && _state != State::Failed) {
        throw std::runtime_error("destroying at wrong execution state");
    }
    if (_world._commands) {
        throw std::runtime_error("destroying systems while commands are processing");
    }
    _world.validateNoActiveIteration();
    _state = State::Destroying;
    try {
        for (auto it = _systemGroups.rbegin(); it != _systemGroups.rend(); it++) {
            execute([&it]() { it->get()->destroy(); });
        }
        _state = State::Destroyed;
    } catch (...) {
        _state = State::Failed;
        throw;
    }
}

} // namespace engine::ecs
