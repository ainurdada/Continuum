#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

#include "type_access_fwd.h"

namespace engine::reflection::modifiers {

enum class MarkerTarget : std::uint8_t {
    None = 0,
    Object = 1 << 0,
    Field = 1 << 1,
    Function = 1 << 2,
};

constexpr MarkerTarget operator|(const MarkerTarget& a, MarkerTarget b) {
    std::uint8_t res = static_cast<std::uint8_t>(a) | static_cast<std::uint8_t>(b);
    return static_cast<MarkerTarget>(res);
}

constexpr MarkerTarget operator&(const MarkerTarget& a, MarkerTarget b) {
    std::uint8_t res = static_cast<std::uint8_t>(a) & static_cast<std::uint8_t>(b);
    return static_cast<MarkerTarget>(res);
}

template <typename T>
concept Modifier = requires {
    { T::targets } -> std::same_as<const MarkerTarget&>;
    requires std::integral_constant<MarkerTarget, T::targets>::value == T::targets;
};

template <Modifier M> consteval bool isModifierAllowed(MarkerTarget target) {
    if (target != MarkerTarget::Object && target != MarkerTarget::Field && target != MarkerTarget::Function) {
        return false;
    }
    return (M::targets & target) != MarkerTarget::None;
}

template <Modifier T, Modifier... M> consteval std::size_t countModifierType() {
    std::size_t res = 0;
    return (res + ... + std::same_as<std::remove_cvref_t<M>, T>);
}

template <Modifier... M> consteval bool areModifiersTypesUnique() {
    return ((countModifierType<M, M...>() == 1) && ...);
}

template <Modifier... M> consteval bool validateModifiers(MarkerTarget target) {
    if (target != MarkerTarget::Object && target != MarkerTarget::Field && target != MarkerTarget::Function) {
        return false;
    }
    if (!areModifiersTypesUnique<M...>()) {
        return false;
    }
    return (isModifierAllowed<M>(target) && ...);
}

/// @brief Overrides the displayed name without changing reflection keys.
/// The referenced string data must outlive this modifier.
struct DisplayName {
    std::string_view value;
    static constexpr MarkerTarget targets = MarkerTarget::Object | MarkerTarget::Field | MarkerTarget::Function;

    /// @brief Overrides the displayed name without changing reflection keys.
    /// The referenced string data must outlive this modifier.
    /// @param name should live longer than modifier
    constexpr explicit DisplayName(std::string_view name) : value(name) {}
};

struct Key {
    std::string_view value;
    static constexpr MarkerTarget targets = MarkerTarget::Object | MarkerTarget::Field;

    constexpr explicit Key(std::string_view key) : value(key) {}
};

/// @brief Requests generation of ECS reflection bindings for this type.
struct Component {
    static constexpr MarkerTarget targets = MarkerTarget::Object;
};

/// @brief Allows the default inspector to display this field.
struct ShowInInspector {
    static constexpr MarkerTarget targets = MarkerTarget::Field;
};

/// @brief Disables editing of this field in the editor.
/// Does not change C++ constness or access outside the editor.
struct ReadOnly {
    static constexpr MarkerTarget targets = MarkerTarget::Field;
};

/// @brief Sets the drag sensitivity of a numeric field in the inspector.
/// Specifies the base change in stored units per pixel of mouse movement.
/// The value must be positive and finite.
/// Does not require edited values to be multiples of the step.
struct Step {
    float value;
    static constexpr MarkerTarget targets = MarkerTarget::Field;

    constexpr explicit Step(float step) : value(step) {}
};

struct Min {
    double value;
    static constexpr MarkerTarget targets = MarkerTarget::Field;

    constexpr explicit Min(double step) : value(step) {}
};

struct Max {
    double value;
    static constexpr MarkerTarget targets = MarkerTarget::Field;

    constexpr explicit Max(double step) : value(step) {}
};

} // namespace engine::reflection::modifiers

#define CONTINUUM_DETAIL_EXPAND_1(...) __VA_ARGS__
#define CONTINUUM_DETAIL_EXPAND_2(...) CONTINUUM_DETAIL_EXPAND_1(CONTINUUM_DETAIL_EXPAND_1(CONTINUUM_DETAIL_EXPAND_1(CONTINUUM_DETAIL_EXPAND_1(__VA_ARGS__))))
#define CONTINUUM_DETAIL_EXPAND_3(...) CONTINUUM_DETAIL_EXPAND_2(CONTINUUM_DETAIL_EXPAND_2(CONTINUUM_DETAIL_EXPAND_2(CONTINUUM_DETAIL_EXPAND_2(__VA_ARGS__))))
#define CONTINUUM_DETAIL_EXPAND_4(...) CONTINUUM_DETAIL_EXPAND_3(CONTINUUM_DETAIL_EXPAND_3(CONTINUUM_DETAIL_EXPAND_3(CONTINUUM_DETAIL_EXPAND_3(__VA_ARGS__))))

#define CONTINUUM_DETAIL_PARENS ()
#define CONTINUUM_DETAIL_MAP_AGAIN() CONTINUUM_DETAIL_MAP_STEP
#define CONTINUUM_DETAIL_MAP_STEP(action, first, ...) action(first) __VA_OPT__(, CONTINUUM_DETAIL_MAP_AGAIN CONTINUUM_DETAIL_PARENS(action, __VA_ARGS__))
#define CONTINUUM_DETAIL_MAP(action, ...) __VA_OPT__(CONTINUUM_DETAIL_EXPAND_4(CONTINUUM_DETAIL_MAP_STEP(action, __VA_ARGS__)))

#define CONTINUUM_DETAIL_MODIFIER_TYPE(modifier) ::std::remove_pointer_t<decltype(::new modifier)>

#define OBJECT(...)                                                                                                                                                                                                                                                                                        \
    static_assert([]() consteval {                                                                                                                                                                                                                                                                         \
        using namespace ::engine::reflection::modifiers;                                                                                                                                                                                                                                                   \
        return ::engine::reflection::modifiers::validateModifiers<CONTINUUM_DETAIL_MAP(CONTINUUM_DETAIL_MODIFIER_TYPE, __VA_ARGS__)>(::engine::reflection::modifiers::MarkerTarget::Object);                                                                                                               \
    }());

#define FIELD(...)                                                                                                                                                                                                                                                                                         \
    static_assert([]() consteval {                                                                                                                                                                                                                                                                         \
        using namespace ::engine::reflection::modifiers;                                                                                                                                                                                                                                                   \
        return ::engine::reflection::modifiers::validateModifiers<CONTINUUM_DETAIL_MAP(CONTINUUM_DETAIL_MODIFIER_TYPE, __VA_ARGS__)>(::engine::reflection::modifiers::MarkerTarget::Field);                                                                                                                \
    }());                                                                                                                                                                                                                                                                                                  \
    template <typename T> friend struct ::engine::reflection::detail::TypeAccess;

#define FUNCTION(...)                                                                                                                                                                                                                                                                                      \
    static_assert([]() consteval {                                                                                                                                                                                                                                                                         \
        using namespace ::engine::reflection::modifiers;                                                                                                                                                                                                                                                   \
        return ::engine::reflection::modifiers::validateModifiers<CONTINUUM_DETAIL_MAP(CONTINUUM_DETAIL_MODIFIER_TYPE, __VA_ARGS__)>(::engine::reflection::modifiers::MarkerTarget::Function);                                                                                                             \
    }());                                                                                                                                                                                                                                                                                                  \
    template <typename T> friend struct ::engine::reflection::detail::TypeAccess;