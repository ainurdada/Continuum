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

} // namespace engine::reflection::serialization
