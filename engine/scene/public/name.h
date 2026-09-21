#pragma once

#include <string>

#include <reflection/public/markers.h>

namespace engine::scene {

OBJECT(Component, Key("continuum.scene.name"))
struct Name {
    FIELD(ShowInInspector, Key("name"))
    std::string value{};
};

} // namespace engine::scene
