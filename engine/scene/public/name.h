#pragma once

#include <string>

#include <reflection/public/markers.h>

namespace engine::scene {

OBJECT(Component)
struct Name {
    FIELD(ShowInInspector)
    std::string value{};
};

} // namespace engine::scene
