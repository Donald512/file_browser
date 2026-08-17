#pragma once

#include "imgui.h"
#include "imgui_internal.h"

#include "BasicTypes.h"
#include "global.h"
#include "theme.h"
#include "Sidebar.h"
#include "Titlebar.h"
#include "CmndBar.h"
#include "App.h"
#include "ImGuiHelpers.h"
#include "FileView.h"

inline void InitializeUI(f32 screenW, f32 screenH){
    (void)screenW;
    (void)screenH;
}

inline void GameLoop( f32 screenW, f32 screenH, f32 mouseX, f32 mouseY, bool isMouseDown, f32 deltaTime, f32 dpi, App& app, bool isMaximized = false){
    (void)mouseX;
    (void)mouseY;
    (void)isMouseDown;

    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(screenW, screenH));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

    ImGui::PushStyleColor(ImGuiCol_WindowBg,Theme::Current.palette.Background);

    ImGui::PushStyleColor(ImGuiCol_ChildBg,Theme::Current.palette.Background);

    ImGui::Begin(
        "AppWorkspace",
        nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoSavedSettings );

    const f32 windowWidth = ImGui::GetWindowWidth();
    const f32 closedSidebarHeaderWidth = TitlebarHeight * dpi * 2.0f;

    // calculate the preferred open width (clamped to absule min/max)
    const f32 preferredOpenWidth = Clampf(windowWidth * g_sidebarRatio, SidebarMinRatio * windowWidth, SidebarMaxRatio * windowWidth);
    
    const f32 widthTarget = g_sidebarOpen ? preferredOpenWidth : 0.0f;
    const f32 headerTarget = g_sidebarOpen ? ImMax(preferredOpenWidth, closedSidebarHeaderWidth) : closedSidebarHeaderWidth;
    
    static bool uiInitialized = false;

    g_sidebarWidthAnim.SetTarget(widthTarget);
    g_sidebarHeaderWidthAnim.SetTarget(headerTarget);

    g_sidebarWidthAnim.Update(deltaTime);
    g_sidebarHeaderWidthAnim.Update(deltaTime);

    RenderTitlebar(dpi, app, isMaximized);

    const f32 sidebarW = g_sidebarWidthAnim.Get();
    const f32 topH = ( TitlebarHeight + titlebarBottomBorderThickness) * dpi;

    if (sidebarW > 1.0f){
        RenderSidebar(dpi, app);
    }

    
    if (ImGui::BeginChild("Content", ImVec2(screenW - sidebarW, screenH - topH), false)){
        RenderCommandbar(dpi, app);    

        if (ImGui::BeginChild("FileViewArea", ImVec2(0, 0), false)) {
            RenderFileGrid(dpi, app);
        }
        ImGui::EndChild();
    }
    ImGui::EndChild();



    ImGui::End();

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
}