#pragma once

#include <expected>
#include <filesystem>
#include <string>

#include <nlohmann/json.hpp>

#include <ecs/ecs.h>

#include <scene/public/scene_serializer.h>

namespace engine::reflection {
class TypeRegistry;
}
namespace engine::ecs_reflection {
class ComponentBindingRegistry;
} // namespace engine::ecs_reflection
namespace engine::reflection::serialization {
class JsonSerializationRegistry;
}

namespace engine::scene::serialization::json {

class SceneSerializerJson : public ISceneSerializer {
    const engine::reflection::TypeRegistry& _typeReg;
    const engine::reflection::serialization::JsonSerializationRegistry& _policies;
    const engine::ecs_reflection::ComponentBindingRegistry& _bindings;

  public:
    SceneSerializerJson(engine::reflection::TypeRegistry& typeRegistry, engine::reflection::serialization::JsonSerializationRegistry& policies, engine::ecs_reflection::ComponentBindingRegistry& bindings) : _typeReg(typeRegistry), _policies(policies), _bindings(bindings) {}

    std::expected<nlohmann::json, std::string> worldToJson(ecs::World& world);
    std::expected<void, std::string> jsonToWorld(ecs::World& world, const nlohmann::json& jsonScene);

    std::expected<void, std::string> serialize(ecs::World& world, std::filesystem::path sceneFile) override;
    std::expected<void, std::string> deserialize(ecs::World& world, std::filesystem::path sceneFile) override;
};

} // namespace engine::scene::serialization::json