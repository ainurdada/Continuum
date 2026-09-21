#pragma once

#include <cmath>
#include <limits>

#include <SDL3/SDL_log.h>
#include <imgui.h>

#include <math/public/g_math.h>

#include <ui/component_drawer.h>
#include <ui/modifierHelpers.h>

namespace editor::ui {

class FloatDrawer : public ComponentDrawer<float> {
    void draw(DrawContext<float>& ctx) override {
        using namespace engine::reflection::modifiers;

        float value = ctx.getValue();

        auto min = ctx.tryGetModifier<Min>();
        auto max = ctx.tryGetModifier<Max>();
        float minF = min ? math::toFloatSafe(min->value) : 0;
        float maxF = max ? math::toFloatSafe(max->value) : 0;

        ImGui::BeginDisabled(!ctx.canEdit());
        bool changed = ImGui::DragScalar((ctx.label() + "###Value").c_str(), ImGuiDataType_Float, &value, getDragSpeed(ctx.field()), min ? &minF : nullptr, max ? &maxF : nullptr, nullptr, ImGuiSliderFlags_NoRoundToFormat | ImGuiSliderFlags_AlwaysClamp);
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