#pragma once

#include <cstddef>
#include <cstdint>

namespace engine::ecs {

// Object ID
struct Entity {
    std::uint32_t index;
    std::uint32_t generation;

    Entity(std::uint32_t i, std::uint32_t g) : index(i), generation(g) {};
    Entity() = delete;

    bool operator==(const Entity& other) const = default;
};

struct EntityHash {
    std::size_t operator()(const Entity& entity) const noexcept;
};

} // namespace engine::ecs