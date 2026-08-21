#pragma once
#include "imgui.h"
#include "imgui_internal.h"
#include "ImGuiHelpers.h"
#include "UI/global.h"
#include "Hover.h"
#include "App.h"
#include "theme.h"
#include "Tab.h"

static HoverPanelState s_viewPanel;
static HoverPanelState s_sortPanel;


// const bool anchorHovered = anchorRect.Contains(ImGui::GetMousePos());
// const bool anchorClicked = anchorHovered && ImGui::IsMouseClicked(0);
// RenderIconButton(anchorRect, "ViewModeAnchor", ICON_REG_LIST, 4.0f*dpi, hoverCol, activeCol, textCol); // visual only

const char* GetLabelForViewMode(ViewMode viewMode){
    switch(viewMode){
        case ViewMode::List: return "List";
        case ViewMode::Small: return "Small";
        case ViewMode::Tiles: return "Tiles";
        case ViewMode::Details: return "Details";
        // case ViewMode::Icons: return "Icons";
    }
    return "Icons";
}

const char* GetIconForViewMode(ViewMode viewMode){
    switch(viewMode){
        case ViewMode::List: return ICON_REG_TEXT_BULLET_LIST;
        case ViewMode::Small: return ICON_REG_CIRCLE_SMALL;
        case ViewMode::Tiles: return ICON_REG_LINE_HORIZONTAL_5_20;
        case ViewMode::Details: return ICON_REG_APPS_LIST;
    }
    return ICON_REG_ICONS;
}

const char* GetLabelForSortMode(SortMode sortMode){
    switch(sortMode){
        case SortMode::Name: return "Name";
        case SortMode::Size: return "Size";
        case SortMode::DateModified: return "Date Modified";
        case SortMode::Type: return "Type";
    }
    return "None";
}

const char* GetLabelForSortDir(SortDirection sortDir){
    switch(sortDir){
        case SortDirection::Ascending: return "Ascending";
        case SortDirection::Descending: return "Descending";
    }
    return ICON_REG_ARROW_SORT_UP;
}
const char* GetIconForSortDir(SortDirection sortDir){
    switch(sortDir){
        case SortDirection::Ascending: return ICON_REG_ARROW_SORT_UP;
        case SortDirection::Descending: return ICON_REG_ARROW_SORT_DOWN;
    }
    return ICON_REG_ARROW_SORT_UP;
}

