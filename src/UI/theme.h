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
        ImVec4 Text;          // Primary / Bright Text
        ImVec4 TextMuted;     // Secondary / Label / Inactive Text
        ImVec4 TextDisabled;  // Fully Disabled / Hidden Text
        ImVec4 Accent;
    };
    
    struct Metrics {
        // Uniform corner radii stored as 4-component vectors (TL, TR, BR, BL)
        ImVec4 WindowRounding = ImVec4(7.0f, 7.0f, 7.0f, 7.0f);
        ImVec4 PopupRounding  = ImVec4(6.0f, 6.0f, 6.0f, 6.0f);
        ImVec4 FrameRounding  = ImVec4(4.0f, 4.0f, 4.0f, 4.0f);
        
        ImVec2 IconMetrics    = ImVec2(16.0f, 8.0f); // X: Size, Y: Spacing
        ImVec2 MenuItemPadding = ImVec2(8.0f, 6.0f);  // X: PadX, Y: PadY
    };
    
    struct UITheme {
        Palette palette;
        Metrics metrics;
    };
    
    // Global active theme instance
    inline UITheme Current;

    // Sets the active global theme data
    void SetTheme(Type themeType);
}

// Global standalone theme applicator function
void ApplyWindows11DarkTheme();