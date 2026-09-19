#pragma once

#include <memory>
#include <span>
#include <typeindex>

#include <markers.h>

namespace engine::reflection {

class Modifiers;

namespace detail {

struct ModifierEntry {
  private:
    friend Modifiers;
    std::type_index _key;
    const void* _value;

  public:
    template <modifiers::Modifier M> ModifierEntry(const M& m) : _key(typeid(M)), _value(std::addressof(m)) {}

    ModifierEntry(const modifiers::Modifier auto&& m) = delete;
};

} // namespace detail

class Modifiers {
    std::span<const detail::ModifierEntry> _entries{};

  public:
    Modifiers() = default;
    Modifiers(const std::span<const detail::ModifierEntry>& entries) : _entries(entries) {}

    template <modifiers::Modifier M> const M* tryGet() const {
        std::type_index key = typeid(M);
        for (const auto& entry : _entries) {
            if (key == entry._key) {
                return static_cast<const M*>(entry._value);
            }
        }

        return nullptr;
    }

    template <modifiers::Modifier M> bool has() const {
        return tryGet<M>();
    }
};

} // namespace engine::reflection
