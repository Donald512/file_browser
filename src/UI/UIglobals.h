#pragma once
#include "BasicTypes.h"
#include "ImGuiHelpers.h"


inline constexpr f32 SidebarMinRatio = 100.0f / 1920.0f;
inline constexpr f32 SidebarMaxRatio = 1720.0f / 1920.0f;

inline bool g_sidebarOpen = true;

inline f32 g_sidebarRatio = 260.0f / 1920.0f;   // current
// inline f32 g_sidebarWidth = 0;     // where the animation currently is
// inline f32 g_sidebarHeaderWidth = 0.0f;    // current animated header width

inline AnimatedFloat g_sidebarWidthAnim;
inline AnimatedFloat g_sidebarHeaderWidthAnim;

inline f32 g_tabsScroll = 0.0f;     // actual render position
inline f32 g_tabsScrollTarget = 0.0f;
inline bool g_tabsScrollAnimating = false;

inline constexpr f32 TitlebarHeight = 32.0f;

// inline constexpr f32 TabMinWidth = 60.0f;
inline constexpr f32 TabMinWidth = 120.0f;
inline constexpr f32 TabMaxWidth = 200.0f;
inline constexpr f32 TabCornerRadius = 8.0f;

inline constexpr f32 TabsToNewTabGap = 30.0f;
// inline constexpr f32 NewTabToCaptionGap = 30.0f;
inline constexpr f32 NewTabToTabChevronGap = 30.0f;

// inline constexpr f32 tabChevronWidth = 45.0f;
inline constexpr f32 CaptionMinWidth = 45.0f;
inline constexpr f32 CaptionMaxWidth = 45.0f;
inline constexpr f32 CaptionCloseWidth = 46.0f;

inline constexpr f32 CaptionButtonsWidth = CaptionMinWidth + CaptionMaxWidth + CaptionCloseWidth;

inline HitTestRegistry g_HitTestRegistry;  // for titlebar wndproc
inline constexpr f32 titlebarBottomBorderThickness = 2.0f;


// for smoothly scrolling to current tab

template <typename T>
inline bool IsInRange(T min, T num, T max){
    return (min <= num) && (num <= max);
}



inline f32 Clampf(f32 value, f32 minValue, f32 maxValue){
    if (value < minValue) return minValue;
    else if (value > maxValue) return maxValue;
    return value;
}

inline f32 CalcSidebarWidth(f32 windowWidth){
    return Clampf(windowWidth * g_sidebarRatio, SidebarMinRatio * windowWidth, SidebarMaxRatio * windowWidth);
}

inline f32 CalcSidebarRatio(){  // !
    return Clampf(g_sidebarRatio, SidebarMinRatio, SidebarMaxRatio);
}

