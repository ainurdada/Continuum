#pragma once

#include <SDL3/SDL_log.h>
#include <imgui.h>

#include <ui/component_drawer.h>

namespace editor::ui {

class BoolDrawer : public ComponentDrawer<bool> {
    void draw(DrawContext<bool>& ctx) override {
        bool value = ctx.getValue();

        ImGui::BeginDisabled(!ctx.canEdit());
        bool changed = ImGui::Checkbox((ctx.label() + "###Value").c_str(), &value);
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