#pragma once

#include "imgui.h"
#include "BasicTypes.h"
#include "global.h"
#include "theme.h"
#include "iconRegular.h"
#include "ImGuiHelpers.h"

#include "TextureManager.h"
#include "IconManager.h"

#include "SidebarEnum.h"

#include <cfloat>

inline bool RenderSectionHeader(const char* id, const char* headerGlyph, const char* label, bool isOpen, bool isDisabled, f32 dpi, f32 h, ImVec2 xFramePadding, f32 frameRounding){
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    ImDrawList* dl = window->DrawList;

    const f32 width = ImGui::GetContentRegionAvail().x;
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const ImRect rect(pos, ImVec2(pos.x + width, pos.y + h));

    ImGui::PushID(id);
    const bool pressed = ImGui::InvisibleButton("header", ImVec2(width, h));
    const bool hovered = ImGui::IsItemHovered();
    ImGui::PopID();

    const ImU32 mutedCol = Theme::Current.palette.TextMuted;
    const ImU32 disabledCol = Theme::Current.palette.TextDisabled;
    const ImU32 textCol  = isDisabled ? disabledCol : mutedCol;

    if (!isDisabled && hovered){
        ImRect bgRect(ImVec2(rect.Min.x + xFramePadding.x, rect.Min.y + 1.0f),
                       ImVec2(rect.Max.x - xFramePadding.y, rect.Max.y - 1.0f));
        DrawSelectableBg(dl, bgRect, true, false, frameRounding);
    }

    ImFont* font = ImGui::GetFont();

    // Header Glyph (muted)
    const f32 glyphFontSize = 16.0f * dpi;
    const ImVec2 glyphSize = font->CalcTextSizeA(glyphFontSize, FLT_MAX, 0.0f, headerGlyph);

    const f32 glyphX = rect.Min.x + 10.0f * dpi;

    dl->AddText(
        font,
        glyphFontSize,
        ImVec2(glyphX, rect.Min.y + (h - glyphSize.y) * 0.5f),
        textCol,
        headerGlyph
    );

    // Label 
    const f32 labelFontSize = ImGui::GetFontSize();
    const ImVec2 labelSize = font->CalcTextSizeA(labelFontSize, FLT_MAX, 0.0f, label);
    const f32 labelX = glyphX + glyphSize.x + 8.0f * dpi;

    dl->AddText(
        font, labelFontSize,
        ImVec2(labelX, rect.Min.y + (h - labelSize.y) * 0.5f),
        textCol,
        label
    );

    // Collapse chevron, right aligned
    const char* chevron =isOpen ? ICON_REG_CHEVRON_DOWN : ICON_REG_CHEVRON_RIGHT;

    const f32 chevFontSize = 12.0f * dpi;
    const ImVec2 chevSize = font->CalcTextSizeA(chevFontSize, FLT_MAX, 0.0f, chevron);

    dl->AddText(
        font, chevFontSize, 
        ImVec2(
            rect.Max.x - 10.0f * dpi - chevSize.x,
            rect.Min.y + (h - chevSize.y) * 0.5f
        ),
        textCol, 
        chevron
    );
    if (isDisabled){
        return false;
    }
    return pressed;
}

