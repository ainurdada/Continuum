#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <unordered_set>

#include "type_descriptor.h"

namespace engine::reflection {

class TypeRegistry {
    struct StoredType {
        TypeDescriptor desc;
        std::vector<FieldDescriptor> fields;

        StoredType(const TypeDescriptor& source) : desc(source), fields(source.fields.begin(), source.fields.end()) {
            desc.fields = fields;

            for (auto& field : fields) {
                field.type = nullptr;
            }
        }

        StoredType(const StoredType&) = delete;
        StoredType& operator=(const StoredType&) = delete;

        StoredType(StoredType&&) = delete;
        StoredType& operator=(StoredType&&) = delete;
    };

    bool _frozen = false;
    std::unordered_map<std::string, const TypeDescriptor*> _typeDescMap{};
    std::unordered_map<std::type_index, std::unique_ptr<StoredType>> _typeIndexDescMap{};

  public:
    bool registerType(const TypeDescriptor* typeDesc) {
        if (isFrozen()) {
            return false;
        }
        if (!typeDesc || typeDesc->key.empty() || _typeDescMap.contains(std::string(typeDesc->key)) || _typeIndexDescMap.contains(typeDesc->nativeTypeKey)) {
            return false;
        }
        auto newStoredType = std::make_unique<StoredType>(*typeDesc);
        auto [it1, nativeTypeKeyAdded] = _typeIndexDescMap.emplace(typeDesc->nativeTypeKey, std::move(newStoredType));
        if (!nativeTypeKeyAdded) {
            return false;
        }
        try {
            auto [it2, keyAdded] = _typeDescMap.emplace(std::string(typeDesc->key), &it1->second->desc);
            if (!keyAdded) {
                _typeIndexDescMap.erase(typeDesc->nativeTypeKey);
                return false;
            }
        } catch (...) {
            _typeIndexDescMap.erase(typeDesc->nativeTypeKey);
            throw;
        }
        return true;
    }

    const TypeDescriptor* findtype(std::string_view key) const {
        if (!_typeDescMap.contains(std::string(key))) {
            return nullptr;
        }
        return _typeDescMap.at(std::string(key));
    }

    const TypeDescriptor* findtype(std::type_index typeIndex) const {
        if (!_typeIndexDescMap.contains(typeIndex)) {
            return nullptr;
        }
        return &_typeIndexDescMap.at(typeIndex)->desc;
    }

    bool isFrozen() const noexcept {
        return _frozen;
    }

    bool freeze() {
        if (_frozen) {
            return true;
        }
        for (auto& [k, v] : _typeIndexDescMap) {
            std::unordered_set<std::string_view> visitedKeys{};
            for (auto& field : v->fields) {
                if (field.key.empty() || !visitedKeys.insert(field.key).second || field.nativeTypeKey == typeid(void) || !findtype(field.nativeTypeKey)) {
                    return false;
                }
            }
        }
        for (auto& [k, v] : _typeIndexDescMap) {
            for (auto& field : v->fields) {
                field.type = findtype(field.nativeTypeKey);
            }
        }
        return _frozen = true;
    }
};

} // namespace engine::reflection
