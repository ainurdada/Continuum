#pragma once

#include <SDL3/SDL.h>
#include <input/public/input.h>

namespace engine::input {

class InputBackend {
  private:
    Input _input;

  public:
    const Input& get() const noexcept;
    void process(const SDL_KeyboardEvent& event);
    void process(const SDL_MouseButtonEvent& event);
    void process(const SDL_MouseMotionEvent& event);
    void process(const SDL_MouseWheelEvent& event);
    void endFrame();
    void cancel();
};

} // namespace engine::input
