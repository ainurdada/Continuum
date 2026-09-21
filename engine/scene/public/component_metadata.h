#pragma once

#include <string_view>
#include <span>

namespace engine::scene {

enum class FieldKind {
    Float,
    Vec3,
    String,
    Entity,
    Enum
};

enum class FieldUnit {
    None,
    Meters,
    Radians
};

struct FieldMetadata {
    std::string_view key;
    std::string_view name;
    FieldKind kind;
    FieldUnit unit = FieldUnit::None;
};

struct ComponentMetadata {
    std::string_view key;
    std::string_view name;
    std::span<const FieldMetadata> fields;
};

} // namespace engine::scene
