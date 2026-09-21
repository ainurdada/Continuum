#pragma once

#include <engine.h>

namespace demo {

OBJECT(Component)
struct Spin {
    enum class TimeSource {
        Simulation,
        Real
    };

    // Euler angles (rad/sec)
    FIELD(ShowInInspector)
    float test;

    Vec3f rotationRate{};
    TimeSource timeSource = TimeSource::Simulation;
};

} // namespace demo
