#pragma once

#include <SDL3/SDL_log.h>
#include <imgui.h>

#include <ui/component_drawer.h>

#include <math/public/g_math.h>
#include <scene/public/transform.h>

namespace editor::ui {

class TransformDrawer : public ComponentDrawer<engine::scene::Transform> {
    void draw(DrawContext<engine::scene::Transform>& ctx) override {
        auto position = ctx.getValue().position;
        auto rotation = math::degrees(ctx.getValue().rotation);
        auto scale = ctx.getValue().scale;

        ImGui::BeginDisabled(!ctx.canEdit());
        bool changed = false;
        bool isItemActive = false;

        bool posChanged = ImGui::DragFloat3("Position", &position.x, 0.1f);
        isItemActive |= ImGui::IsItemActive();

        bool rotChanged = ImGui::DragFloat3("Rotation", &rotation.x, 0.1f);
        isItemActive |= ImGui::IsItemActive();

        bool scaleChanged = ImGui::DragFloat3("Scale", &scale.x, 0.1f);
        isItemActive |= ImGui::IsItemActive();

        ImGui::EndDisabled();

        if (posChanged || rotChanged || scaleChanged) {
            auto begin = ctx.beginEdit();
            if (!begin) {
                SDL_Log("%s", begin.error().c_str());
                return;
            }

            if (posChanged)
                ctx.getValueMut().position = position;
            if (rotChanged)
                ctx.getValueMut().rotation = math::radians(rotation);
            if (scaleChanged)
                ctx.getValueMut().scale = scale;
            ctx.markChanged();
        }

        if (ctx.isEditing() && !isItemActive) {
            ctx.endEdit();
        }
    }
};

} // namespace editor::ui