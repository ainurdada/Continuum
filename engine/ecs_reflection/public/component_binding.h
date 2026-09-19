#pragma once

#include <expected>
#include <optional>
#include <string>
#include <type_traits>
#include <typeindex>

#include <ecs/ecs.h>
#include <reflection/public/object_view.h>
#include <reflection/public/type_descriptor.h>

namespace engine::ecs_reflection { 

struct ComponentBinding {
    std::type_index nativeTypeIndex;

    const reflection::TypeDescriptor* type = nullptr;

    using ReadFn = std::optional<reflection::ObjectView> (*)(const ecs::IStash&, ecs::Entity, const reflection::TypeDescriptor&);
    ReadFn read = nullptr;

    using WriteFn = std::optional<reflection::ObjectView> (*)(ecs::IStash&, ecs::Entity, const reflection::TypeDescriptor&);
    WriteFn write = nullptr;

    using CreateFn = std::expected<reflection::ObjectView, std::string> (*)(ecs::World&, ecs::Entity, const reflection::TypeDescriptor&);
    CreateFn create = nullptr;
};

namespace detail {

template <typename T> std::optional<reflection::ObjectView> readComponent(const ecs::IStash& stash, ecs::Entity entity, const reflection::TypeDescriptor& desc) {
    if (stash.nativeTypeKey() != typeid(T)) {
        return std::nullopt;
    }
    const ecs::Stash<T>& typedStash = static_cast<const ecs::Stash<T>&>(stash);
    const auto* component = typedStash.get(entity);
    if (!component) {
        return std::nullopt;
    }
    return reflection::ObjectView::from(*component, desc);
}

template <typename T> std::optional<reflection::ObjectView> writeComponent(ecs::IStash& stash, ecs::Entity entity, const reflection::TypeDescriptor& desc) {
    if (stash.nativeTypeKey() != typeid(T)) {
        return std::nullopt;
    }
    ecs::Stash<T>& typedStash = static_cast<ecs::Stash<T>&>(stash);
    auto* component = typedStash.getMut(entity);
    if (!component) {
        return std::nullopt;
    }
    return reflection::ObjectView::from(*component, desc);
}

template <typename T> std::expected<reflection::ObjectView, std::string> createComponent(ecs::World& world, ecs::Entity entity, const reflection::TypeDescriptor& desc) {
    if (desc.nativeTypeKey != typeid(T)) {
        return std::unexpected("Not correct component");
    }
    if (!world.hasEntity(entity)) {
        return std::unexpected("Not valid entity");
    }

    auto& stash = world.getStash<T>();
    if (stash.has(entity)) {
        return std::unexpected("Component already exists");
    }

    stash.add(entity, T{});
    auto component = stash.getMut(entity);
    if (!component) {
        return std::unexpected("Failed to create component");
    }

    auto view = reflection::ObjectView::from(*component, desc);
    if (!view) {
        return std::unexpected("Failed to get object view");
    }

    return view.value();
}

} // namespace detail

template <typename T> std::optional<ComponentBinding> makeComponentBinding(const reflection::TypeRegistry& reg) {
    if (!reg.isFrozen()) {
        return std::nullopt;
    }

    auto desc = reg.findtype(typeid(T));
    if (!desc) {
        return std::nullopt;
    }

    if (!desc->modifiers.has<reflection::modifiers::Component>()) {
        return std::nullopt;
    }

    auto binding = ComponentBinding{
        .nativeTypeIndex = typeid(T),
        .type = desc,
        .read = &detail::readComponent<T>,
        .write = &detail::writeComponent<T>,
    };

    if constexpr (std::is_default_constructible_v<T> && std::is_copy_constructible_v<T>) {
        binding.create = &detail::createComponent<T>;
    }

    return binding;
}

} // namespace engine::ecs_reflection
