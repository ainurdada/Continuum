#include <console_log.h>

namespace editor {
void SDLCALL ConsoleLog::logCallback(void* userData, int category, SDL_LogPriority priority, const char* text) noexcept {
    ConsoleLog* consoleLog = static_cast<ConsoleLog*>(userData);
    {
        try {
            std::lock_guard lock{consoleLog->_mutex};
            consoleLog->_consoleMessages.push_back(ConsoleMessage{.category = category, .priority = priority, .text = text});
        } catch (...) {
        }
    }
    if (consoleLog->_previousLogOutFunction) {
        consoleLog->_previousLogOutFunction(consoleLog->_previousUserData, category, priority, text);
    }
}

ConsoleLog::ConsoleLog() {
    SDL_GetLogOutputFunction(&_previousLogOutFunction, &_previousUserData);
    SDL_SetLogOutputFunction(logCallback, this);
}

ConsoleLog::~ConsoleLog() {
    SDL_SetLogOutputFunction(_previousLogOutFunction, _previousUserData);
}

void ConsoleLog::clear() {
    std::lock_guard lock{_mutex};
    _consoleMessages.clear();
}

std::vector<ConsoleMessage> ConsoleLog::snapshot() {
    std::lock_guard lock{_mutex};
    return _consoleMessages;
}

} // namespace editor
