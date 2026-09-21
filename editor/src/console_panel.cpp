#include <console_panel.h>

#include <imgui.h>

#include <console_log.h>

namespace editor {

namespace {

std::string sdlLogPriorityToString(SDL_LogPriority priority) {
    switch (priority) {
    case SDL_LOG_PRIORITY_TRACE:
        return "TRACE";
    case SDL_LOG_PRIORITY_VERBOSE:
        return "VERBOSE";
    case SDL_LOG_PRIORITY_DEBUG:
        return "DEBUG";
    case SDL_LOG_PRIORITY_INFO:
        return "INFO";
    case SDL_LOG_PRIORITY_WARN:
        return "WARN";
    case SDL_LOG_PRIORITY_ERROR:
        return "ERROR";
    case SDL_LOG_PRIORITY_CRITICAL:
        return "CRITICAL";
    default:
        return "UNKNOWN";
    }
}

} // namespace

void ConsolePanel::draw(ConsoleLog& log) {
    if (ImGui::Begin(CONSOLE_WINDOW_NAME)) {
        if (ImGui::Button("Clear")) {
            log.clear();
        }
        if (ImGui::BeginChild("ConsoleMessages", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar)) {
            for (const auto& consoleMessage : log.snapshot()) {
                std::string logMessageText = "[" + sdlLogPriorityToString(consoleMessage.priority) + "] " + consoleMessage.text;
                ImGui::TextUnformatted(logMessageText.c_str());
            }
        }
        ImGui::EndChild();
    }
    ImGui::End();
}

} // namespace editor
