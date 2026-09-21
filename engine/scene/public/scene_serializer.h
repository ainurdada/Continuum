#pragma once

#include <expected>
#include <filesystem>
#include <string>

#include <ecs/ecs.h>

namespace engine::scene::serialization {

class ISceneSerializer {
  public:
    virtual std::expected<void, std::string> serialize(ecs::World& world, std::filesystem::path sceneFile) = 0;
    virtual std::expected<void, std::string> deserialize(ecs::World& world, std::filesystem::path sceneFile) = 0;
};

} // namespace engine::scene::serialization