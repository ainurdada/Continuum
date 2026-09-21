#pragma once

#include <expected>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

#include <reflection/public/object_view.h>

#include <component_edit.h>
#include <editor_session.h>
#include <ui/modifierHelpers.h>

namespace editor::ui {

template <typename T> class DrawContext {
    static_assert(std::is_object_v<T>);
    static_assert(!std::is_const_v<T>);
    static_assert(!std::is_volatile_v<T>);

    EditorSession& _session;
    ComponentEditTarget _target;
    engine::reflection::ObjectView _rootView;
    engine::reflection::ObjectView _view;
    EditInteraction _interactionKey;
    const engine::reflection::FieldDescriptor* const fieldDesc;
    bool _effectiveReadOnly;

  public:
    DrawContext(EditorSession& session, ComponentEditTarget target, engine::reflection::ObjectView rootView, const engine::reflection::ObjectView& view, EditInteraction interactionKey, const engine::reflection::FieldDescriptor* const fieldDesc, bool effectiveReadOnly)
        : _session(session), _target(target), _rootView(rootView), _view(view), _interactionKey(interactionKey), fieldDesc(fieldDesc), _effectiveReadOnly(effectiveReadOnly) {
        if (!_view.tryAs<T>()) {
            throw std::invalid_argument("View is not belong to type");
        }
        if (_rootView.type()->nativeTypeKey != _target.componentType) {
            throw std::invalid_argument("root type is not equal target type");
        }
        if (_interactionKey.source.empty()) {
            throw std::invalid_argument("interaction key is empty");
        }
    }

    const engine::reflection::TypeDescriptor& type() const {
        return *_view.type();
    }

    const engine::reflection::FieldDescriptor* const field() const {
        return fieldDesc;
    }

    std::string label() const {
        return getDisplayName(type(), fieldDesc);
    }

    const T& getValue() const {
        return *_view.tryAs<T>();
    }

    bool canEdit() const {
        return !_effectiveReadOnly && _rootView.canWrite() && _view.canWrite();
    }

    bool isEditing() const {
        return _session.isComponentEditActive(_target, _interactionKey);
    }

    T& getValueMut() {
        if (!canEdit()) {
            throw std::logic_error("Trying to get mutable access to not mutable object");
        }
        if (!_session.isComponentEditActive(_target, _interactionKey)) {
            throw std::logic_error("Trying to get mutable access to object while it is not in edit mode");
        }
        return *_view.tryAsMut<T>();
    }

    std::expected<void, std::string> beginEdit() {
        if (!canEdit()) {
            return std::unexpected("Component is not able for editing");
        }
        return _session.beginComponentEdit(_target, _rootView, _interactionKey);
    }

    std::expected<void, std::string> markChanged() {
        if (!canEdit()) {
            return std::unexpected("Trying to save changes of not mutable object");
        }
        return _session.markComponentEditChanged(_target, _interactionKey);
    }

    std::expected<void, std::string> endEdit() {
        return _session.endComponentEdit(_target, _rootView, _interactionKey);
    }

    std::expected<void, std::string> cancelEdit() {
        return _session.cancelComponentEdit(_target, _rootView, _interactionKey);
    }

    template <engine::reflection::modifiers::Modifier M> const M* tryGetModifier() {
        if (field()) {
            return field()->modifiers.template tryGet<M>();
        }
        return type().modifiers.template tryGet<M>();
    }

    template <engine::reflection::modifiers::Modifier M> bool hasModifier() {
        return tryGetModifier<M>();
    }
};

} // namespace editor::ui