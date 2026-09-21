#pragma once

#include <cstdint>

namespace engine::scene {

struct SceneEntityId {
    std::uint64_t value{};

    bool operator==(const SceneEntityId&) const = default;
};

} // namespace engine::scene
