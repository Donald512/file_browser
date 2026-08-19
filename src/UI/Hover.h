#pragma once
#include "imgui.h"
#include "imgui_internal.h"
#include "BasicTypes.h"
#include "theme.h"
#include "ImGuiHelpers.h"
#include "iconRegular.h"

// One static instance per panel. The engine handles:
//  - hover-intent open (delay so it doesn't flicker on pass-through)
//  - grace-period close (lets the mouse travel anchor -> panel)
//  - click anchor = pin (floating, movable, stays until X / Esc / outside click)
struct HoverPanelState {
    bool   open         = false;
    bool   pinned       = false;
    f32    hoverTime    = 0.0f;
    f32    leaveTime    = 0.0f;
    bool   panelHovered = false;   // cached from previous frame
    ImVec2 lastSize     = ImVec2(0,0);
};

struct HoverPanelConfig {
    f32 openDelay  = 0.12f;
    f32 closeDelay = 0.25f;
    f32 gap        = 4.0f;
};

inline void CloseHoverPanel(HoverPanelState& st){
    st.open = false; st.pinned = false; st.leaveTime = 0.0f; st.hoverTime = 0.0f;
}

// Returns true = panel is open: draw your content, then call EndHoverPanel.
inline bool BeginHoverPanel(HoverPanelState& st, const char* name, const ImRect& anchorRect,
    bool anchorHovered, bool anchorClicked, f32 dpi, const HoverPanelConfig& cfg = HoverPanelConfig{}){

    const ImGuiIO& io = ImGui::GetIO();
    bool justOpened = false;

    // Anchor click: open+pinned -> pin -> close
    if (anchorClicked){
        if (!st.open){ st.open = true; st.pinned = true; }
        else if (!st.pinned) st.pinned = true;
        else { CloseHoverPanel(st); return false; }
        st.leaveTime = 0.0f; st.hoverTime = 0.0f;
    }

    // Hover-intent open
    if (!st.open){
        st.hoverTime = anchorHovered ? st.hoverTime + io.DeltaTime : 0.0f;
        if (st.hoverTime < cfg.openDelay) return false;
        st.open = true;
        justOpened = true;
    }

    // Close logic
    const bool busy = ImGui::IsAnyItemActive(); // e.g. dragging the slider outside the panel
    if (!st.pinned){
        if (anchorHovered || st.panelHovered || busy) st.leaveTime = 0.0f;
        else {
            st.leaveTime += io.DeltaTime;
            if (st.leaveTime >= cfg.closeDelay){ CloseHoverPanel(st); return false; }
        }
    }
    else if ((ImGui::IsMouseClicked(0) && !st.panelHovered && !anchorHovered) || ImGui::IsKeyPressed(ImGuiKey_Escape)){
        CloseHoverPanel(st); return false;
    }

    // Position: above anchor, right-aligned (File Pilot style)
    const f32 gap = cfg.gap * dpi;
    ImVec2 pos = (st.lastSize.x > 0.0f)
        ? ImVec2(anchorRect.Max.x - st.lastSize.x, anchorRect.Min.y - st.lastSize.y - gap)
        : ImVec2(anchorRect.Max.x - 240.0f * dpi, anchorRect.Min.y - 300.0f * dpi); // frame-1 estimate
    ImGui::SetNextWindowPos(pos);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, Theme::Current.palette.Surface);
    ImGui::PushStyleColor(ImGuiCol_Border,  Theme::Current.palette.Border);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f * dpi);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f * dpi, 6.0f * dpi));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2.0f * dpi, 2.0f * dpi));

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNavFocus;
    if (!st.pinned) flags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus; // pinned = floating/draggable

    if (!ImGui::Begin(name, nullptr, flags)){
        ImGui::End(); ImGui::PopStyleVar(3); ImGui::PopStyleColor(2);
        st.panelHovered = false;
        return false;
    }
    
    if (justOpened){   // Very important
        ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
    }
    return true;
}

inline void EndHoverPanel(HoverPanelState& st){
    st.panelHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem | ImGuiHoveredFlags_ChildWindows);
    st.lastSize = ImGui::GetWindowSize();

    if (st.pinned){ // little X so a pinned panel can be dismissed
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        ImDrawList* dl = window->DrawList;
        const f32 s = 20.0f;
        ImRect closeRect(ImVec2(window->Pos.x + window->Size.x - s - 4.0f, window->Pos.y + 4.0f),
                         ImVec2(window->Pos.x + window->Size.x - 4.0f,     window->Pos.y + 4.0f + s));
        ImGuiID id = window->GetID("##hoverpanel_close");
        ImGui::ItemAdd(closeRect, id);
        bool hov = false, held = false;
        if (ImGui::ButtonBehavior(closeRect, id, &hov, &held)){ CloseHoverPanel(st); }
        else if (hov) dl->AddRectFilled(closeRect.Min, closeRect.Max, Theme::Current.palette.SurfaceHover, 4.0f);
        DrawTextCenteredSingleLine(dl, closeRect.Min, closeRect.Max, ICON_REG_DISMISS, Theme::Current.palette.Text, 12.0f);
    }
    ImGui::End();
    ImGui::PopStyleVar(3); ImGui::PopStyleColor(2);
}