inline bool RenderItemRow(const char* id, ImTextureID icon, const char* label, bool isSelected, f32 dpi, f32 h, ImVec2 xFramePadding, f32 frameRounding, f32 indent){
    ImGuiWindow* window = ImGui::GetCurrentWindow();

    if (window->SkipItems) return false;

    ImDrawList* dl = window->DrawList;

    const f32 width = ImGui::GetContentRegionAvail().x;
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const ImRect rect(pos, ImVec2(pos.x + width, pos.y + h));

    ImGui::PushID(id);
    const bool pressed = ImGui::InvisibleButton("row", ImVec2(width, h));
    const bool hovered = ImGui::IsItemHovered();
    ImGui::PopID();

    const ImU32 textCol   = Theme::Current.palette.Text;
    const ImU32 mutedCol  = Theme::Current.palette.TextMuted;

    if (hovered || isSelected){
        ImRect bgRect(ImVec2(rect.Min.x + xFramePadding.x, rect.Min.y + 1.0f),
                       ImVec2(rect.Max.x - xFramePadding.y, rect.Max.y - 1.0f));
        DrawSelectableBg(dl, bgRect, hovered, isSelected, frameRounding);
    }

    ImFont* font = ImGui::GetFont();

    const f32 iconSize = 16.0f * dpi;
    const f32 iconX = rect.Min.x + indent;
    const f32 iconY = rect.Min.y + (h - iconSize) * 0.5f;

    if (icon){
        dl->AddImage(
            icon, 
            ImVec2(iconX, iconY),
            ImVec2(iconX + iconSize, iconY + iconSize)
        );
    }
    else {
        // Placeholder while the async icon is loading
        const char* fallback = ICON_REG_FOLDER;
        const ImVec2 fallbackSize = font->CalcTextSizeA(iconSize, FLT_MAX, 0.0f, fallback);

        dl->AddText(
            font, iconSize,
            ImVec2(iconX, rect.Min.y + (h - fallbackSize.y) * 0.5f),
            mutedCol, fallback
        );
    }

    // Label with ellipsis
    ImRect textRect(ImVec2(iconX + iconSize + 8.0f * dpi, rect.Min.y),
                     ImVec2(rect.Max.x - 8.0f * dpi, rect.Max.y));
    DrawTextEllipsisSingleLine(dl, textRect, label, textCol);

    return pressed;
}


