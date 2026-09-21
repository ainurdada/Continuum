#pragma once

#include <input/public/input.h>
#include <render/public/render_frame_data.h>

namespace engine {

enum class Result {
    Continue,
    Success,
    Failure
};

struct FrameInfo {
    double realDeltaSeconds;
};

enum class EventType {
    Unknown,
    CloseRequested
};

struct Event {
    EventType type{EventType::Unknown};
};

struct GameDescription {
    const char* title;
    int windowWidth;
    int windowHeight;
};

class Engine {
  private:
    const input::Input& _input;

  public:
    explicit Engine(const input::Input& input) noexcept : _input(input) {}
    const input::Input& input() const noexcept {
        return _input;
    }
};

GameDescription createGame();
Result startGame(void** appState);
Result updateGame(void* appState, const Engine& engine, const FrameInfo& frameInfo);
Result renderGame(const void* appState, RenderFrameData& data);
Result processEvent(void* appState, const Event& event);
void stopGame(void* appState, Result result);

} // namespace engine
