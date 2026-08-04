#pragma once
#include <imgui.h>

namespace Theme {

    // Available themes in your application
    enum class Type {
        Windows11Dark,
        Windows11Light,
        Dracula,
        MidnightPurple,
        Theme2,
        ModernDark
    };

    struct Palette {
        ImVec4 Background;
        ImVec4 Surface;
        ImVec4 SurfaceHover;
        ImVec4 SurfaceActive;
        ImVec4 Border;
        ImVec4 Text;
        ImVec4 TextDisabled;
        ImVec4 Accent;
    };
    
    struct Metrics {
        float WindowRounding = 7.0f;
        float PopupRounding  = 6.0f;
        float FrameRounding  = 4.0f;
        float IconSize       = 16.0f;
        float IconSpacing    = 8.0f;
        float MenuItemPadX   = 8.0f;
        float MenuItemPadY   = 6.0f;
    };
    
    struct UITheme {
        Palette palette;
        Metrics metrics;
    };
    
    // Global active theme instance
    inline UITheme Current;

    // Functions to set up distinct theme values
    void SetTheme(Type themeType);
    void ApplyToImGui(ImGuiStyle& style = ImGui::GetStyle());
}

void ApplyWindows11DarkTheme();