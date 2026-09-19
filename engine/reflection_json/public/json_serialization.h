#pragma once

#include <expected>
#include <string>

#include <nlohmann/json.hpp>

#include <json_serialization_registry.h>
#include <object_view.h>

namespace engine::reflection::serialization {

std::expected<nlohmann::json, std::string> serializeValue(const ObjectView& view, const engine::reflection::serialization::JsonSerializationRegistry& policies);

std::expected<void, std::string> deserializeValue(const nlohmann::json& json, const ObjectView& view, const engine::reflection::serialization::JsonSerializationRegistry& policies);

} // namespace engine::reflection::serialization
