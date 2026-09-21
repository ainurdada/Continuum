#pragma once

#include <string>

#include <SDL3/SDL_log.h>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <ui/component_drawer.h>

namespace editor::ui {

class StringDrawer : public ComponentDrawer<std::string> {
    void draw(DrawContext<std::string>& ctx) override {
        std::string value = ctx.getValue();

        ImGui::BeginDisabled(!ctx.canEdit());
        bool changed = ImGui::InputText((ctx.label() + "###Value").c_str(), &value);
        bool active = ImGui::IsItemActive();
        ImGui::EndDisabled();

        if (changed) {
            auto begin = ctx.beginEdit();
            if (!begin) {
                SDL_Log("%s", begin.error().c_str());
                return;
            }
            ctx.getValueMut() = value;
            ctx.markChanged();
        }

        if (ctx.isEditing() && !active) {
            ctx.endEdit();
        }
    }
};

} // namespace editor::ui