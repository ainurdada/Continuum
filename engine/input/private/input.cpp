#include <input.h>

namespace engine::input {

inline bool validateKey(Key key) {
    return key > Key::UNKNOWN && key < Key::COUNT;
}

inline bool validateMouseButton(MouseButton button) {
    return button > MouseButton::UNKNOWN && button < MouseButton::COUNT;
}

void Input::press(Key key) {
    if (!validateKey(key)) {
        return;
    }
    ButtonState& state = _keyboardState.states[static_cast<std::size_t>(key)];
    if (state.isDown) {
        return;
    }
    state.isDown = true;
    state.wasPressedThisFrame = true;
}

void Input::release(Key key) {
    if (!validateKey(key)) {
        return;
    }
    ButtonState& state = _keyboardState.states[static_cast<std::size_t>(key)];
    if (!state.isDown) {
        return;
    }
    state.isDown = false;
    state.wasReleasedThisFrame = true;
}

void Input::press(MouseButton button) {
    if (!validateMouseButton(button)) {
        return;
    }
    ButtonState& state = _mouseState.states[static_cast<std::size_t>(button)];
    if (state.isDown) {
        return;
    }
    state.isDown = true;
    state.wasPressedThisFrame = true;
}

void Input::release(MouseButton button) {
    if (!validateMouseButton(button)) {
        return;
    }
    ButtonState& state = _mouseState.states[static_cast<std::size_t>(button)];
    if (!state.isDown) {
        return;
    }
    state.isDown = false;
    state.wasReleasedThisFrame = true;
}

void Input::addMouseMotion(Vec2f position, Vec2f deltaPosition) {
    _mouseState.position = position;
    _mouseState.deltaPosition += deltaPosition;
}

void Input::addWheelDelta(Vec2f delta) {
    _mouseState.wheelDelta += delta;
}

void Input::clearTransientState() {
    for (auto& state : _keyboardState.states) {
        state.wasPressedThisFrame = false;
        state.wasReleasedThisFrame = false;
    }
    for (auto& state : _mouseState.states) {
        state.wasPressedThisFrame = false;
        state.wasReleasedThisFrame = false;
    }

    _mouseState.deltaPosition = {};
    _mouseState.wheelDelta = {};
}

void Input::cancelState() {
    for (auto& state : _keyboardState.states) {
        state.wasPressedThisFrame = false;
        state.wasReleasedThisFrame = false;
        state.isDown = false;
    }
    for (auto& state : _mouseState.states) {
        state.wasPressedThisFrame = false;
        state.wasReleasedThisFrame = false;
        state.isDown = false;
    }

    _mouseState.deltaPosition = {};
    _mouseState.wheelDelta = {};
}

bool Input::isPressed(Key key) const {
    if (!validateKey(key)) {
        return false;
    }
    const ButtonState& state = _keyboardState.states[static_cast<std::size_t>(key)];
    return state.wasPressedThisFrame;
}

bool Input::isHold(Key key) const {
    if (!validateKey(key)) {
        return false;
    }
    const ButtonState& state = _keyboardState.states[static_cast<std::size_t>(key)];
    return state.isDown;
}

bool Input::isReleased(Key key) const {
    if (!validateKey(key)) {
        return false;
    }
    const ButtonState& state = _keyboardState.states[static_cast<std::size_t>(key)];
    return state.wasReleasedThisFrame;
}

bool Input::isPressed(MouseButton button) const {
    if (!validateMouseButton(button)) {
        return false;
    }
    const ButtonState& state = _mouseState.states[static_cast<std::size_t>(button)];
    return state.wasPressedThisFrame;
}

bool Input::isHold(MouseButton button) const {
    if (!validateMouseButton(button)) {
        return false;
    }
    const ButtonState& state = _mouseState.states[static_cast<std::size_t>(button)];
    return state.isDown;
}

bool Input::isReleased(MouseButton button) const {
    if (!validateMouseButton(button)) {
        return false;
    }
    const ButtonState& state = _mouseState.states[static_cast<std::size_t>(button)];
    return state.wasReleasedThisFrame;
}

Vec2f Input::mousePosition() const noexcept {
    return _mouseState.position;
}

Vec2f Input::mouseDeltaPosition() const noexcept {
    return _mouseState.deltaPosition;
}

Vec2f Input::mouseWheelDelta() const noexcept {
    return _mouseState.wheelDelta;
}

} // namespace engine::input