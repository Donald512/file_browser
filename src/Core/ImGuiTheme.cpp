#include "ImGuiTheme.h"
#include "imgui.h"

void ApplyWindows11DarkTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // --- 1. Layout Properties ---
    style.WindowRounding    = 7.0f;  // Signature Windows 11 rounded corners
    style.FrameRounding     = 4.0f;
    style.PopupRounding     = 6.0f;
    style.ChildRounding     = 4.0f;
    style.WindowBorderSize  = 1.0f;
    style.ChildBorderSize   = 1.0f;
    style.FrameBorderSize   = 1.0f;
    style.ItemSpacing       = ImVec2(8.0f, 6.0f);

    // --- 2. Color Tokens ---
    // Top-level background
    colors[ImGuiCol_WindowBg]             = ImVec4(0.098f, 0.098f, 0.098f, 1.00f); // #191919
    colors[ImGuiCol_ChildBg]              = ImVec4(0.10f,  0.10f,  0.10f,  1.00f); // #1A1A1A
    colors[ImGuiCol_PopupBg]              = ImVec4(0.14f,  0.14f,  0.14f,  0.98f); // #242424

    // Subtle divider lines and panel borders
    colors[ImGuiCol_Border]               = ImVec4(0.18f, 0.18f, 0.18f, 1.00f); // #2E2E2E
    colors[ImGuiCol_BorderShadow]         = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    // Text
    colors[ImGuiCol_Text]                 = ImVec4(0.90f, 0.90f, 0.90f, 1.00f); // #E6E6E6
    colors[ImGuiCol_TextDisabled]         = ImVec4(0.50f, 0.50f, 0.50f, 1.00f); // #808080

    // --- 3. Interactive States (Neutral Grays - No Accent Blue) ---
    colors[ImGuiCol_FrameBg]              = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]       = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBgActive]        = ImVec4(0.26f, 0.26f, 0.26f, 1.00f); // Bright gray click

    // Selectable Headers & Items (Active click is bright neutral gray)
    colors[ImGuiCol_Header]               = ImVec4(0.22f, 0.22f, 0.22f, 1.00f); // Selected item bg
    colors[ImGuiCol_HeaderHovered]        = ImVec4(0.20f, 0.20f, 0.20f, 1.00f); // Hovered item bg
    colors[ImGuiCol_HeaderActive]         = ImVec4(0.28f, 0.28f, 0.28f, 1.00f); // Bright gray click state

    // Buttons (Toolbar style)
    colors[ImGuiCol_Button]               = ImVec4(0.14f, 0.14f, 0.14f, 0.00f);
    colors[ImGuiCol_ButtonHovered]        = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
    colors[ImGuiCol_ButtonActive]         = ImVec4(0.28f, 0.28f, 0.28f, 1.00f); // Bright gray click

    // Scrollbars
    colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.10f, 0.10f, 0.10f, 0.00f);
    colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
}
