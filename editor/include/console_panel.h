#pragma once

#define CONSOLE_WINDOW_NAME "Console"

namespace editor {

class ConsoleLog;

class ConsolePanel {
  public:
    void draw(ConsoleLog& log);
};

} // namespace editor
