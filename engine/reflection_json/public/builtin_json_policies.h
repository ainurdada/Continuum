#pragma once

#include <expected>
#include <string>

#include <nlohmann/json.hpp>

#include "json_serialization_registry.h"

#include <scene/public/transform.h>
#include <scene/public/mesh_renderer.h>

namespace engine::reflection::serialization {

bool registerBuiltinJsonPolicies(JsonSerializationRegistry& reg);

struct BoolJsonPolicy {
    static std::expected<nlohmann::json, std::string> serialize(const bool& value);
    static std::expected<void, std::string> deserialize(const nlohmann::json& json, bool& value);
};

struct StringJsonPolicy {
    static std::expected<nlohmann::json, std::string> serialize(const std::string& value);
    static std::expected<void, std::string> deserialize(const nlohmann::json& json, std::string& value);
};

struct IntJsonPolicy {
    static std::expected<nlohmann::json, std::string> serialize(const int& value);
    static std::expected<void, std::string> deserialize(const nlohmann::json& json, int& value);
};

struct DoubleJsonPolicy {
    static std::expected<nlohmann::json, std::string> serialize(const double& value);
    static std::expected<void, std::string> deserialize(const nlohmann::json& json, double& value);
};

struct FloatJsonPolicy {
    static std::expected<nlohmann::json, std::string> serialize(const float& value);
    static std::expected<void, std::string> deserialize(const nlohmann::json& json, float& value);
};

struct TransformJsonPolicy {
    static std::expected<nlohmann::json, std::string> serialize(const engine::scene::Transform& value);
    static std::expected<void, std::string> deserialize(const nlohmann::json& json, engine::scene::Transform& value);
};

struct MeshRendererJsonPolicy {
    static std::expected<nlohmann::json, std::string> serialize(const engine::scene::MeshRenderer& value);
    static std::expected<void, std::string> deserialize(const nlohmann::json& json, engine::scene::MeshRenderer& value);
};

} // namespace engine::reflection::serialization
