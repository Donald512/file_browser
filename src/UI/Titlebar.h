#pragma once

#include "imgui.h"
#include "imgui_internal.h"

#include "BasicTypes.h"
#include "global.h"
#include "theme.h"
#include "iconRegular.h"
#include "Tab.h"
#include "TextureManager.h"
#include "App.h"
#include "ImGuiHelpers.h"

#include <vector>
#include <cstddef>
#include <cfloat>

inline void RenderTitlebar(f32 dpi, App& app, bool isMaximized = false){
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (!window || window->SkipItems) return;

    const std::vector<Tab>& tabs = app.window.tabs;
    const size_t activeTabIndex = app.window.activeTabIndex;
    const TextureManager& textures = app.textures;
    const IconManager& icons = app.icons;

    g_HitTestRegistry.Clear();
    
    ImDrawList* dl = window->DrawList;

    const ImVec2 windowPos = window->Pos;
    const ImVec2 contentSize = ImGui::GetWindowSize();

    const f32 titleH = TitlebarHeight * dpi;
    const f32 captionW = CaptionButtonsWidth * dpi;

    const f32 tabChevronWidth = titleH;

    const ImRect titleRect(
        windowPos,
        ImVec2(windowPos.x + contentSize.x, windowPos.y + titleH)
    );

    const ImU32 surfaceCol = Theme::Current.palette.Surface;
    const ImU32 backgroundCol = Theme::Current.palette.Background;
    const ImU32 surfaceHoverCol = Theme::Current.palette.SurfaceHover;
    const ImU32 surfaceActiveCol = Theme::Current.palette.SurfaceActive;
    const ImU32 textCol = Theme::Current.palette.Text;
    const ImU32 closeHoverCol = IM_COL32(232, 17, 35, 255);
    const ImU32 borderCol = Theme::Current.palette.Border;

    // Titlebar background.
    dl->AddRectFilled(titleRect.Min, titleRect.Max, surfaceCol);
    // draw border below titlebar
    f32 bottomBorderThickness = titlebarBottomBorderThickness * dpi;
    dl->AddRectFilled(ImVec2(titleRect.Min.x, titleRect.Max.y), ImVec2(titleRect.Max.x, titleRect.Max.y + bottomBorderThickness), borderCol);

    //------------------------------------------------------------------------
    // Caption buttons.
    //------------------------------------------------------------------------

    f32 captionX = titleRect.Max.x - captionW;
    
    const f32 minW = CaptionMinWidth * dpi;
    const f32 maxW = CaptionMaxWidth * dpi;
    const f32 closeW = CaptionCloseWidth * dpi;
    
    
    // Actual window actions still be handled by WndProc.
    ImRect minRect(ImVec2(captionX, titleRect.Min.y), ImVec2(captionX + minW, titleRect.Max.y));
    RenderIconButton(minRect, "CaptionMinimize", ICON_REG_SUBTRACT, 0.0f, surfaceHoverCol, surfaceActiveCol, textCol);
    captionX += minW;
    
    ImRect maxRect(ImVec2(captionX, titleRect.Min.y), ImVec2(captionX + maxW, titleRect.Max.y));
    RenderIconButton(maxRect, "CaptionMaximize", 
    isMaximized ? ICON_REG_SQUARE_MULTIPLE : ICON_REG_MAXIMIZE, 
    0.0f, surfaceHoverCol, surfaceActiveCol, textCol);
    captionX += maxW;

    ImRect closeRect(ImVec2(captionX, titleRect.Min.y), ImVec2(captionX + closeW, titleRect.Max.y));
    RenderIconButton(closeRect, "CaptionClose", ICON_REG_DISMISS, 0.0f, closeHoverCol, closeHoverCol, textCol);
    captionX += closeW;
    

    //------------------------------------------------------------------------
    // Sidebar header.
    //------------------------------------------------------------------------

    // titleH is width of buttons too, they are square
    // f32 sidebarHeaderW = g_sidebarOpen ? CalcSidebarWidth(windowWidth, dpi) : titleH * 2.0f;
    f32 sidebarHeaderW = ImMax(g_sidebarHeaderWidthAnim.Get(), titleH * 2.0f);

    sidebarHeaderW = ImMax(sidebarHeaderW, titleH * 2.0f);

    // so it doesnt draw on top of captionW, they have priority
    if (contentSize.x < sidebarHeaderW + captionW) return;


    const ImRect appMenuRect(
        titleRect.Min,
        ImVec2(titleRect.Min.x + titleH, titleRect.Min.y + titleH)
    );

    g_HitTestRegistry.RegisterElement(appMenuRect);
    if (RenderIconButton(appMenuRect, "AppMenuButton", ICON_REG_NAVIGATION, 4.0f * dpi, surfaceHoverCol, surfaceHoverCol, textCol)){
        // TODO: open app menu popup.
    }

    const ImRect sidebarToggleRect(
        ImVec2(titleRect.Min.x + sidebarHeaderW - titleH, titleRect.Min.y),
        ImVec2(titleRect.Min.x + sidebarHeaderW, titleRect.Min.y + titleH)
    );

    g_HitTestRegistry.RegisterElement(sidebarToggleRect);
    if (RenderIconButton(sidebarToggleRect, "SidebarToggleButton",
            g_sidebarOpen ? ICON_REG_CHEVRON_LEFT : ICON_REG_CHEVRON_RIGHT,
            4.0f * dpi, surfaceHoverCol, surfaceHoverCol, textCol)){
        g_sidebarOpen = !g_sidebarOpen;
    }

    
    //------------------------------------------------------------------------
    // Tabs.
    //------------------------------------------------------------------------
    
    const f32 newTabSize = titleH;
    
    const f32 tabsLeft = titleRect.Min.x + sidebarHeaderW + 6.0f * dpi;

    const f32 chevronLeft = titleRect.Max.x - captionW - tabChevronWidth;

    const f32 newTabXAnchored = chevronLeft - (NewTabToTabChevronGap * dpi) - newTabSize;

    const f32 tabsRight = newTabXAnchored - (TabsToNewTabGap * dpi);

    const f32 tabsAvailableWidth = tabsRight - tabsLeft;

    if (tabsAvailableWidth <= 0.0f || tabs.empty()) return;
        
    const f32 minTabW = TabMinWidth * dpi;
    const f32 maxTabW = TabMaxWidth * dpi;
    const f32 gapBetweenTabs = 2.0f * dpi;

    f32 tabW = ImClamp( tabsAvailableWidth / (f32)tabs.size(), minTabW, maxTabW);
    
    f32 totalTabWidth = 0;
    if (!tabs.empty()) totalTabWidth = tabW * tabs.size() + gapBetweenTabs * (tabs.size() - 1);

    //--------------
    // Make sure the active tab index is visible on top if state is changed
    //--------------
    const f32 maxTabsScroll = ImMax(0.0f, totalTabWidth - tabsAvailableWidth);
    
    if (activeTabIndex < tabs.size()){
        
        static size_t prevActiveTabIndex = activeTabIndex;
        static size_t  prevTabCount = tabs.size();
        
        bool shouldScrollToActiveTab = false;
        if (activeTabIndex != prevActiveTabIndex) shouldScrollToActiveTab = true;
        else if (tabs.size() > prevTabCount) shouldScrollToActiveTab = true; // newtab opened
        
        if (shouldScrollToActiveTab){
            ScrollTabIntoView(activeTabIndex, tabW, gapBetweenTabs, tabsAvailableWidth, g_tabsScroll);
        }
        prevTabCount = tabs.size();
        prevActiveTabIndex = activeTabIndex;

        g_tabsScroll = Clampf(g_tabsScroll, 0.0f, maxTabsScroll);
    }
    

    
    dl->PushClipRect(
        ImVec2(tabsLeft, titleRect.Min.y),
        ImVec2(tabsRight, titleRect.Max.y + bottomBorderThickness /*required to erase the border below active tab*/),
        true
    );
    
    const ImVec2 mousePos = ImGui::GetMousePos();
    
    const f32 radius = TabCornerRadius * dpi;
    const f32 iconSize = 16.0f * dpi;
    const f32 closeSize = 16.0f * dpi;
    const f32 padding = 6.0f * dpi;
    const f32 gap = 4.0f * dpi;
    
    
    f32 currentX = tabsLeft - g_tabsScroll;
    for (size_t i = 0; i < tabs.size(); ++i){
        const f32 remainingWidth = tabsRight - currentX;
        if (remainingWidth < 2.0f) break;
        
        // draw the tab if its actually visible inside the clip rect
        if ((currentX + tabW >= tabsLeft && currentX <=  tabsRight)){

            const f32 currentTabW = ImMin(tabW, remainingWidth);

            const ImRect tabRect(
                ImVec2(currentX, titleRect.Min.y),
                ImVec2(currentX + currentTabW, titleRect.Max.y)
            );

            g_HitTestRegistry.RegisterElement(tabRect);
            
            const Tab& tab = tabs[i];
            
            const bool isActiveTab = (i == activeTabIndex);
            const bool isHovered = tabRect.Contains(mousePos);
            const bool showCloseButton = isActiveTab || isHovered;
            
            f32 contentRight = tabRect.Max.x - padding;
            ImRect tabCloseRect(ImVec2(0, 0), ImVec2(0, 0));
            bool closeHovered = false;
            f32 closeBtnSize = ImMax(tabRect.Max.y - tabRect.Min.y - (8.0f * dpi), closeSize);
            
            if (showCloseButton){
                tabCloseRect = ImRect(
                    ImVec2(
                        contentRight - closeBtnSize,
                        tabRect.Min.y + (titleH - closeBtnSize) * 0.5f
                    ),
                    ImVec2(
                        contentRight,
                        tabRect.Min.y + (titleH + closeBtnSize) * 0.5f
                    )
                );
                closeHovered = tabCloseRect.Contains(mousePos);
                contentRight = tabCloseRect.Min.x - gap;
            }
            
            const bool isTabHoveredOnly = isHovered && !closeHovered;

            if (isActiveTab){
                dl->AddRectFilled( tabRect.Min, tabRect.Max, backgroundCol, radius, ImDrawFlags_RoundCornersTopLeft | ImDrawFlags_RoundCornersTopRight);
                // Active erases its below border
                dl->AddRectFilled(ImVec2(tabRect.Min.x, tabRect.Max.y), ImVec2(tabRect.Max.x, tabRect.Max.y + bottomBorderThickness), backgroundCol);

            }

            else if (isTabHoveredOnly){
                dl->AddRectFilled(tabRect.Min, tabRect.Max, surfaceHoverCol, radius);
            }
            
            if (showCloseButton && closeHovered) {
                // Render close button highlight independently
                dl->AddRectFilled(tabCloseRect.Min, tabCloseRect.Max, surfaceHoverCol, 4.0f * dpi);
            }

            // Tab icon.
            const ImVec2 iconMin(
                tabRect.Min.x + padding,
                tabRect.Min.y + (titleH - iconSize) * 0.5f
            );

            ImTextureID iconTex = textures.GetTexture({icons.GetIconIndex(tab.dir.parent.pidl.get(), tab.dir.parent.hash), SHIL_SMALL});

            if (iconTex){
                dl->AddImage(iconTex, iconMin, ImVec2(iconMin.x + iconSize, iconMin.y + iconSize));
            }
            
            // Tab title.
            ImFont* font = ImGui::GetFont();
            const f32 fontSize = ImGui::GetFontSize();
            const f32 textLeft = iconMin.x + iconSize + gap;
            const ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, tab.dir.parent.name.c_str());
            
            if (contentRight > textLeft){
                DrawTextEllipsisSingleLine(dl,
                ImRect(ImVec2(textLeft, tabRect.Min.y), ImVec2(contentRight, tabRect.Max.y)), tab.dir.parent.name.c_str(), textCol);
            }
            
            // Close button glyph.
            if (showCloseButton){
                DrawTextCenteredSingleLine( dl, tabCloseRect.Min, tabCloseRect.Max, ICON_REG_DISMISS, textCol, 12.0f);
            }
            
            // Input.
            // Required so you dont queue the app for close, and queue for switching
            // The tab after it becomes active tab because it assumes position of the activeTabIndex
            bool clickConsumed = false;
            
            if (closeHovered && ImGui::IsMouseClicked(0)){
                app.QueueCommand(Cmd_CloseTab{activeTabIndex});
                clickConsumed = true;
            }

            if (!clickConsumed && isHovered && ImGui::IsMouseClicked(0)){
                app.QueueCommand(Cmd_SwitchTab{i});
            }
        }
        currentX += tabW + gapBetweenTabs;
    }
    dl->PopClipRect();

    // -----------
    // Scrollbar
    // -----------
    f32 scrollbarHeight = 4.0f * dpi;
    ImRect scrollTrackRect(
        ImVec2(tabsLeft + (6.0f * dpi), titleRect.Max.y - scrollbarHeight), 
        ImVec2(tabsRight - (6.0f * dpi), titleRect.Max.y)
    );
    
    ImRect scrollHoverTrackRect(
        ImVec2(tabsLeft, titleRect.Min.y),
        ImVec2(tabsRight, titleRect.Max.y + scrollbarHeight)
    );


    RenderHorizontalScrollbar("TabScrollbar", scrollTrackRect, scrollHoverTrackRect, totalTabWidth, tabsAvailableWidth, g_tabsScroll, dpi, true);

    //------------------------------------------------------------------------
    // New tab button.
    //------------------------------------------------------------------------


    const f32 newTabX1 = tabsLeft + totalTabWidth + (TabsToNewTabGap * dpi);
    const f32 newTabX2 = newTabXAnchored;
    const f32 newTabX = ImMin(newTabX1, newTabX2);

    // newTabRect start
    const ImRect newTabRect(
        ImVec2(newTabX, titleRect.Min.y + (titleH - newTabSize) * 0.5f),
        ImVec2(newTabX + newTabSize, titleRect.Min.y + (titleH + newTabSize) * 0.5f)
    );

    g_HitTestRegistry.RegisterElement(newTabRect);
    if (newTabRect.Min.x <= titleRect.Min.x + sidebarHeaderW + titleH) return;

    if (RenderIconButton(newTabRect, "NewTabButton", ICON_REG_ADD, 4.0f * dpi, surfaceHoverCol, surfaceHoverCol, textCol)){
        app.QueueCommand(Cmd_NewTab{});
    }
    
    const ImRect tabChevronRect(
        ImVec2(chevronLeft, titleRect.Min.y),
        ImVec2(chevronLeft + tabChevronWidth, titleRect.Max.y)
    );
    
    
    g_HitTestRegistry.RegisterElement(tabChevronRect);
    if (RenderIconButton(tabChevronRect, "TabChevronDropdownButton", ICON_REG_CHEVRON_DOWN, titleH/2, surfaceHoverCol, surfaceHoverCol, textCol)){
    }

    // Reserve space for the custom titlebar.
    ImGui::Dummy(ImVec2(0.0f, titleH + bottomBorderThickness));

}