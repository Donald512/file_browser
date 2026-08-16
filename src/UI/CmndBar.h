#pragma once
#include "BasicTypes.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "App.h"
#include "ImGuiHelpers.h"
#include "global.h"
#include "theme.h"
#include "iconRegular.h"

inline void RenderCommandbar(f32 dpi, App& app){
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (!window || window->SkipItems) return;
    ImDrawList* dl = window->DrawList;

    const ImVec2 winPos = window->Pos;
    const ImVec2 winSize = window->Size;
    const f32 barH = TitlebarHeight * dpi;
    const f32 pad  = 4.0f * dpi;

    const ImU32 hoverCol  = ToImU32(Theme::Current.palette.SurfaceHover);
    const ImU32 activeCol = ToImU32(Theme::Current.palette.SurfaceActive);
    const ImU32 textCol   = ToImU32(Theme::Current.palette.Text);        // was SurfaceActive!
    const ImU32 mutedCol  = ToImU32(Theme::Current.palette.TextDisabled);

    const ImRect barRect(winPos, ImVec2(winPos.x + winSize.x, winPos.y + barH));
    const f32 btnSize = barH - pad * 2.0f;
    const f32 startY  = barRect.Min.y + pad;
    f32 currentX = barRect.Min.x + pad;   // <-- was 0

    auto cmdBarButton = [&](const char* id, const char* icon, bool enabled) -> bool {
        const ImRect r(ImVec2(currentX, startY), ImVec2(currentX + btnSize, startY + btnSize));
        currentX += btnSize;
        return RenderIconButton(r, id, icon, 8.0f * dpi, hoverCol, activeCol, enabled ? textCol : mutedCol, !enabled);
    };

    const Tab& tab = app.window.GetActiveTab();
    if (cmdBarButton("NavGoBack",    ICON_REG_ARROW_LEFT,      tab.CanGoBack())){
        app.QueueCommand({ CmdType::GoBack, {}, 0, 0, L"" });
    }
    if (cmdBarButton("NavGoForward", ICON_REG_ARROW_RIGHT,     tab.CanGoForward())) {
        app.QueueCommand({ CmdType::GoForward, {}, 0, 0, L"" });
    }
    if (cmdBarButton("NavGoParent",  ICON_REG_ARROW_UP,        tab.CanGoParent()))  { 
        app.QueueCommand({ CmdType::GoParent, {}, 0, 0, L"" });
    }
    if (cmdBarButton("NavRefresh",   ICON_REG_ARROW_CLOCKWISE, true))               {
        app.QueueCommand({ CmdType::Refresh, {}, 0, 0, L"" });
    }
    if (cmdBarButton("HistoryDropdown",   ICON_REG_CHEVRON_DOWN, true)){ /* Open Dropdown showing visited tabs, like a history, but listing everything in history vector, not just up to currentIndex */ 
    }

    if (cmdBarButton("VertSep1", "|", false)){}
    if (cmdBarButton("SaveViewSettings", ICON_REG_LOCK_OPEN, true)){}   // will change when locked, so need a bool later
    if (cmdBarButton("BookmarkTab", ICON_REG_BOOKMARK, true)){}    // will change to BOOKMARK_OFF if already bookmarked
    if (cmdBarButton("VertSep2", "|", false)){}



    // --                                                              
    // Breadcrumbs
    // --                                                              
    const f32 crumbGap = 4.0f * dpi;
    u64 i = 0;
    const f32 fontSize = ImGui::GetFontSize();
    ImFont* font = ImGui::GetFont();
    const ImVec2 mousePos = ImGui::GetMousePos();
    const f32 btnRadius = 4.0f * dpi;
    const ImVec2 chevSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, ICON_REG_CHEVRON_RIGHT);
    
    const auto& crumbs = app.window.GetActiveTab().breadcrumbs.crumbs;
    const size_t numCrumbs = crumbs.size();
    const f32 rightBtnX = barRect.Max.x - pad - btnSize; // Reserve space for "MoreOptions"
 
    const f32 leftPad = 8.0f * dpi;
    const f32 rightPad = leftPad;

    const bool drawLastChevron = app.window.GetActiveTab().breadcrumbs.hasSubFolders;
    for (const auto& crumb : crumbs){
        ImGui::PushID((int)i);
        const bool isLast = (i == numCrumbs - 1);
        ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, crumb.displayName.c_str());
        
        // Simple overflow handling: clamp the last crumb if it hits the right button
        f32 maxTextW = rightBtnX - currentX - chevSize.x - crumbGap;
        bool isClamped = false;
        if (isLast && textSize.x > maxTextW && maxTextW > 0) {
            textSize.x = maxTextW;
            isClamped = true;
        }

        const ImRect textRect(ImVec2(currentX, startY), ImVec2(currentX + textSize.x + leftPad + rightPad, startY + btnSize));
        const ImRect chevRect(ImVec2(textRect.Max.x, startY), ImVec2(textRect.Max.x + btnSize, startY + btnSize));
        const ImRect fullRect(textRect.Min, chevRect.Max);
        
        // 1. Hit testing (makes them clickable!)
        ImGuiID textId = window->GetID((void*)(intptr_t)(i + 1000));
        Interaction textInteract = MakeInteractive(textId, textRect);

        ImGuiID chevId = window->GetID((void*)(intptr_t)(i + 2000));
        Interaction chevInteract = MakeInteractive(chevId, chevRect);

        ImU32 crumbTextCol = isLast ? textCol : mutedCol;

        if (chevInteract.hovered && (!isLast || drawLastChevron)){
            dl->AddRectFilled(fullRect.Min, fullRect.Max, hoverCol, btnRadius);
        }
        else if (textInteract.hovered){
            dl->AddRectFilled(textRect.Min, textRect.Max, hoverCol, btnRadius);
        }
            
        
        
        DrawTextCenteredSingleLine(dl, textRect.Min, textRect.Max, crumb.displayName.c_str() ,crumbTextCol);
        if (!isLast || drawLastChevron){
            DrawTextCenteredSingleLine(dl, chevRect.Min, chevRect.Max, ICON_REG_CHEVRON_RIGHT ,crumbTextCol);
        }

        // 6. Handle Navigation Clicks
        if (textInteract.pressed && !isLast) {
            app.QueueCommand({ CmdType::GoTo, crumb.pidl.Clone(), crumb.hash, app.window.activeTabIndex, L"" });
        }
        
        if (chevInteract.pressed) {
            // TODO: Open popup for this crumb's subfolders
            // ImGui::OpenPopup(...);
        }

        i++;
        currentX += textSize.x + chevSize.x + crumbGap + leftPad + rightPad; // Advance by text + chevron + gap
        ImGui::PopID();
    }

    // right-aligned ⋮ button
    RenderIconButton(ImRect(ImVec2(rightBtnX, startY), ImVec2(rightBtnX + btnSize, startY + btnSize)),
        "MoreOptions", ICON_REG_MORE_VERTICAL, 8.0f * dpi,
        hoverCol, activeCol, textCol);

    ImGui::Dummy(ImVec2(0.0f, barH));   // reserve bar height in Content child
}