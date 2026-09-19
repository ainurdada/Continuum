#pragma once

#include <expected>
#include <string>
#include <typeindex>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include <reflection/public/object_view.h>

namespace engine::reflection::serialization {

class JsonSerializationRegistry {
  public:
    using SerializeFn = std::expected<nlohmann::json, std::string> (*)(const ObjectView& view);
    using DeserializeFn = std::expected<void, std::string> (*)(const nlohmann::json& json, const ObjectView& view);

    struct PolicyEntry {
        SerializeFn serialize = nullptr;
        DeserializeFn deserialize = nullptr;
    };

  private:
    std::unordered_map<std::type_index, PolicyEntry> _entries;

  public:
    const PolicyEntry* findPolicy(std::type_index typeIndex) const {
        auto it = _entries.find(typeIndex);
        if (it == _entries.end()) {
            return nullptr;
        }
        return &it->second;
    }

  private:
    template <typename T, typename Policy> static std::expected<nlohmann::json, std::string> serializeTyped(const ObjectView& view) {
        auto value = view.tryAs<T>();
        if (!value) {
            return std::unexpected("Not correct type");
        }
        return Policy::serialize(*value);
    }

    template <typename T, typename Policy> static std::expected<void, std::string> deserializeTyped(const nlohmann::json& json, const ObjectView& view) {
        auto value = view.tryAsMut<T>();
        if (!value) {
            return std::unexpected("Not correct type");
        }
        return Policy::deserialize(json, *value);
    }

  public:
    template <typename T, typename Policy> bool registerPolicy() {
        if (_entries.contains(typeid(T))) {
            return false;
        }
        _entries.emplace(typeid(T), PolicyEntry{.serialize = &serializeTyped<T, Policy>, .deserialize = &deserializeTyped<T, Policy>});
        return true;
    }
};

} // namespace engine::reflection::serialization
