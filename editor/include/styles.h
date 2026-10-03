#pragma once

#include <imgui.h>

namespace editor::styles {

inline ImVec4 color(unsigned int rgb, float alpha = 1.0f) {
    constexpr float toFloat = 1.0f / 255.0f;

    return ImVec4(
        static_cast<float>((rgb >> 16) & 0xFF) * toFloat,
        static_cast<float>((rgb >> 8) & 0xFF) * toFloat,
        static_cast<float>(rgb & 0xFF) * toFloat,
        alpha
    );
}

inline void setDarkPastelImGuiStyle() {
    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // Surfaces
    colors[ImGuiCol_WindowBg]             = color(0x0D121B);
    colors[ImGuiCol_ChildBg]              = color(0x0A0F17);
    colors[ImGuiCol_PopupBg]              = color(0x121925);
    colors[ImGuiCol_MenuBarBg]            = color(0x070A10);
    colors[ImGuiCol_DockingEmptyBg]       = color(0x070A10);

    // Text and borders
    colors[ImGuiCol_Text]                 = color(0xEAF3F5);
    colors[ImGuiCol_TextDisabled]         = color(0x8999A8);
    colors[ImGuiCol_Border]               = color(0x2A3542, 0.70f);
    colors[ImGuiCol_BorderShadow]         = color(0x000000, 0.00f);

    // Input fields
    colors[ImGuiCol_FrameBg]              = color(0x171F2C);
    colors[ImGuiCol_FrameBgHovered]       = color(0x202B3A);
    colors[ImGuiCol_FrameBgActive]        = color(0x2A394B);

    // Buttons
    colors[ImGuiCol_Button]               = color(0x171F2C);
    colors[ImGuiCol_ButtonHovered]        = color(0x223044);
    colors[ImGuiCol_ButtonActive]         = color(0x2B3D53);

    // Selected items and tree nodes
    colors[ImGuiCol_Header]               = color(0x182431);
    colors[ImGuiCol_HeaderHovered]        = color(0x223448);
    colors[ImGuiCol_HeaderActive]         = color(0x2B4054);

    // Tabs
    colors[ImGuiCol_Tab]                  = color(0x0A1019);
    colors[ImGuiCol_TabHovered]           = color(0x223448);
    colors[ImGuiCol_TabActive]            = color(0x1B2D3F);
    colors[ImGuiCol_TabUnfocused]         = color(0x080D14);
    colors[ImGuiCol_TabUnfocusedActive]   = color(0x141F2D);

    // Window titles
    colors[ImGuiCol_TitleBg]              = color(0x090E16);
    colors[ImGuiCol_TitleBgActive]        = color(0x14202E);
    colors[ImGuiCol_TitleBgCollapsed]     = color(0x070A10);

    // Scrollbars
    colors[ImGuiCol_ScrollbarBg]          = color(0x0A0F17);
    colors[ImGuiCol_ScrollbarGrab]        = color(0x283544);
    colors[ImGuiCol_ScrollbarGrabHovered] = color(0x394A5E);
    colors[ImGuiCol_ScrollbarGrabActive]  = color(0x52687E);

    // Accent controls
    colors[ImGuiCol_CheckMark]            = color(0x39C9ED);
    colors[ImGuiCol_SliderGrab]           = color(0x329FBE);
    colors[ImGuiCol_SliderGrabActive]     = color(0x39C9ED);

    // Resize grips
    colors[ImGuiCol_ResizeGrip]           = color(0x2A3542, 0.35f);
    colors[ImGuiCol_ResizeGripHovered]    = color(0x39C9ED, 0.60f);
    colors[ImGuiCol_ResizeGripActive]     = color(0x39C9ED, 0.90f);

    // Separators
    colors[ImGuiCol_Separator]            = color(0x2A3542, 0.65f);
    colors[ImGuiCol_SeparatorHovered]     = color(0x329FBE);
    colors[ImGuiCol_SeparatorActive]      = color(0x39C9ED);

    // Selection and docking
    colors[ImGuiCol_TextSelectedBg]       = color(0x39C9ED, 0.28f);
    colors[ImGuiCol_DragDropTarget]       = color(0x39C9ED, 0.90f);
    colors[ImGuiCol_DockingPreview]       = color(0x39C9ED, 0.28f);
    colors[ImGuiCol_ModalWindowDimBg]     = color(0x000000, 0.65f);

    // Shape and spacing
    style.WindowRounding = 7.0f;
    style.ChildRounding = 5.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 3.0f;

    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;
    style.PopupBorderSize = 1.0f;

    style.WindowPadding = ImVec2(14.0f, 12.0f);
    style.FramePadding = ImVec2(9.0f, 5.0f);
    style.ItemSpacing = ImVec2(9.0f, 7.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
    style.IndentSpacing = 20.0f;
    style.ScrollbarSize = 12.0f;
    style.GrabMinSize = 10.0f;
}

} // namespace editor::styles