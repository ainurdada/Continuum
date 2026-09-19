#include <json_serialization.h>

#include <cmath>

std::expected<nlohmann::json, std::string> engine::reflection::serialization::serializeValue(const ObjectView& view, const engine::reflection::serialization::JsonSerializationRegistry& policies) {
    auto policy = policies.findPolicy(view.type()->nativeTypeKey);
    if (policy) {
        return policy->serialize(view);
    }
    switch (view.type()->category) {
    case TypeCategory::Object: {
        nlohmann::json obj = nlohmann::json::object();
        for (auto& field : view.type()->fields) {
            auto rField = view.readField(field);
            if (!rField) {
                return std::unexpected(std::string(field.key) + ": Failed to read field");
            }
            auto serializedValue = serializeValue(rField.value(), policies);
            if (!serializedValue) {
                return std::unexpected(std::string(field.key) + ": " + serializedValue.error());
            }
            obj.emplace(field.key, serializedValue.value());
        }
        return obj;
    } break;

    default:
        return std::unexpected("Not supported type");
    }
}

std::expected<void, std::string> engine::reflection::serialization::deserializeValue(const nlohmann::json& json, const ObjectView& view, const engine::reflection::serialization::JsonSerializationRegistry& policies) {
    if (!view.canWrite()) {
        return std::unexpected("Failed to deserialized: can't write to view");
    }

    auto policy = policies.findPolicy(view.type()->nativeTypeKey);
    if (policy) {
        return policy->deserialize(json, view);
    }

    if (json.is_object() && view.type()->category == TypeCategory::Object) {
        std::size_t count = 0;
        for (auto& field : view.type()->fields) {
            if (!json.contains(field.key)) {
                return std::unexpected("Failed to deserialize: field " + std::string(field.key) + " not found");
            }
            count++;
        }

        if (count != json.size()) {
            return std::unexpected("Failed to deserialize: type " + std::string(view.type()->key) + " has not correct serialized fields. required " + std::to_string(json.size()) + " found " + std::to_string(count));
        }

        for (auto& field : view.type()->fields) {
            auto edit = view.editField(field);
            if (!edit) {
                return std::unexpected("Failed to deserialize: failed to edit field " + std::string(field.key));
            }

            auto deserializedField = deserializeValue(json.at(field.key), edit.value(), policies);
            if (!deserializedField) {
                return std::unexpected("Failed to deserialize field " + std::string(field.key) + ": " + deserializedField.error());
            }
        }

        return {};
    }

    return std::unexpected("Failed to deserialized: not supported type");
}
