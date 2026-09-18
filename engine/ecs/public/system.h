#pragma once

#include <concepts>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace engine::ecs {

class World;

class ISystem {
  public:
    virtual ~ISystem() = default;
    virtual void awake(World& world) = 0;
    virtual void update(float deltaTime) = 0;
    virtual void destroy() = 0;
};

class SystemGroup {
  private:
    friend class ExecutionLayer;

    std::vector<std::unique_ptr<ISystem>> _systems;

    bool _isConfigured = false;
    std::size_t _numAwakedSystems = 0;

    void awake(World& world);
    void update(float deltaTime);
    void destroy();

  public:
    template <std::derived_from<ISystem> T> void registerSystem();
};

template <std::derived_from<ISystem> T> inline void SystemGroup::registerSystem() {
    if (_isConfigured) {
        throw std::runtime_error("register system is not allowed after system group is configured");
    }
    std::unique_ptr<ISystem> pSystem = static_cast<std::unique_ptr<ISystem>>(std::make_unique<T>());
    _systems.push_back(std::move(pSystem));
}

} // namespace engine::ecs