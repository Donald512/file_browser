#include "ImGuiTheme.h"
#include "imgui.h"

void ApplyWindows11DarkTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    style.WindowRounding    = 7.0f;  // Signature Windows 11 rounded corners
    style.FrameRounding     = 4.0f;
    style.PopupRounding     = 6.0f;
    style.ChildRounding     = 4.0f;
    style.WindowBorderSize  = 1.0f;
    style.ChildBorderSize   = 1.0f;
    style.FrameBorderSize   = 1.0f;
    style.ItemSpacing       = ImVec2(8.0f, 6.0f);

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

namespace Theme {

    void SetTheme(Type themeType) {
        switch (themeType) {

            case Type::Windows11Dark: {
                Current.palette.Background    = ImVec4(0.12f, 0.12f, 0.12f, 0.98f);
                Current.palette.Surface       = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
                Current.palette.SurfaceHover  = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
                Current.palette.SurfaceActive = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
                Current.palette.Border        = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
                Current.palette.Text          = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
                Current.palette.TextDisabled  = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
                Current.palette.Accent        = ImVec4(0.00f, 0.47f, 0.84f, 1.00f); 

                Current.metrics.WindowRounding = 8.0f;
                Current.metrics.FrameRounding  = 4.0f;
            } break;

            case Type::Windows11Light: {
                Current.palette.Background    = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
                Current.palette.Surface       = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
                Current.palette.SurfaceHover  = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
                Current.palette.SurfaceActive = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
                Current.palette.Border        = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
                Current.palette.Text          = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
                Current.palette.TextDisabled  = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
                Current.palette.Accent        = ImVec4(0.00f, 0.47f, 0.84f, 1.00f);

                Current.metrics.WindowRounding = 8.0f;
                Current.metrics.FrameRounding  = 4.0f;
            } break;

            case Type::MidnightPurple: {
                Current.palette.Background    = ImVec4(0.08f, 0.06f, 0.12f, 0.98f);
                Current.palette.Surface       = ImVec4(0.13f, 0.10f, 0.20f, 1.00f);
                Current.palette.SurfaceHover  = ImVec4(0.20f, 0.15f, 0.30f, 1.00f);
                Current.palette.SurfaceActive = ImVec4(0.26f, 0.18f, 0.40f, 1.00f);
                Current.palette.Border        = ImVec4(0.25f, 0.20f, 0.35f, 1.00f);
                Current.palette.Text          = ImVec4(0.92f, 0.90f, 0.98f, 1.00f);
                Current.palette.TextDisabled  = ImVec4(0.50f, 0.45f, 0.60f, 1.00f);
                Current.palette.Accent        = ImVec4(0.65f, 0.30f, 0.95f, 1.00f);
                
                Current.metrics.WindowRounding = 10.0f;
                Current.metrics.FrameRounding  = 6.0f;
            } break;
                
            case Type::Dracula: {
                Current.palette.Background    = ImVec4(0.16f, 0.16f, 0.21f, 0.98f); // #282a36
                Current.palette.Surface       = ImVec4(0.27f, 0.28f, 0.35f, 1.00f); // #44475a
                Current.palette.SurfaceHover  = ImVec4(0.33f, 0.35f, 0.44f, 1.00f); 
                Current.palette.SurfaceActive = ImVec4(0.38f, 0.40f, 0.50f, 1.00f); 
                Current.palette.Border        = ImVec4(0.38f, 0.40f, 0.50f, 1.00f); 
                Current.palette.Text          = ImVec4(0.97f, 0.97f, 0.95f, 1.00f); // #f8f8f2
                Current.palette.TextDisabled  = ImVec4(0.38f, 0.44f, 0.64f, 1.00f); // #6272a4
                Current.palette.Accent        = ImVec4(0.74f, 0.46f, 1.00f, 1.00f); // #bd93f9
                
                Current.metrics.WindowRounding = 6.0f;
                Current.metrics.FrameRounding  = 4.0f;
            } break;

            case Type::Theme2:{
                Current.palette.Background   = ImVec4(0.098f, 0.098f, 0.098f, 1.00f);
                Current.palette.Surface      = ImVec4(0.14f,  0.14f,  0.14f,  0.98f);
                Current.palette.SurfaceHover = ImVec4(1.0f, 1.0f, 1.0f, 0.08f);
                Current.palette.SurfaceActive= ImVec4(0.0f, 0.47f, 0.84f, 0.55f);
                Current.palette.Border       = ImVec4(1.0f, 1.0f, 1.0f, 0.08f);
                Current.palette.Text         = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
                Current.palette.TextDisabled = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
                Current.palette.Accent       = ImVec4(0.0f, 0.47f, 0.84f, 1.00f);
            }

            case Type::ModernDark: {
                Current.palette.Background    = ImVec4(0.06f, 0.06f, 0.07f, 0.94f); // window_background (16, 16, 18)
                Current.palette.Surface       = ImVec4(0.09f, 0.09f, 0.11f, 1.00f); // selection_background (24, 25, 29)
                Current.palette.SurfaceHover  = ImVec4(0.16f, 0.18f, 0.20f, 1.00f); // checkbox_background (42, 46, 50)
                Current.palette.SurfaceActive = ImVec4(0.11f, 0.11f, 0.11f, 1.00f); // window_stroke (27, 27, 29)
                Current.palette.Border        = ImVec4(0.11f, 0.11f, 0.11f, 1.00f); // stroke
                Current.palette.Text          = ImVec4(0.79f, 0.79f, 0.80f, 1.00f); // child_label (201, 201, 203)
                Current.palette.TextDisabled  = ImVec4(0.53f, 0.53f, 0.53f, 1.00f); // selection_label_inactive
                Current.palette.Accent        = ImVec4(0.92f, 0.17f, 0.14f, 1.00f); // accent (235, 44, 35)

                Current.metrics.WindowRounding = 8.0f;
                Current.metrics.FrameRounding  = 2.0f; // widgets_rounding
                Current.metrics.PopupRounding  = 8.0f;
                Current.metrics.MenuItemPadX   = 10.0f; // widgets_padding
                Current.metrics.MenuItemPadY   = 10.0f;
            } break;
        }

        ApplyToImGui();
    }

    void ApplyToImGui(ImGuiStyle& style) {
        style.WindowRounding = Current.metrics.WindowRounding;
        style.PopupRounding  = Current.metrics.PopupRounding;
        style.FrameRounding  = Current.metrics.FrameRounding;

        style.Colors[ImGuiCol_WindowBg]      = Current.palette.Background;
        style.Colors[ImGuiCol_ChildBg]       = Current.palette.Surface;
        style.Colors[ImGuiCol_PopupBg]       = Current.palette.Surface;
        style.Colors[ImGuiCol_Header]        = Current.palette.Surface;
        style.Colors[ImGuiCol_HeaderHovered] = Current.palette.SurfaceHover;
        style.Colors[ImGuiCol_HeaderActive]  = Current.palette.SurfaceActive;
        style.Colors[ImGuiCol_Button]        = Current.palette.Surface;
        style.Colors[ImGuiCol_ButtonHovered] = Current.palette.SurfaceHover;
        style.Colors[ImGuiCol_ButtonActive]  = Current.palette.SurfaceActive;
        style.Colors[ImGuiCol_Border]        = Current.palette.Border;
        style.Colors[ImGuiCol_Text]          = Current.palette.Text;
        style.Colors[ImGuiCol_TextDisabled]  = Current.palette.TextDisabled;
        style.Colors[ImGuiCol_CheckMark]     = Current.palette.Accent;
        style.Colors[ImGuiCol_SliderGrab]    = Current.palette.Accent;
    }
}