#pragma once

#include <SDL3/SDL_log.h>
#include <imgui.h>

#include <ui/component_drawer.h>
#include <ui/modifierHelpers.h>

namespace editor::ui {

class DoubleDrawer : public ComponentDrawer<double> {
    void draw(DrawContext<double>& ctx) override {
        using namespace engine::reflection::modifiers;
        
        auto value = ctx.getValue();

        auto min = ctx.tryGetModifier<Min>();
        auto max = ctx.tryGetModifier<Max>();

        ImGui::BeginDisabled(!ctx.canEdit());
        bool changed = ImGui::DragScalar((ctx.label() + "###Value").c_str(), ImGuiDataType_Double, &value, getDragSpeed(ctx.field()), min ? &min->value : nullptr, max ? &max->value : nullptr, nullptr, ImGuiSliderFlags_NoRoundToFormat | ImGuiSliderFlags_AlwaysClamp);
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