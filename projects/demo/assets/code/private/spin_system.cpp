#include <spin_system.h>

void SpinSystem::awake(engine::ecs::World& world) {
    _transforms = &world.getStash<engine::scene::Transform>();
    _spins = &world.getStash<demo::Spin>();
    _spinQuery = world.query().with<engine::scene::Transform>().with<demo::Spin>().build();
}

void SpinSystem::update(float deltaTime) {
    for (auto entity : _spinQuery->view()) {
        auto* transform = _transforms->getMut(entity);
        const auto* spin = _spins->get(entity);

        double deltaSecond = 0;
        switch (spin->timeSource) {
        case demo::Spin::TimeSource::Simulation:
            deltaSecond = deltaTime;
            break;
        case demo::Spin::TimeSource::Real:
            deltaSecond = deltaTime;
            break;
        default:
            break;
        }

        transform->rotation += spin->rotationRate * static_cast<float>(deltaSecond);
        if (transform->rotation.y > math::radians(270)) {
            _spins->remove(entity);
        }
    }
}

void SpinSystem::destroy() {
    _spinQuery = std::nullopt;
    _spins = nullptr;
    _transforms = nullptr;
}
