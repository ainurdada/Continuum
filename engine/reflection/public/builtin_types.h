#pragma once

#include <typeinfo>
#include <span>
#include <array>
#include <string>

#include "type_descriptor.h"

namespace engine::reflection {

// clang-format off
inline const TypeDescriptor* voidType() {
    static const TypeDescriptor desc{
        .nativeTypeKey = typeid(void),
        .key = "void",
        .name = "void",
        .category = TypeCategory::Void,
    };
    return &desc;
}

inline const TypeDescriptor* boolType() {
    static const TypeDescriptor desc{
        .nativeTypeKey = typeid(bool),
        .key = "bool",
        .name = "bool",
        .category = TypeCategory::Boolean,
    };
    return &desc;
}

inline const TypeDescriptor* intType() {
    static const TypeDescriptor desc{
        .nativeTypeKey = typeid(int),
        .key = "int",
        .name = "int",
        .category = TypeCategory::Integer,
    };
    return &desc;
}

inline const TypeDescriptor* floatType() {
    static const TypeDescriptor desc{
        .nativeTypeKey = typeid(float),
        .key = "float",
        .name = "float",
        .category = TypeCategory::FloatingPoint,
    };
    return &desc;
}

inline const TypeDescriptor* doubleType() {
    static const TypeDescriptor desc{
        .nativeTypeKey = typeid(double),
        .key = "double",
        .name = "double",
        .category = TypeCategory::FloatingPoint,
    };
    return &desc;
}

inline const TypeDescriptor* stringType() {
    static const TypeDescriptor desc{
        .nativeTypeKey = typeid(std::string),
        .key = "std::string",
        .name = "string",
        .category = TypeCategory::String,
    };
    return &desc;
}

inline std::span<const TypeDescriptor* const> getBuiltinTypes() {
    static const std::array<const TypeDescriptor* const, 6> builtintTypes {
        voidType(),
        boolType(),
        intType(),
        floatType(),
        doubleType(),
        stringType(),
    };
    return builtintTypes;
}
// clang-format on

} // namespace engine::reflection
