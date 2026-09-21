#include <entity.h>

#include <functional>

namespace engine::ecs {

std::size_t EntityHash::operator()(const Entity& entity) const noexcept {
    std::size_t indexHesh = std::hash<std::uint32_t>{}(entity.index) + std::size_t{0x9e3779b9U};
    std::size_t generationHesh = std::hash<std::uint32_t>{}(entity.generation) + std::size_t{0x9e3779b9U} + (indexHesh << 6U) + (indexHesh >> 2U);
    return indexHesh ^ generationHesh;
}

} // namespace engine::ecs