#pragma once

#include <math/public/g_math.h>
#include <reflection/public/markers.h>

namespace engine::scene {

OBJECT(Component, DisplayName("Game Camera"))
struct Camera {

    // vertical fov in radians
    FIELD(ShowInInspector, DisplayName("Vertical FOV"))
    float verticalFov = math::pi<float>() / 3.0f;

    // meters to near plane
    FIELD(ShowInInspector, Step(0.01f), Min(0.1))
    float nearPlane = 0.1f;

    // meters to far plane
    FIELD(ShowInInspector, Step(1))
    float farPlane = 100.0f;

};

} // namespace engine::scene