inline void RenderSidebar(f32 dpi, App& app){
    SidebarManager& sidebarManager = app.sidebar;
    const TextureManager& textures = app.textures;
    const IconManager& icons = app.icons;

    const f32 sidebarW = g_sidebarWidthAnim.Get();
    if (sidebarW <= 1.0f) return;
    
    const f32 titleH = TitlebarHeight * dpi;
    const f32 searchHeight = titleH;
    
    const ImU32 bgCol = Theme::Current.palette.Background;
    const ImU32 surfaceCol = Theme::Current.palette.Surface;
    const ImU32 borderCol = Theme::Current.palette.Border;
    const ImU32 textMutedCol = Theme::Current.palette.TextMuted;
    
    ImVec2 mousePos = ImGui::GetMousePos();
    
    // td make a custom sidebar scroll
    ImGui::PushStyleColor(ImGuiCol_ChildBg, surfaceCol);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    
    ImVec2 searchMin, searchMax;
    
    
    ImVec2 sidebarStartPos = ImGui::GetCursorScreenPos();
    ImGui::BeginGroup();

    if (ImGui::BeginChild("SidebarSearch", ImVec2(sidebarW, searchHeight), ImGuiChildFlags_None,ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)){
        ImDrawList* dl = ImGui::GetWindowDrawList();
        searchMin = ImGui::GetWindowPos();
        f32 searchWidth = ImGui::GetWindowWidth();
        searchMax = ImVec2(searchMin.x + searchWidth, searchMin.y + searchHeight);
        ImRect searchRect (searchMin, searchMax);
        
        // Search box background.
        ImU32 searchBgCol = (searchRect.Contains(mousePos)) ? bgCol : surfaceCol;
        dl->AddRectFilled(searchMin, searchMax, searchBgCol);
        
        // Search box text.
        ImFont* font = ImGui::GetFont();
        const f32 fontSize = ImGui::GetFontSize();
        
        const char* iconText = ICON_REG_SEARCH;
        const char* placeholderText = "Filter Quick Access...";
        
        const ImVec2 iconSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, iconText);
        const ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, placeholderText);
        const f32 distanceBetweenSearchIconAndText = 6.0f * dpi;
        
        const f32 totalPlaceholderWidth = iconSize.x + distanceBetweenSearchIconAndText + textSize.x;
        
        f32 iconAndTextX = searchRect.Min.x;
        if (totalPlaceholderWidth < searchRect.GetWidth()) iconAndTextX = (searchRect.GetWidth() - totalPlaceholderWidth) * 0.5f;
        
        iconAndTextX += DrawTextAtX(dl, iconAndTextX, searchRect, iconText, textMutedCol).x + distanceBetweenSearchIconAndText;
        DrawTextAtX(dl, iconAndTextX, searchRect, placeholderText, textMutedCol);
        
     
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
    
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(ImVec2(searchMin.x, searchMax.y), ImVec2(searchMax.x, searchMax.y + titlebarBottomBorderThickness * dpi), borderCol);
    ImGui::Dummy(ImVec2(0.0f, titlebarBottomBorderThickness * dpi));    
    
    // ----------------------------
    // Sections
    // ----------------------------

    ImGui::PushStyleColor(ImGuiCol_ChildBg, surfaceCol);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    if (ImGui::BeginChild("##SidebarContent", ImVec2(sidebarW, 0.0f), false)) {

        // how do i handle make the ID unique, in case the user adds two groups with same name
        // make empty sidebarcategories disabled text, and disabled button
        f32 indent = 36.0f * dpi;
        for (SidebarCategory& sec : sidebarManager.categories){
            if (RenderSectionHeader(sec.name, sec.icon, sec.name, sec.isOpen, sec.contents.empty(), dpi, 32.0f * dpi, {4.0f * dpi, 4.0f * dpi}, 4.0f * dpi)){
                sec.isOpen = !sec.isOpen;
            }

            if (sec.isOpen){
                for (size_t i = 0; i < sec.contents.size(); i++){

                    DirItem& item = sec.contents[i];
                    ImTextureID iconTex = textures.GetTexture({icons.GetIconIndex(item.pidl.get(), item.hash), SHIL_SMALL});

                    std::string rowID = item.name + std::to_string(i);

                    bool isSelected = app.window.GetActiveTab().dir.parent.hash == item.hash;
                    if (RenderItemRow(rowID.c_str(), iconTex, item.name.c_str(), isSelected, dpi, 30.0f * dpi, {4.0f * dpi, 4.0f * dpi}, 4.0f * dpi, indent)){
                        app.QueueCommand(Cmd_GoTo {app.window.activeTabIndex, item.pidl.Clone()});
                    }
                }
            }
            // Gap between sections
            ImGui::Dummy(ImVec2(0.0f, 6.0f * dpi));
        }   
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);

    ImGui::EndGroup();

    f32 screenH = ImGui::GetWindowHeight();
    f32 screenW = ImGui::GetWindowWidth();

    const f32 handleW = 4.0f * dpi;
    const f32 handleH = screenH - (TitlebarHeight + titlebarBottomBorderThickness) * dpi;
    const ImVec2 topLeft(ImGui::GetWindowPos().x + g_sidebarWidthAnim.Get(), ImGui::GetWindowPos().y + titleH + titlebarBottomBorderThickness * dpi);


    f32 dragDelta = RenderResizeHorizontalHandle(
        "SidebarResize", topLeft, handleW, handleH,
        Theme::Current.palette.SurfaceActive,
        Theme::Current.palette.SurfaceHover
    );

    if (dragDelta != 0.0f){
        // user is dragging 
        const f32 closedSidebarHeaderWidth = TitlebarHeight * dpi * 2.0f;

        f32 newWidth = g_sidebarWidthAnim.Get() + dragDelta;
        newWidth = Clampf(newWidth, SidebarMinRatio * screenW, SidebarMaxRatio * screenW);

        
        g_sidebarWidthAnim.SetValue( newWidth);
        g_sidebarHeaderWidthAnim.SetValue(ImMax(newWidth, closedSidebarHeaderWidth));

        g_sidebarRatio = newWidth / screenW;
    }


    f32 sidebarRightBorderThickness = titlebarBottomBorderThickness * dpi;
    dl->AddRectFilled(topLeft, ImVec2(topLeft.x + titlebarBottomBorderThickness * dpi, topLeft.y + handleH), borderCol);
    // move forward to set space for the white border and resizeable width, unless they will be covered, important 
    ImGui::SetCursorScreenPos({sidebarStartPos.x + sidebarW + sidebarRightBorderThickness + handleW, sidebarStartPos.y});
    
}