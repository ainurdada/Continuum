#include <builtin_json_policies.h>

#include <cmath>

namespace engine::reflection::serialization {

bool registerBuiltinJsonPolicies(JsonSerializationRegistry& reg) {
    if (!reg.findPolicy(typeid(bool)) && !reg.registerPolicy<bool, BoolJsonPolicy>()) {
        return false;
    }

    if (!reg.findPolicy(typeid(std::string)) && !reg.registerPolicy<std::string, StringJsonPolicy>()) {
        return false;
    }

    if (!reg.findPolicy(typeid(int)) && !reg.registerPolicy<int, IntJsonPolicy>()) {
        return false;
    }

    if (!reg.findPolicy(typeid(double)) && !reg.registerPolicy<double, DoubleJsonPolicy>()) {
        return false;
    }

    if (!reg.findPolicy(typeid(float)) && !reg.registerPolicy<float, FloatJsonPolicy>()) {
        return false;
    }

    if (!reg.findPolicy(typeid(engine::scene::Transform)) && !reg.registerPolicy<engine::scene::Transform, TransformJsonPolicy>()) {
        return false;
    }

    if (!reg.findPolicy(typeid(engine::scene::MeshRenderer)) && !reg.registerPolicy<engine::scene::MeshRenderer, MeshRendererJsonPolicy>()) {
        return false;
    }

    return true;
}

std::expected<nlohmann::json, std::string> BoolJsonPolicy::serialize(const bool& value) {
    return nlohmann::json(value);
}

std::expected<void, std::string> BoolJsonPolicy::deserialize(const nlohmann::json& json, bool& value) {
    if (!json.is_boolean()) {
        return std::unexpected("Json is not boolean");
    }
    value = json.get<bool>();
    return {};
}

std::expected<nlohmann::json, std::string> StringJsonPolicy::serialize(const std::string& value) {
    return nlohmann::json(value);
}

std::expected<void, std::string> StringJsonPolicy::deserialize(const nlohmann::json& json, std::string& value) {
    if (!json.is_string()) {
        return std::unexpected("Json is not string");
    }
    value = json.get<std::string>();
    return {};
}

std::expected<nlohmann::json, std::string> IntJsonPolicy::serialize(const int& value) {
    return nlohmann::json(value);
}

std::expected<void, std::string> IntJsonPolicy::deserialize(const nlohmann::json& json, int& value) {
    if (json.is_number_integer()) {
        value = json.get<int>();
        return {};
    }
    return std::unexpected("Json is not int");
}

std::expected<nlohmann::json, std::string> DoubleJsonPolicy::serialize(const double& value) {
    if (!std::isfinite(value)) {
        return std::unexpected("Failed to serialized double: infinity value");
    }
    return nlohmann::json(value);
}

std::expected<void, std::string> DoubleJsonPolicy::deserialize(const nlohmann::json& json, double& value) {
    if (json.is_number()) {
        value = json.get<double>();
        return {};
    }
    return std::unexpected("Json is not double");
}

std::expected<nlohmann::json, std::string> FloatJsonPolicy::serialize(const float& value) {
    if (!std::isfinite(value)) {
        return std::unexpected("Failed to serialized float: infinity value");
    }
    return nlohmann::json(value);
}

std::expected<void, std::string> FloatJsonPolicy::deserialize(const nlohmann::json& json, float& value) {
    if (json.is_number()) {
        value = json.get<float>();
        return {};
    }
    return std::unexpected("Json is not float");
}

std::expected<nlohmann::json, std::string> TransformJsonPolicy::serialize(const engine::scene::Transform& value) {
    nlohmann::json json = nlohmann::json::object();

    nlohmann::json position = nlohmann::json::array();
    for (int i = 0; i < 3; i++) {
        auto res = FloatJsonPolicy::serialize(value.position[i]);
        if (!res) {
            return std::unexpected(res.error());
        }
        position.push_back(res.value());
    }

    nlohmann::json rotation = nlohmann::json::array();
    for (int i = 0; i < 3; i++) {
        auto res = FloatJsonPolicy::serialize(value.rotation[i]);
        if (!res) {
            return std::unexpected(res.error());
        }
        rotation.push_back(res.value());
    }

    nlohmann::json scale = nlohmann::json::array();
    for (int i = 0; i < 3; i++) {
        auto res = FloatJsonPolicy::serialize(value.scale[i]);
        if (!res) {
            return std::unexpected(res.error());
        }
        scale.push_back(res.value());
    }

    json.emplace("position", position);
    json.emplace("rotation", rotation);
    json.emplace("scale", scale);

    return json;
}

std::expected<void, std::string> TransformJsonPolicy::deserialize(const nlohmann::json& json, engine::scene::Transform& value) {
    if (!json.is_object()) {
        return std::unexpected("Json is not object");
    }
    if (!json.contains("position") || !json.contains("rotation") || !json.contains("scale")) {
        return std::unexpected("Json does not have valid elements");
    }

    auto position = json.at("position");
    auto rotation = json.at("rotation");
    auto scale = json.at("scale");

    if (!position.is_array() || position.size() != 3 || !rotation.is_array() || rotation.size() != 3 || !scale.is_array() || scale.size() != 3) {
        return std::unexpected("Json does not have valid elements");
    }

    engine::scene::Transform transform{};

    for (int i = 0; i < 3; i++) {
        float element;
        auto res = FloatJsonPolicy::deserialize(position[i], element);
        if (!res) {
            return std::unexpected(res.error());
        }
        transform.position[i] = element;
    }

    for (int i = 0; i < 3; i++) {
        float element;
        auto res = FloatJsonPolicy::deserialize(rotation[i], element);
        if (!res) {
            return std::unexpected(res.error());
        }
        transform.rotation[i] = element;
    }

    for (int i = 0; i < 3; i++) {
        float element;
        auto res = FloatJsonPolicy::deserialize(scale[i], element);
        if (!res) {
            return std::unexpected(res.error());
        }
        transform.scale[i] = element;
    }

    value = transform;
    return {};
}

std::expected<nlohmann::json, std::string> MeshRendererJsonPolicy::serialize(const engine::scene::MeshRenderer& value) {
    auto json = nlohmann::json::object();
    switch (value.geometryId) {

    case engine::GeometryId::Cube:
        json.emplace("geometry", "cube");
        break;

    default:
        return std::unexpected("Not supported geometry id");
    }

    return json;
}

std::expected<void, std::string> MeshRendererJsonPolicy::deserialize(const nlohmann::json& json, engine::scene::MeshRenderer& value) {
    if (!json.is_object()) {
        return std::unexpected("Mesh renderer json is not object");
    }
    if (!json.contains("geometry")) {
        return std::unexpected("Mesh renderer json does not have \"geometry\" field");
    }

    std::string geometryId;
    auto geometry = StringJsonPolicy::deserialize(json.at("geometry"), geometryId);
    if (!geometry) {
        return std::unexpected(geometry.error());
    }

    if (geometryId == "cube") {
        value = engine::scene::MeshRenderer{.geometryId = engine::GeometryId::Cube};
        return {};
    } else {
        return std::unexpected("Not supported geometry id");
    }
}

} // namespace engine::reflection::serialization
