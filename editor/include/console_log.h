#pragma once

#include <mutex>
#include <string>
#include <vector>

#include <SDL3/SDL_log.h>

namespace editor {

struct ConsoleMessage {
    int category;
    SDL_LogPriority priority;
    std::string text;
};

class ConsoleLog {
  private:
    std::mutex _mutex{};
    std::vector<ConsoleMessage> _consoleMessages;
    SDL_LogOutputFunction _previousLogOutFunction;
    void* _previousUserData;

  private:
    static void SDLCALL logCallback(void* userData, int category, SDL_LogPriority priority, const char* text) noexcept;

  public:
    ConsoleLog(const ConsoleLog&) = delete;
    ConsoleLog& operator=(const ConsoleLog&) = delete;

    ConsoleLog(ConsoleLog&&) = delete;
    ConsoleLog& operator=(ConsoleLog&&) = delete;

    ConsoleLog();
    ~ConsoleLog();

    /// @brief Clear console messages
    void clear();

    /// @brief Get a copy of console messages
    [[nodiscard]] std::vector<ConsoleMessage> snapshot();
};

} // namespace editor
