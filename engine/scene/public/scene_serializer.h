#pragma once

#include <expected>
#include <filesystem>
#include <string>

#include <ecs/ecs.h>

namespace engine::reflection {
class TypeRegistry;
}
namespace engine::ecs_reflection {
class WorldReflectionContext;
class ComponentBindingRegistry;
} // namespace engine::ecs_reflection
namespace engine::reflection::serialization {
class JsonSerializationRegistry;
}

namespace engine::scene::serialization {

class ISceneSerializer {
  public:
    virtual std::expected<void, std::string> serialize(ecs::World& world, std::filesystem::path sceneFile) = 0;
    virtual std::expected<void, std::string> deserialize(ecs::World& world, std::filesystem::path sceneFile) = 0;
};

namespace json {
class SceneSerializerJson : public ISceneSerializer {
    const engine::reflection::TypeRegistry& _typeReg;
    const engine::reflection::serialization::JsonSerializationRegistry& _policies;
    const engine::ecs_reflection::ComponentBindingRegistry& _bindings;

  public:
    SceneSerializerJson(engine::reflection::TypeRegistry& typeRegistry, engine::reflection::serialization::JsonSerializationRegistry& policies, engine::ecs_reflection::ComponentBindingRegistry& bindings) : _typeReg(typeRegistry), _policies(policies), _bindings(bindings) {}

    std::expected<void, std::string> serialize(ecs::World& world, std::filesystem::path sceneFile) override;
    std::expected<void, std::string> deserialize(ecs::World& world, std::filesystem::path sceneFile) override;
};
} // namespace json

} // namespace engine::scene::serialization