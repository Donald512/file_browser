#pragma once
#include "Types.h"
#include "imgui.h"
#include "AppContext.h"
#include "iconFilled.h"
#include "iconRegular.h"


namespace UI{
    constexpr ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse;
    void Render(AppContext& ctx);
}

namespace UI::Colors{
    constexpr ImVec4 WindowBackground(0.102f, 0.133f, 0.141f, 1.00f);
    constexpr ImVec4 TopBarBackground(0.0f, 0.055f, 0.071f, 1.00f);
    constexpr ImVec4 AddressBarBackground(0.22f, 0.22f, 0.22f, 1.00f);
    constexpr ImVec4 WindowForeground(0.098f, 0.098f, 0.098f, 1.00f);   // def need to fix these names
    constexpr ImVec4 SidebarBackground(0.14f, 0.14f, 0.14f, 1.00f);
    constexpr ImVec4 AccentBlue(0.0f, 0.47f, 0.84f, 1.0f);
}

namespace UI::Style{
    constexpr ImVec2 NoPadding(0.0f, 0.0f);
    constexpr ImVec2 AutoFillRemnantWindow(0.0f, 0.0f);
    constexpr f32 NoBorder = 0.0f;
    constexpr f32 NoRounding = 0.0f;
}

namespace TopBar{
    constexpr ImGuiWindowFlags Flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    constexpr f32 PaddingForIfWindowRestored = 8.0f;
    constexpr f32 Height            = 32.0f;
    constexpr f32 TotalBtnsWidth    = 145.0f; // 145 is the distance from the vertical bar to the end of the screen
    constexpr f32 BtnHeight         = Height;
    constexpr f32 BtnWidth          = 45.0f;
    constexpr f32 CloseBtnWidth     = 46.0f;
    /*  minimize and maximize buttons are 45 wide x 32 tall, but
    close button is 46 to account for the 1 px margin */
    void Render(AppContext& ctx);
}

namespace ToolBar{
    constexpr f32 Height             = 48.0f;
    constexpr f32 LeftPadding        = 3.0f;    // distance from left window edge
    constexpr f32 AddressToSearchGap = 8.0f;
    constexpr f32 RightPadding       = 11.0f;
    constexpr f32 AddressRatio       = 0.706f;
    constexpr f32 SearchRatio        = 0.294f;

    constexpr ImGuiWindowFlags Flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    void Render(AppContext& ctx);
}
// UI::Style::ToolBarLayout removed — it was a byte-for-byte duplicate of the five
// constants above. Anything that used ToolBarLayout::X now just uses ToolBar::X.

namespace NavBar{
    constexpr f32 Width        = 198.0f;
    constexpr f32 Height       = 48.0f;
    constexpr f32 BtnSize      = 32.0f;
    constexpr f32 XPadding     = 16.0f;
    constexpr f32 ButtonStartX = 8.0f;

    void Render(AppContext& ctx);
}

namespace AddressBar{
    constexpr f32 Height = 32.0f;
    constexpr f32 BarStartX = ToolBar::LeftPadding + NavBar::Width;   // was UI::Style::ToolBarLayout::LeftPadding
    constexpr f32 CrumbButtonHeight = 19.0f;
    void Render(AppContext& ctx);
}

namespace UI::Helpers{
    bool IconAndTextButton(const char* str_id, const char* icon, const char* label, const ImVec4& icon_color = UI::Colors::AccentBlue);
}

namespace CommandBar{
    constexpr f32 Height = 46.0f;
    void Render(AppContext& ctx);
}

namespace FileView{
    inline f32 IconSize   = 48.0f;
    constexpr f32 XPadding = 32.0f;
    void Render(AppContext& ctx);
}

