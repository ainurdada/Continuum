#pragma once

#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>
#include <typeindex>

#include <modifiers.h>

namespace engine::reflection {

enum class TypeQualifier : std::uint8_t {
    None = 0,
    Const = 1 << 0,
    Volatile = 1 << 1,
};

enum class TypeCategory {
    Void,
    Boolean,
    Integer,
    FloatingPoint,
    String,
    Object
};

inline TypeQualifier operator|(TypeQualifier lhs, TypeQualifier rhs) {
    using T = std::underlying_type_t<TypeQualifier>;
    return static_cast<TypeQualifier>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

inline TypeQualifier& operator|=(TypeQualifier& lhs, TypeQualifier rhs) {
    lhs = lhs | rhs;
    return lhs;
}

inline TypeQualifier operator&(TypeQualifier lhs, TypeQualifier rhs) {
    using T = std::underlying_type_t<TypeQualifier>;
    return static_cast<TypeQualifier>(static_cast<T>(lhs) & static_cast<T>(rhs));
}

inline TypeQualifier& operator&=(TypeQualifier& lhs, TypeQualifier rhs) {
    lhs = lhs & rhs;
    return lhs;
}

struct TypeDescriptor;

class ObjectView;

struct FieldDescriptor {
    /// @brief C++ type index
    std::type_index nativeTypeKey = typeid(void);

    /// @brief Constant field key inside owner
    std::string_view key;

    /// @brief Displayed name
    std::string_view name;

    /// @brief Field type descriptor
    const TypeDescriptor* type = nullptr;

    /// @brief FIELD modifiers
    engine::reflection::Modifiers modifiers{};

    /// @brief C++ type qualifiers
    TypeQualifier qualifiers;

    using ReadAddressFn = const void* (*)(const ObjectView&);
    ReadAddressFn readAddress = nullptr;

    using MutableAddressFn = void* (*)(const ObjectView&);
    MutableAddressFn mutableAddress = nullptr;
};

struct TypeDescriptor {
    /// @brief C++ type index
    std::type_index nativeTypeKey;

    /// @brief Constant type key
    std::string_view key;

    /// @brief Displayed name
    std::string_view name;

    /// @brief OBJECT modifiers
    engine::reflection::Modifiers modifiers{};

    /// @brief Common type category (for example, float and double belong to TypeCategory::FloatingPoint)
    TypeCategory category;

    std::span<const FieldDescriptor> fields;
};

} // namespace engine::reflection