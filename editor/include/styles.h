#pragma once

#include <imgui.h>

// Dark inspector theme based on the supplied UI reference.
// Copy this file's contents into editor/include/styles.h to keep the existing
// include path and setDarkPastelImGuiStyle() call unchanged.
namespace editor::styles {

namespace palette {
constexpr unsigned int Canvas          = 0x141414;
constexpr unsigned int Panel           = 0x1E1E1E;
constexpr unsigned int PanelRaised     = 0x242424;
constexpr unsigned int Control         = 0x303030;
constexpr unsigned int ControlHovered  = 0x3A3A3A;
constexpr unsigned int ControlActive   = 0x515151;
constexpr unsigned int Border          = 0x292929;
constexpr unsigned int BorderStrong    = 0x383838;
constexpr unsigned int Text            = 0xE3E3E3;
constexpr unsigned int TextMuted       = 0x9C9C9C;
constexpr unsigned int Token           = 0x20506A;
constexpr unsigned int TokenHovered    = 0x286582;
constexpr unsigned int TokenActive     = 0x337B9B;
constexpr unsigned int Cyan            = 0x59C7EB;
} // namespace palette

inline ImVec4 color(unsigned int rgb, float alpha = 1.0f) {
    return ImVec4(
        static_cast<float>((rgb >> 16) & 0xFF) / 255.0f,
        static_cast<float>((rgb >> 8) & 0xFF) / 255.0f,
        static_cast<float>(rgb & 0xFF) / 255.0f,
        alpha);
}

inline void setInspectorImGuiStyle() {
    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* c = style.Colors;

    // Surface hierarchy: canvas -> rounded panels -> recessed controls.
    c[ImGuiCol_WindowBg]             = color(palette::Panel);
    c[ImGuiCol_ChildBg]              = color(palette::Panel);
    c[ImGuiCol_PopupBg]              = color(palette::PanelRaised, 0.98f);
    c[ImGuiCol_MenuBarBg]            = color(palette::Canvas);
    c[ImGuiCol_DockingEmptyBg]       = color(palette::Canvas);
    c[ImGuiCol_TitleBg]              = color(palette::Canvas);
    c[ImGuiCol_TitleBgActive]        = color(palette::Panel);
    c[ImGuiCol_TitleBgCollapsed]     = color(palette::Canvas);

    c[ImGuiCol_Text]                 = color(palette::Text);
    c[ImGuiCol_TextDisabled]         = color(palette::TextMuted);
    c[ImGuiCol_Border]               = color(palette::BorderStrong, 0.52f);
    c[ImGuiCol_BorderShadow]         = color(0x000000, 0.0f);
    c[ImGuiCol_Separator]            = color(palette::BorderStrong, 0.65f);
    c[ImGuiCol_SeparatorHovered]     = color(palette::ControlActive);
    c[ImGuiCol_SeparatorActive]      = color(palette::TokenHovered);

    c[ImGuiCol_FrameBg]              = color(palette::Control);
    c[ImGuiCol_FrameBgHovered]       = color(palette::ControlHovered);
    c[ImGuiCol_FrameBgActive]        = color(palette::ControlActive);
    c[ImGuiCol_Button]               = color(palette::Control);
    c[ImGuiCol_ButtonHovered]        = color(palette::ControlHovered);
    c[ImGuiCol_ButtonActive]         = color(palette::ControlActive);
    c[ImGuiCol_Header]               = color(palette::Control);
    c[ImGuiCol_HeaderHovered]        = color(palette::ControlHovered);
    c[ImGuiCol_HeaderActive]         = color(palette::ControlActive);

    c[ImGuiCol_Tab]                  = color(palette::PanelRaised);
    c[ImGuiCol_TabHovered]           = color(palette::ControlHovered);
    c[ImGuiCol_TabActive]            = color(palette::Control);
    c[ImGuiCol_TabUnfocused]         = color(palette::Canvas);
    c[ImGuiCol_TabUnfocusedActive]   = color(palette::Panel);

    c[ImGuiCol_ScrollbarBg]          = color(palette::Panel);
    c[ImGuiCol_ScrollbarGrab]        = color(0x414141);
    c[ImGuiCol_ScrollbarGrabHovered] = color(0x555555);
    c[ImGuiCol_ScrollbarGrabActive]  = color(0x686868);
    c[ImGuiCol_CheckMark]            = color(palette::Cyan);
    c[ImGuiCol_SliderGrab]           = color(palette::TokenHovered);
    c[ImGuiCol_SliderGrabActive]     = color(palette::Cyan);
    c[ImGuiCol_ResizeGrip]           = color(palette::ControlActive, 0.35f);
    c[ImGuiCol_ResizeGripHovered]    = color(palette::TokenHovered, 0.70f);
    c[ImGuiCol_ResizeGripActive]     = color(palette::Cyan, 0.85f);

    c[ImGuiCol_TextSelectedBg]       = color(palette::TokenHovered, 0.55f);
    c[ImGuiCol_NavHighlight]         = color(palette::Cyan, 0.70f);
    c[ImGuiCol_DragDropTarget]       = color(palette::Cyan, 0.90f);
    c[ImGuiCol_DockingPreview]       = color(palette::TokenHovered, 0.55f);
    c[ImGuiCol_ModalWindowDimBg]     = color(0x000000, 0.72f);

    c[ImGuiCol_TableHeaderBg]        = color(palette::PanelRaised);
    c[ImGuiCol_TableBorderStrong]    = color(palette::BorderStrong);
    c[ImGuiCol_TableBorderLight]     = color(palette::Border);
    c[ImGuiCol_TableRowBg]           = color(palette::Panel, 0.0f);
    c[ImGuiCol_TableRowBgAlt]        = color(0xFFFFFF, 0.025f);

    style.Alpha                    = 1.0f;
    style.WindowPadding            = ImVec2(12.0f, 11.0f);
    style.FramePadding             = ImVec2(8.0f, 5.0f);
    style.ItemSpacing              = ImVec2(7.0f, 7.0f);
    style.ItemInnerSpacing         = ImVec2(5.0f, 4.0f);
    style.IndentSpacing            = 16.0f;
    style.ScrollbarSize            = 10.0f;
    style.GrabMinSize              = 9.0f;

    style.WindowRounding           = 8.0f;
    style.ChildRounding            = 8.0f;
    style.FrameRounding            = 5.0f;
    style.PopupRounding            = 6.0f;
    style.ScrollbarRounding        = 6.0f;
    style.GrabRounding             = 4.0f;
    style.TabRounding              = 5.0f;

    style.WindowBorderSize         = 1.0f;
    style.ChildBorderSize          = 1.0f;
    style.PopupBorderSize          = 1.0f;
    style.FrameBorderSize          = 0.0f;
    style.TabBorderSize            = 0.0f;
}

// Optional: wrap a Button() in this pair for the blue token chips in the image.
inline void pushTokenStyle() {
    ImGui::PushStyleColor(ImGuiCol_Button,        color(palette::Token));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color(palette::TokenHovered));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  color(palette::TokenActive));
    ImGui::PushStyleColor(ImGuiCol_Text,          color(0xF0F8FC));
}

inline void popTokenStyle() {
    ImGui::PopStyleColor(4);
}

// Compatibility with the editor's existing initialization call.
inline void setDarkPastelImGuiStyle() {
    setInspectorImGuiStyle();
}

} // namespace editor::styles
