
#pragma once

#include "CtxMenuUI.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "BasicTypes.h"
#include "theme.h"
#include "ImGuiHelpers.h"
#include "App.h"
#include <cfloat>
#include <cstring>
#include <unordered_set>

#include <functional>


void PushMenuTheme(f32 dpi){
    ImGui::PushStyleColor(ImGuiCol_PopupBg,       Theme::Current.palette.Surface);
    ImGui::PushStyleColor(ImGuiCol_ChildBg,       Theme::Current.palette.Surface);
    ImGui::PushStyleColor(ImGuiCol_Header,        IM_COL32(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, IM_COL32(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive,  IM_COL32(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_Separator,     Theme::Current.palette.Border);
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize,   12.0f * dpi);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding,  8.0f * dpi);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f * dpi);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,  ImVec2(8.0f * dpi, 16.0f * dpi));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,    ImVec2(2.0f * dpi, 8.0f * dpi));
}
void PopMenuTheme(){ ImGui::PopStyleVar(5); ImGui::PopStyleColor(6); }

void RenderContextMenuStructure(ComPtr<IContextMenu> ctxMenu, std::vector<ContextMenuItem>& items, PCIDLIST_ABSOLUTE parentPidl,  std::vector<PCITEMID_CHILD>& childPidls, HWND hwnd,f32 dpi){
    if (items.empty())  return;
    const f32 iconSize = 16.0f * dpi;
    const f32 gutter   = iconSize + 12.0f * dpi;
    const f32 extraWidth = 100.0f * dpi;

    // spaces only reserve the icon column width (stock text is invisible anyway)
    f32 spaceW = ImGui::CalcTextSize(" ").x;
    std::string pad((int)ceilf(gutter / spaceW), ' ');
    
    int extraSpaces = (int)ceilf(extraWidth / spaceW);
    std::string extraPad(extraSpaces, ' ');


    const ImU32 textCol  = Theme::Current.palette.Text;
    const ImU32 mutedCol = Theme::Current.palette.TextDisabled;
    const ImU32 activeCol = Theme::Current.palette.SurfaceActive;
    const ImU32 hoverCol = Theme::Current.palette.SurfaceHover;

    for (auto& item : items){
        if (item.isSeparator){ ImGui::Separator(); continue; }
        if (item.text.empty()) continue;


        // PER-ITEM cache: unique label per row, built once per open/dpi-change
        if (item.labelDpi != dpi){
            item.label = pad + item.text + extraPad + "##ctx" + std::to_string(item.id);
            item.labelDpi = dpi;
        }

        bool hasSub = !item.subItems.empty();
        bool open = false, clicked = false;
        
        // 1) Stock widget, invisible ink: behavior + width yes, pixels no. (transparent Text hides the label AND the stock arrow)
        ImGui::PushStyleColor(ImGuiCol_Text,         IM_COL32(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_TextDisabled, IM_COL32(0,0,0,0));
        
        if (hasSub) open   = ImGui::BeginMenu(item.label.c_str(), item.enabled);
        else       clicked = ImGui::MenuItem(item.label.c_str(), nullptr, false, item.enabled);
        
        ImGui::PopStyleColor(2);
        
        bool hovered = ImGui::IsItemHovered();
        bool held    = ImGui::IsItemActive();
        ImVec2 rMin = ImGui::GetItemRectMin();
        ImVec2 rMax = ImGui::GetItemRectMax();
        
        ImDrawList* dl = ImGui::GetForegroundDrawList();
        
        const ImVec2 hMin(rMin.x + 2.0f*dpi, rMin.y + 1.0f);
        const ImVec2 hMax(rMax.x - 2.0f*dpi, rMax.y - 1.0f);
        if (held)                dl->AddRectFilled(hMin, hMax, activeCol, 4.0f*dpi);
        else if (hovered || open) dl->AddRectFilled(hMin, hMax, hoverCol,  4.0f*dpi);
        
        // shell icon in the gutter (drawn AFTER the highlight
        if (item.hIconTex){
            f32 iy = rMin.y + ((rMax.y - rMin.y) - iconSize) * 0.5f;
            dl->AddImage(item.hIconTex, ImVec2(rMin.x + 6.0f*dpi, iy), ImVec2(rMin.x + 6.0f*dpi + iconSize, iy + iconSize));
        }

        // our text, themed + ellipsized
        f32 textRight = hasSub ? rMax.x - 20.0f*dpi : rMax.x - 8.0f*dpi;
        DrawTextEllipsisSingleLine(dl, ImRect(ImVec2(rMin.x + gutter, rMin.y), ImVec2(textRight, rMax.y)), item.text.c_str(), item.enabled ? textCol : mutedCol);

        if (!item.shortcut.empty()){
            ImVec2 shortcutSize = ImGui::CalcTextSize(item.shortcut.c_str());
            f32 shortcutX = rMax.x - shortcutSize.x - 12.0f * dpi;
            if (hasSub) shortcutX -= 16.0f * dpi;

            ImRect shortcutRect = ImRect(ImVec2(shortcutX, rMin.y), ImVec2(rMax.x - 8.0f*dpi, rMax.y));
            DrawTextEllipsisSingleLine(dl, shortcutRect, item.shortcut.c_str(), mutedCol);
        }

        // OUR chevron, our glyph, our color
        if (hasSub){
            DrawTextCenteredSingleLine(dl, ImVec2(rMax.x - 18.0f*dpi, rMin.y), ImVec2(rMax.x - 4.0f*dpi, rMax.y), ICON_REG_CHEVRON_RIGHT, (hovered || open) ? textCol : mutedCol, 12.0f * dpi);
        } 

        if (open){ 
            RenderContextMenuStructure(ctxMenu, item.subItems, parentPidl, childPidls, hwnd, dpi); 
            ImGui::EndMenu(); 
        }
        else if (clicked){ 
            ExecuteContextMenuCommand(ctxMenu, parentPidl, childPidls, item.id, hwnd); 
        }
    }
}