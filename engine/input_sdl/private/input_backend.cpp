#include <input_backend.h>

namespace engine::input {

static_assert(static_cast<int>(Key::UNKNOWN) == static_cast<int>(SDL_SCANCODE_UNKNOWN));
static_assert(static_cast<int>(Key::A) == static_cast<int>(SDL_SCANCODE_A));
static_assert(static_cast<int>(Key::SPACE) == static_cast<int>(SDL_SCANCODE_SPACE));
static_assert(static_cast<int>(Key::COUNT) == static_cast<int>(SDL_SCANCODE_COUNT));

const Input& InputBackend::get() const noexcept {
    return _input;
}

void InputBackend::process(const SDL_KeyboardEvent& event) {
    Key key = static_cast<Key>(event.scancode);

    if (event.down) {
        _input.press(key);
    } else {
        _input.release(key);
    }
}

void InputBackend::process(const SDL_MouseButtonEvent& event) {
    MouseButton button{};
    switch (event.button) {
    case SDL_BUTTON_LEFT:
        button = MouseButton::LEFT;
        break;

    case SDL_BUTTON_MIDDLE:
        button = MouseButton::MIDDLE;
        break;

    case SDL_BUTTON_RIGHT:
        button = MouseButton::RIGHT;
        break;

    case SDL_BUTTON_X1:
        button = MouseButton::X1;
        break;

    case SDL_BUTTON_X2:
        button = MouseButton::X2;
        break;
    default:
        break;
    }

    if (event.down) {
        _input.press(button);
    } else {
        _input.release(button);
    }
}

void InputBackend::process(const SDL_MouseMotionEvent& event) {
    Vec2f position{event.x, event.y};
    Vec2f delta{event.xrel, event.yrel};
    _input.addMouseMotion(position, delta);
}

void InputBackend::process(const SDL_MouseWheelEvent& event) {
    Vec2f delta{event.x, event.y};
    if (event.direction == SDL_MOUSEWHEEL_FLIPPED) {
        delta = -delta;
    }
    _input.addWheelDelta(delta);
}

void InputBackend::endFrame() {
    _input.clearTransientState();
}

void InputBackend::cancel() {
    _input.cancelState();
}

} // namespace engine::input