#include <public/system.h>

namespace engine::ecs {

void SystemGroup::awake(World& world) {
    for (auto& system : _systems) {
        system->awake(world);
        _numAwakedSystems++;
    }
}

void SystemGroup::update(float deltaTime) {
    for (auto& system : _systems) {
        system->update(deltaTime);
    }
}

void SystemGroup::destroy() {
    while (_numAwakedSystems > 0) {
        _systems[--_numAwakedSystems]->destroy();
    }
}

} // namespace engine::ecs