void RenderBottombar(f32 dpi, App& app){

    ImVec2 bbarPos = ImGui::GetWindowPos();
    ImVec2 bbarSize = ImGui::GetContentRegionAvail();
    
    ImDrawList* dl = ImGui::GetWindowDrawList();

    dl->ChannelsSplit(2);
    
    dl->ChannelsSetCurrent(1);
    
    f32 padX = 12.0f * dpi;
    const f32 distanceBetweeIconAndText = 6.0f * dpi;
    
    
    Tab& activeTab = app.window.GetActiveTab();
    
    ImFont* font = ImGui::GetFont();
    const f32 fontSize = ImGui::GetFontSize();
    const f32 lineHeight = ImGui::GetTextLineHeight();
    const f32 rounding = 8.0f * dpi;
    const ImU32& textCol = Theme::Current.palette.Text;
    // const ImU32& activeCol = Theme::Current.palette.SurfaceActive;
    const ImU32& hoverCol = Theme::Current.palette.SurfaceHover;
    const ImU32& accentCol = Theme::Current.palette.Accent;
    
    f32 cursorX = bbarPos.x + bbarSize.x - padX;
    
    auto& vs = activeTab.viewState;
    // --------------
    // - View 
    // --------------
    const char* viewIconText = GetIconForViewMode(vs.viewMode);
    const char* viewPlaceholderText = GetLabelForViewMode(vs.viewMode);
    
    const ImVec2 viewIconSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, viewIconText);
    const ImVec2 viewTextSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, viewPlaceholderText);
    
    const f32 viewElementWidth =  viewIconSize.x + distanceBetweeIconAndText + viewTextSize.x;

    cursorX -= viewElementWidth;

    ImRect viewRect(
        ImVec2(cursorX, bbarPos.y),
        ImVec2(cursorX + viewElementWidth, bbarPos.y + bbarSize.y)
    );

    f32 drawX = cursorX;
    drawX += DrawTextAtX(dl, drawX, viewRect, viewIconText, textCol).x + distanceBetweeIconAndText;
    DrawTextAtX(dl, drawX, viewRect, viewPlaceholderText, textCol);

    // --------------
    // - Sort 
    // --------------
    cursorX -= padX;    // padding between hem
    const char* sortPlaceholderText = GetLabelForSortMode(vs.sortMode);
    const char* sortIconText = GetIconForSortDir(vs.sortDir);
    
    const ImVec2 sortTextSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, sortPlaceholderText);
    const ImVec2 sortIconSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, sortIconText);
    
    const f32 sortElementWidth =  sortIconSize.x + distanceBetweeIconAndText + sortTextSize.x;
    
    cursorX -= sortElementWidth;
    
    ImRect sortRect(
        ImVec2(cursorX, bbarPos.y),
        ImVec2(cursorX + sortElementWidth, bbarPos.y + bbarSize.y)
    );
    
    drawX = cursorX;
    drawX += DrawTextAtX(dl, drawX, sortRect, sortPlaceholderText, textCol).x + distanceBetweeIconAndText;
    DrawTextAtX(dl, drawX, sortRect, sortIconText, textCol);
    
    // --------------
    // - Hidden
    // --------------
    cursorX -= padX;
    const char* hiddenIconText = vs.showHidden? ICON_REG_EYE : ICON_REG_EYE_OFF;

    const ImVec2 hiddenIconSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, hiddenIconText);
    
    cursorX -= hiddenIconSize.x;

    ImRect hiddenRect(
        ImVec2(cursorX, bbarPos.y),
        ImVec2(cursorX + hiddenIconSize.x, bbarPos.y + bbarSize.y)
    );

    if (RenderIconButton(hiddenRect, "hiddenToggle", hiddenIconText, 0, ImU32(0.0f), ImU32(0.0f))){
        activeTab.ToggleShowHidden();
    }


    dl->ChannelsSetCurrent(0);
    
    f32 finalLeftEdge = cursorX - padX;
    
    ImRect rightBottomGroup(
        ImVec2(finalLeftEdge, bbarPos.y),
        ImVec2(bbarPos.x + bbarSize.x, bbarPos.y + bbarSize.y)
    );
    
    ImDrawFlags cornerFlags = ImDrawFlags_RoundCornersTopLeft;
    dl->AddRectFilled(rightBottomGroup.Min, rightBottomGroup.Max, Theme::Current.palette.Surface, rounding, cornerFlags);
    dl->AddRect(rightBottomGroup.Min, rightBottomGroup.Max, Theme::Current.palette.Border, rounding, titlebarBottomBorderThickness * dpi, cornerFlags);

    dl->ChannelsMerge();

    const ImRect viewAnchorRect = viewRect;
    const bool viewAnchorHovered = viewAnchorRect.Contains(ImGui::GetMousePos());
    const bool viewAnchorClicked = viewAnchorHovered && ImGui::IsMouseClicked(0);

    if (BeginHoverPanel(s_viewPanel, "##ViewModePanel", viewAnchorRect, viewAnchorHovered, viewAnchorClicked, dpi)){
        ImDrawList* hoverDl = ImGui::GetWindowDrawList();

        const f32 viewRowPadY = 4.0f * dpi;
        const f32 rowW = 200.0f * dpi;
        const f32 rowH = lineHeight + 2 * viewRowPadY;

        ImGui::SetNextItemWidth(rowW);

        const f32 minSize = 32.0f;
        const f32 maxSize = 256.0f;
        f32 sz = vs.iconSize;
        f32 percent = ((sz - minSize) / (maxSize - minSize)) * 100.0f;

        if (ImGui::SliderFloat("##iconSize", &percent, 0.0f, 100.0f, "Icons %.0f%%")){
            // Map the percentage back to the 24-256 pixel scale
            sz = minSize + (percent / 100.0f) * (maxSize - minSize);
            vs.iconSize = sz;
            vs.viewMode = ViewMode::Icons;
        }

        auto viewRow = [&](ViewMode m, const char* icon, const char* label){
            ImVec2 p = ImGui::GetCursorScreenPos();
            ImRect r(p, ImVec2(p.x + rowW, p.y + rowH));
            ImGuiID id = ImGui::GetID(label);
            ImGui::ItemAdd(r, id);
            bool hov = false;
            bool held = false;
            if (ImGui::ButtonBehavior(r, id, &hov, &held)){
                vs.viewMode = m;
            }
            if (vs.viewMode == m)      hoverDl->AddRectFilled(r.Min, r.Max, accentCol, 4.0f * dpi);
            else if (hov)              hoverDl->AddRectFilled(r.Min, r.Max, hoverCol, 4.0f * dpi);

            DrawTextCenteredSingleLine(hoverDl, ImVec2(r.Min.x, r.Min.y), ImVec2(r.Min.x + 24.0f*dpi, r.Max.y), icon, textCol, 14.0f*dpi);
            DrawTextSingleLine(hoverDl, ImVec2(r.Min.x + 24.0f*dpi, r.Min.y), r.Max, label, textCol, ImVec2(0.0f, 0.5f));
            ImGui::Dummy(ImVec2(rowW, rowH)); // advance layout so AlwaysAutoResize measures height
        };

        constexpr ViewMode vm[] = {ViewMode::List, ViewMode::Details, ViewMode::Tiles, ViewMode::Small};
        for (auto& v : vm){
            viewRow(v, GetIconForViewMode(v), GetLabelForViewMode(v));
        }
        EndHoverPanel(s_viewPanel);
    }

    const ImRect sortAnchorRect = sortRect;
    const bool sortAnchorHovered = sortAnchorRect.Contains(ImGui::GetMousePos());
    const bool sortAnchorClicked = sortAnchorHovered && ImGui::IsMouseClicked(0);

    if (BeginHoverPanel(s_sortPanel, "##sortModePanel", sortAnchorRect, sortAnchorHovered, sortAnchorClicked, dpi)){
        ImDrawList* hoverDl = ImGui::GetWindowDrawList();

        const f32 sortRowPadY = 4.0f * dpi;
        const f32 rowW = 200.0f * dpi;
        const f32 rowH = lineHeight + 2 * sortRowPadY;

        ImGui::SetNextItemWidth(rowW);

        auto sortRow = [&](SortMode* m, SortDirection* d, const char* label){
            ImVec2 p = ImGui::GetCursorScreenPos();
            ImRect r(p, ImVec2(p.x + rowW, p.y + rowH));
            ImGuiID id = ImGui::GetID(label);
            ImGui::ItemAdd(r, id);
            bool hov = false;
            bool held = false;
            if (ImGui::ButtonBehavior(r, id, &hov, &held)){
                if (m){
                    vs.sortMode = *m;
                }
                else if (d){
                    vs.sortDir = *d;
                }
                app.QueueCommand({CmdType::ReSort, {}, 0, app.window.activeTabIndex, L""});
            }
            if (m){
                if (vs.sortMode == *m)      hoverDl->AddRectFilled(r.Min, r.Max, accentCol, 4.0f * dpi);
                else if (hov)              hoverDl->AddRectFilled(r.Min, r.Max, hoverCol, 4.0f * dpi);
            }
            else if (d){
                if (vs.sortDir == *d)      hoverDl->AddRectFilled(r.Min, r.Max, accentCol, 4.0f * dpi);
                else if (hov)              hoverDl->AddRectFilled(r.Min, r.Max, hoverCol, 4.0f * dpi);
            }

            DrawTextSingleLine(hoverDl, ImVec2(r.Min.x + 24.0f*dpi, r.Min.y), r.Max, label, textCol, ImVec2(0.0f, 0.5f));
            ImGui::Dummy(ImVec2(rowW, rowH)); // advance layout so AlwaysAutoResize measures height
        };

        SortMode sm[] = {SortMode::Name, SortMode::Size, SortMode::DateModified, SortMode::Type};
        for (auto& m : sm){
            sortRow(&m, nullptr, GetLabelForSortMode(m));
        }
        addSeparator(8.0f * dpi, rowW, 9.0f * dpi);
        
        SortDirection sd[] = {SortDirection::Ascending, SortDirection::Descending};
        for (auto& d : sd){
            sortRow(nullptr, &d, GetLabelForSortDir(d));
        }

        EndHoverPanel(s_sortPanel);
    }



}

