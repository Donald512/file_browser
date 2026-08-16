#include "Theme.h"

void ApplyWindows11DarkTheme() {
    Theme::SetTheme(Theme::Type::Windows11Dark);
}

namespace Theme {

    void SetTheme(Type themeType) {
        switch (themeType) {

            case Type::Windows11Dark: {
                Current.palette.Background    = ImVec4(0.12f, 0.12f, 0.12f, 0.98f); // #1F1F1F
                Current.palette.Surface       = ImVec4(0.16f, 0.16f, 0.16f, 1.00f); // #292929
                Current.palette.SurfaceHover  = ImVec4(0.22f, 0.22f, 0.22f, 1.00f); 
                Current.palette.SurfaceActive = ImVec4(0.26f, 0.26f, 0.26f, 1.00f); 
                Current.palette.Border        = ImVec4(0.22f, 0.22f, 0.22f, 1.00f); 
                
                Current.palette.Text         = ImVec4(1.00f, 1.00f, 1.00f, 1.00f); // Pure White (High contrast)
                Current.palette.TextMuted    = ImVec4(0.60f, 0.63f, 0.67f, 1.00f); // Slate Gray / Dull Silver (~#9AA0A6)
                Current.palette.TextDisabled = ImVec4(0.40f, 0.42f, 0.45f, 1.00f); // Darker Muted Gray (~#666B70)
                                
                Current.palette.Accent        = ImVec4(0.00f, 0.47f, 0.84f, 1.00f); 

                Current.metrics.WindowRounding = ImVec4(8.0f, 8.0f, 8.0f, 8.0f);
                Current.metrics.FrameRounding  = ImVec4(4.0f, 4.0f, 4.0f, 4.0f);
                Current.metrics.PopupRounding  = ImVec4(6.0f, 6.0f, 6.0f, 6.0f);
            } break;

            case Type::Windows11Light: {
                Current.palette.Background    = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
                Current.palette.Surface       = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
                Current.palette.SurfaceHover  = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
                Current.palette.SurfaceActive = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
                Current.palette.Border        = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
                
                Current.palette.Text          = ImVec4(0.06f, 0.06f, 0.06f, 1.00f); // Deep Black
                Current.palette.TextMuted     = ImVec4(0.10f, 0.10f, 0.10f, 1.00f); // Dark Gray
                Current.palette.TextDisabled  = ImVec4(0.60f, 0.60f, 0.60f, 1.00f); // Light Muted Gray
                
                Current.palette.Accent        = ImVec4(0.00f, 0.47f, 0.84f, 1.00f);

                Current.metrics.WindowRounding = ImVec4(8.0f, 8.0f, 8.0f, 8.0f);
                Current.metrics.FrameRounding  = ImVec4(4.0f, 4.0f, 4.0f, 4.0f);
                Current.metrics.PopupRounding  = ImVec4(6.0f, 6.0f, 6.0f, 6.0f);
            } break;

            case Type::MidnightPurple: {
                Current.palette.Background    = ImVec4(0.08f, 0.06f, 0.12f, 0.98f);
                Current.palette.Surface       = ImVec4(0.13f, 0.10f, 0.20f, 1.00f);
                Current.palette.SurfaceHover  = ImVec4(0.20f, 0.15f, 0.30f, 1.00f);
                Current.palette.SurfaceActive = ImVec4(0.26f, 0.18f, 0.40f, 1.00f);
                Current.palette.Border        = ImVec4(0.25f, 0.20f, 0.35f, 1.00f);
                
                Current.palette.Text          = ImVec4(1.00f, 0.98f, 1.00f, 1.00f); // Light Purple/White
                Current.palette.TextMuted     = ImVec4(0.92f, 0.90f, 0.98f, 1.00f); // Soft Purple-Gray
                Current.palette.TextDisabled  = ImVec4(0.50f, 0.45f, 0.60f, 1.00f); // Dark Muted Purple
                
                Current.palette.Accent        = ImVec4(0.65f, 0.30f, 0.95f, 1.00f);
                
                Current.metrics.WindowRounding = ImVec4(10.0f, 10.0f, 10.0f, 10.0f);
                Current.metrics.FrameRounding  = ImVec4(6.0f, 6.0f, 6.0f, 6.0f);
                Current.metrics.PopupRounding  = ImVec4(6.0f, 6.0f, 6.0f, 6.0f);
            } break;

            case Type::Dracula: {
                Current.palette.Background    = ImVec4(0.16f, 0.16f, 0.21f, 0.98f); // #282a36
                Current.palette.Surface       = ImVec4(0.27f, 0.28f, 0.35f, 1.00f); // #44475a
                Current.palette.SurfaceHover  = ImVec4(0.33f, 0.35f, 0.44f, 1.00f); 
                Current.palette.SurfaceActive = ImVec4(0.38f, 0.40f, 0.50f, 1.00f); 
                Current.palette.Border        = ImVec4(0.38f, 0.40f, 0.50f, 1.00f); 
                
                Current.palette.Text          = ImVec4(1.00f, 1.00f, 1.00f, 1.00f); // White
                Current.palette.TextMuted     = ImVec4(0.97f, 0.97f, 0.95f, 1.00f); // #f8f8f2
                Current.palette.TextDisabled  = ImVec4(0.38f, 0.44f, 0.64f, 1.00f); // #6272a4
                
                Current.palette.Accent        = ImVec4(0.74f, 0.46f, 1.00f, 1.00f); // #bd93f9
                
                Current.metrics.WindowRounding = ImVec4(6.0f, 6.0f, 6.0f, 6.0f);
                Current.metrics.FrameRounding  = ImVec4(4.0f, 4.0f, 4.0f, 4.0f);
                Current.metrics.PopupRounding  = ImVec4(6.0f, 6.0f, 6.0f, 6.0f);
            } break;

            case Type::Theme2: {
                Current.palette.Background    = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
                Current.palette.Surface       = ImVec4(0.14f, 0.14f, 0.14f, 0.98f);
                Current.palette.SurfaceHover  = ImVec4(1.00f, 1.00f, 1.00f, 0.08f);
                Current.palette.SurfaceActive = ImVec4(0.00f, 0.47f, 0.84f, 0.55f);
                Current.palette.Border        = ImVec4(1.00f, 1.00f, 1.00f, 0.08f);
                
                Current.palette.Text          = ImVec4(1.00f, 1.00f, 1.00f, 1.00f); // Crisp White
                Current.palette.TextMuted     = ImVec4(0.90f, 0.90f, 0.90f, 1.00f); // Off-white
                Current.palette.TextDisabled  = ImVec4(0.50f, 0.50f, 0.50f, 1.00f); // Gray
                
                Current.palette.Accent        = ImVec4(0.00f, 0.47f, 0.84f, 1.00f);

                Current.metrics.WindowRounding = ImVec4(7.0f, 7.0f, 7.0f, 7.0f);
                Current.metrics.FrameRounding  = ImVec4(4.0f, 4.0f, 4.0f, 4.0f);
                Current.metrics.PopupRounding  = ImVec4(6.0f, 6.0f, 6.0f, 6.0f);
            } break;

            case Type::ModernDark: {
                Current.palette.Background    = ImVec4(0.06f, 0.06f, 0.07f, 0.94f); // (16, 16, 18)
                Current.palette.Surface       = ImVec4(0.09f, 0.09f, 0.11f, 1.00f); // (24, 25, 29)
                Current.palette.SurfaceHover  = ImVec4(0.16f, 0.18f, 0.20f, 1.00f); // (42, 46, 50)
                Current.palette.SurfaceActive = ImVec4(0.11f, 0.11f, 0.11f, 1.00f); // (27, 27, 29)
                Current.palette.Border        = ImVec4(0.11f, 0.11f, 0.11f, 1.00f); 
                
                Current.palette.Text          = ImVec4(0.94f, 0.94f, 0.94f, 1.00f); 
                Current.palette.TextMuted     = ImVec4(0.79f, 0.79f, 0.80f, 1.00f); // (201, 201, 203)
                Current.palette.TextDisabled  = ImVec4(0.53f, 0.53f, 0.53f, 1.00f); 
                
                Current.palette.Accent        = ImVec4(0.92f, 0.17f, 0.14f, 1.00f); // (235, 44, 35)

                Current.metrics.WindowRounding = ImVec4(8.0f, 8.0f, 8.0f, 8.0f);
                Current.metrics.FrameRounding  = ImVec4(2.0f, 2.0f, 2.0f, 2.0f);
                Current.metrics.PopupRounding  = ImVec4(8.0f, 8.0f, 8.0f, 8.0f);
            } break;
        }
    }
}