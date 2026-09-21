#pragma once

#include <string>
#include <vector>
#include <optional>
#include <cstdint>
#include <type_traits>
#include <variant>

enum class Access {
    Private,
    Protected,
    Public
};

enum class Qualifier : uint8_t {
    None = 0,
    Const = 1 << 0,
    Volatile = 1 << 1
};

inline Qualifier operator|(Qualifier lhs, Qualifier rhs) {
    using T = std::underlying_type_t<Qualifier>;
    return static_cast<Qualifier>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

inline Qualifier& operator|=(Qualifier& lhs, Qualifier rhs) {
    lhs = lhs | rhs;
    return lhs;
}

inline Qualifier operator&(Qualifier lhs, Qualifier rhs) {
    using T = std::underlying_type_t<Qualifier>;
    return static_cast<Qualifier>(static_cast<T>(lhs) & static_cast<T>(rhs));
}

inline Qualifier& operator&=(Qualifier& lhs, Qualifier rhs) {
    lhs = lhs & rhs;
    return lhs;
}

inline bool hasAny(const Qualifier& lhs, Qualifier rhs) {
    return (lhs & rhs) != Qualifier::None;
}

enum class BuiltinKind {
    Void,
    Bool,
    Int,
    Float,
    Double,
    String,
};

struct RecordTypeRefModel {
    std::string qualifiedName;
};

struct TypeRefModel {
    std::string spelling;
    std::variant<std::monostate, BuiltinKind, RecordTypeRefModel> target;
    Qualifier qualifiers = Qualifier::None;
};

struct ModifierModel {
    std::string qualifiedTypeName;
    std::string initializationExpression;
};

struct FieldModel {
    std::string name;
    TypeRefModel type;
    Access access;
    std::vector<ModifierModel> modifiers{};
};

struct ParameterModel {
    std::string name;
    TypeRefModel type;
};

struct MethodModel {
    std::string name;
    Access access;
    bool isStatic = false;
    Qualifier qualifiers = Qualifier::None;
    std::vector<ModifierModel> modifiers{};
    TypeRefModel returnType;
    std::vector<ParameterModel> parameters{};
};

struct TypeModel {
    std::string name;
    std::string displayName;
    std::vector<ModifierModel> modifiers{};
    std::vector<FieldModel> fields{};
    std::vector<MethodModel> methods{};
};
