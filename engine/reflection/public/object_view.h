#pragma once

#include <cassert>
#include <memory>
#include <optional>
#include <type_traits>

#include "type_descriptor.h"
#include "type_registry.h"

namespace engine::reflection {

class ObjectView {
    const void* _object;
    void* _mutableObject;
    const TypeDescriptor* _desc;

    ObjectView(const void* object, void* mutableObject, const TypeDescriptor* desc) : _object(object), _mutableObject(mutableObject), _desc(desc) {
        assert(!_mutableObject || _mutableObject == object);
    }

  public:
    template <typename T> static std::optional<ObjectView> from(T& object, const TypeRegistry& reg) {
        if (!reg.isFrozen()) {
            return std::nullopt;
        }
        
        auto desc = reg.findtype(typeid(T));
        if (!desc) {
            return std::nullopt;
        }

        return from(object, *desc);
    }

    template <typename T> static std::optional<ObjectView> from(T& object, const TypeDescriptor& desc) {
        static_assert(!std::is_volatile_v<T>);

        if (desc.nativeTypeKey != typeid(T)) {
            return std::nullopt;
        }

        if constexpr (std::is_const_v<T>) {
            return ObjectView(std::addressof(object), nullptr, &desc);
        } else {
            return ObjectView(std::addressof(object), std::addressof(object), &desc);
        }
    }

    const void* const data() const {
        return _object;
    }

    const TypeDescriptor* const type() const {
        return _desc;
    }

    bool canWrite() const {
        return _mutableObject;
    }

    template <typename T> const T* tryAs() const {
        static_assert(std::is_object_v<T>);
        static_assert(!std::is_volatile_v<T>);

        if (_desc->nativeTypeKey != typeid(T)) {
            return nullptr;
        }
        return static_cast<const T*>(_object);
    }

    template <typename T> T* tryAsMut() const {
        static_assert(std::is_object_v<T>);
        static_assert(!std::is_volatile_v<T>);
        static_assert(!std::is_const_v<T>);

        if (!canWrite() || _desc->nativeTypeKey != typeid(T)) {
            return nullptr;
        }
        return static_cast<T*>(_mutableObject);
    }

    std::optional<ObjectView> readField(const FieldDescriptor& desc) const {
        if (!desc.type || !desc.readAddress) {
            return std::nullopt;
        }
        auto address = desc.readAddress(*this);
        if (!address) {
            return std::nullopt;
        }
        return ObjectView(address, nullptr, desc.type);
    }

    std::optional<ObjectView> editField(const FieldDescriptor& desc) const {
        if (!canWrite()) {
            return std::nullopt;
        }
        if (!desc.type || !desc.mutableAddress) {
            return std::nullopt;
        }
        if ((desc.qualifiers & engine::reflection::TypeQualifier::Const) != engine::reflection::TypeQualifier::None) {
            return std::nullopt;
        }
        if ((desc.qualifiers & engine::reflection::TypeQualifier::Volatile) != engine::reflection::TypeQualifier::None) {
            return std::nullopt;
        }
        auto address = desc.mutableAddress(*this);
        if (!address) {
            return std::nullopt;
        }
        return ObjectView(address, address, desc.type);
    }
};

} // namespace engine::reflection
