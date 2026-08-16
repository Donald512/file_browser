#pragma once
#include "BasicTypes.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "App.h"
#include "ImGuiHelpers.h"
#include "global.h"
#include "theme.h"
#include "iconRegular.h"
#include <cfloat>
#include <cmath>
#include <algorithm>
#include <vector>
#include <string>

struct GridViewParams {
    f32 width;
    f32 height;
};

inline GridViewParams GetGridParamsForMode(ViewMode mode){
    switch (mode){
        case ViewMode::ExtraLarge: return {271.0f, 260.0f};
        case ViewMode::Large:      return {105.0f, 100.0f};
        case ViewMode::Medium:     return {74.0f,  52.0f};
        case ViewMode::Small:      return {308.0f, 30.0f};
        case ViewMode::List:       return {308.0f, 30.0f};
        case ViewMode::Details:    return {308.0f, 30.0f};
        case ViewMode::Tiles:      return {250.0f, 52.0f};
        default:                   return GetGridParamsForMode(ViewMode::Large);
    }
}

inline int ShilSizeForMode(ViewMode mode){
    switch (mode){
        case ViewMode::ExtraLarge: return SHIL_JUMBO;
        case ViewMode::Large:      return SHIL_JUMBO;
        case ViewMode::Medium:     return SHIL_EXTRALARGE;
        case ViewMode::Small:      return SHIL_SMALL;
        case ViewMode::List:       return SHIL_SMALL;
        case ViewMode::Details:    return SHIL_SMALL;
        case ViewMode::Tiles:      return SHIL_EXTRALARGE;
        default:                   return SHIL_EXTRALARGE;
    }
}

struct ItemInteraction {
    bool hovered;
    bool clicked;
};

// FIX 1: Double click timing mismatch. 
// ButtonBehavior returns true on RELEASE. IsMouseDoubleClicked triggers on PRESS.
inline ItemInteraction HandleItemInteraction(App& app, const DirParent& parent, const DirChild& child, ImGuiID id, const ImRect& rect){
    ImGui::ItemAdd(rect, id);
    bool hovered, held;
    bool clicked = ImGui::ButtonBehavior(rect, id, &hovered, &held);
    
    // Check double click via hovered state, NOT the clicked state
    bool doubleClicked = hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
    
    if (doubleClicked) {
        if (child.attributes & SFGAO_FOLDER){
            PCIDLIST_ABSOLUTE newPidl = GetFullPidl(parent.pidl.get(), child.pidl.get());
            // ! showuld i change it to accept a PCIDLIST_ABSOLUTE, instead of WShell::Pidl
            app.QueueCommand({CmdType::GoTo, WShell::Pidl(newPidl), 0, app.window.activeTabIndex, L""});
        }
        else
            app.QueueCommand({CmdType::OpenFile, child.pidl.Clone(), {}, {}, L""});
    } else if (clicked) {
        app.window.GetActiveTab().selectItem(child.Hash(parent.pidl.get()), SelectMode::OneItem);
    }
    return {hovered, clicked || doubleClicked};
}

// FIX 2: Synchronous Icon Loading causes UI Freezing.
// GetIconIndex calls Windows Shell API on the main thread. 
// You MUST use your async request system here instead of synchronous fetching.
inline void DrawItemIcon(ImDrawList* dl, App& app, const DirParent& parent, const DirChild& item, ImVec2 pos, f32 iconSize, int shilSize){
    // if (!app.icons.IsCached(item.Hash(), shilSize)) { app.icons.RequestAsync(...); }
    
    // ImTextureID iconTex = 0; // Disabled synchronous call to prevent freezing
    ImTextureID iconTex = app.textures.GetTexture({app.icons.GetIconIndex(parent.shellFolder.Get(), item.pidl.get(), item.Hash(parent.pidl)), shilSize});
    
    if (iconTex){
        dl->AddImage(iconTex, pos, ImVec2(pos.x + iconSize, pos.y + iconSize));
    } else {
        const bool isFolder = (item.attributes & SFGAO_FOLDER) != 0;
        const char* fallback = isFolder ? ICON_REG_FOLDER : ICON_REG_DOCUMENT;
        DrawTextCenteredSingleLine(dl, pos, ImVec2(pos.x + iconSize, pos.y + iconSize), fallback, ToImU32(Theme::Current.palette.TextMuted), iconSize);
    }
}

inline const char* KindLabel(const DirChild& item){
    return (item.attributes & SFGAO_FOLDER) ? "File folder" : "File";
}

// FIX 3: Removed ImGuiTable from Grid views. 
// ImGuiTable and ImGuiListClipper fight over Y-coordinates, causing overlaps and crashes.
inline void RenderGridView(f32 dpi, App& app, ViewMode mode){
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (!window || window->SkipItems) return;
    ImDrawList* dl = window->DrawList;
    auto& activeTab = app.window.GetActiveTab();

    GridViewParams p = GetGridParamsForMode(mode);
    const int shilSize = ShilSizeForMode(mode);
    const f32 imageSize = p.height * dpi;
    const f32 xGap = 8.0f * dpi;
    const f32 yTextPadding = 4.0f * dpi;
    const f32 lineHeight = ImGui::GetFontSize();
    const int maxLines = (mode == ViewMode::Medium) ? 1 : 3;
    const f32 itemWidth = p.width * dpi;
    const f32 cellH = imageSize + yTextPadding + (maxLines * lineHeight) + yTextPadding;

    const Directory& directory = activeTab.directory;
    const std::vector<DirChild>& dirChildren = directory.children;

    // Cells are itemWidth wide but stride by itemWidth + xGap, so there's a
    // visible gap between them.
    ForEachGridCell(dirChildren.size(), itemWidth + xGap, cellH, [&](size_t i, ImVec2 cellPos){
        const DirChild& child = dirChildren[i];
        bool isSelected = activeTab.isSelected(child.Hash(directory.parent.pidl));
        ImRect fullRect(cellPos, ImVec2(cellPos.x + itemWidth, cellPos.y + cellH));

        ImGuiID id = window->GetID((void*)(intptr_t)i);
        ItemInteraction ia = HandleItemInteraction(app, directory.parent, child, id, fullRect);
        DrawSelectableBg(dl, fullRect, ia.hovered, isSelected, 4.0f * dpi);

        f32 iconX = cellPos.x + (itemWidth - imageSize) * 0.5f;
        DrawItemIcon(dl, app, directory.parent, child, ImVec2(iconX, cellPos.y), imageSize, shilSize);

        f32 textY = cellPos.y + imageSize + yTextPadding;
        RenderTextWrappedCenteredEllipsis(
            dl,
            ImVec2(cellPos.x + 4.0f * dpi, textY),
            ImVec2(itemWidth - 8.0f * dpi, maxLines * lineHeight),
            child.name.c_str(),
            nullptr,
            maxLines
        );
    });
}

inline void RenderSmallView(f32 dpi, App& app){
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (!window || window->SkipItems) return;
    ImDrawList* dl = window->DrawList;
    auto& activeTab = app.window.GetActiveTab();

    GridViewParams p = GetGridParamsForMode(ViewMode::Small);
    const f32 cellW = p.width * dpi;
    const f32 cellH = p.height * dpi;
    const f32 iconSize = 16.0f * dpi;
    const f32 iconPad = 6.0f * dpi;
    const f32 textGap = 8.0f * dpi;
    const ImU32 textCol = ToImU32(Theme::Current.palette.Text);

    const Directory& directory = activeTab.directory;
    const std::vector<DirChild>& dirChildren = directory.children;

    ForEachGridCell(dirChildren.size(), cellW, cellH, [&](size_t i, ImVec2 cellPos){
        const DirChild& child = dirChildren[i];
        bool isSelected = activeTab.isSelected(child.Hash(directory.parent.pidl));
        ImRect fullRect(cellPos, ImVec2(cellPos.x + cellW, cellPos.y + cellH));

        ImGuiID id = window->GetID((void*)(intptr_t)i);
        ItemInteraction ia = HandleItemInteraction(app, directory.parent, child, id, fullRect);
        DrawSelectableBg(dl, fullRect, ia.hovered, isSelected, 4.0f * dpi);

        f32 iconX = cellPos.x + iconPad;
        f32 iconY = cellPos.y + (cellH - iconSize) * 0.5f;
        DrawItemIcon(dl, app, directory.parent, child, ImVec2(iconX, iconY), iconSize, SHIL_SMALL);

        ImRect textRect(ImVec2(iconX + iconSize + textGap, cellPos.y),
                         ImVec2(cellPos.x + cellW - 8.0f * dpi, cellPos.y + cellH));
        DrawTextEllipsisSingleLine(dl, textRect, child.name.c_str(), textCol);
    });
}

inline void RenderListView(f32 dpi, App& app){
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (!window) return;
    GridViewParams p = GetGridParamsForMode(ViewMode::List);
    ImGuiChildFlags childFlags = ImGuiChildFlags_NavFlattened;
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_HorizontalScrollbar;
    if (!ImGui::BeginChild("FileViewList", ImVec2(0, 0), childFlags, windowFlags)){
        ImGui::EndChild();
        return;
    }
    if (ImGui::IsWindowHovered() && ImGui::GetIO().MouseWheel != 0.0f){
        f32 scrollAmount = ImGui::GetIO().MouseWheel * (60.0f * dpi);
        ImGui::SetScrollX(ImGui::GetScrollX() - scrollAmount);
    }
    ImDrawList* dl = ImGui::GetWindowDrawList();
    auto& activeTab = app.window.GetActiveTab();

    const Directory& directory = activeTab.directory;
    const std::vector<DirChild>& dirChildren = directory.children;

    const f32 xGap = 8.0f * dpi;
    const f32 cellHeight = p.height * dpi;
    const f32 iconSize = 16.0f * dpi;
    const ImU32 textCol = ToImU32(Theme::Current.palette.Text);
    const f32 minColWidth = 120.0f * dpi;
    const f32 maxColWidth = p.width * dpi;
    const f32 availY = ImGui::GetContentRegionAvail().y;
    int rowsPerColumn = (int)(availY / cellHeight);
    if (rowsPerColumn < 1) rowsPerColumn = 1;
    const int totalItems = (int)dirChildren.size();
    if (totalItems == 0){
        ImGui::EndChild();
        return;
    }
    const int totalColumns = (totalItems + rowsPerColumn - 1) / rowsPerColumn;
    std::vector<f32> colWidths(totalColumns, minColWidth);
    for (int i = 0; i < totalItems; i++){
        int c = i / rowsPerColumn;
        f32 textWidth = ImGui::CalcTextSize(dirChildren[i].name.c_str()).x;
        f32 requiredWidth = textWidth + iconSize + (xGap * 3.0f);
        if (requiredWidth > colWidths[c]) colWidths[c] = (std::min)(requiredWidth, maxColWidth);
    }
    std::vector<f32> colOffsets(totalColumns + 1, 0.0f);
    for (int c = 0; c < totalColumns; c++) colOffsets[c + 1] = colOffsets[c] + colWidths[c];
    f32 totalVirtualWidth = colOffsets[totalColumns];
    ImGui::Dummy(ImVec2(totalVirtualWidth, rowsPerColumn * cellHeight));
    f32 scrollX = ImGui::GetScrollX();
    f32 windowX = ImGui::GetWindowWidth();
    int startCol = 0;
    while (startCol < totalColumns && colOffsets[startCol + 1] < scrollX) startCol++;
    int endCol = startCol;
    while (endCol < totalColumns && colOffsets[endCol] < scrollX + windowX) endCol++;
    for (int c = startCol; c < endCol; c++){
        f32 currentColWidth = colWidths[c];
        f32 currentColOffset = colOffsets[c];
        for (int r = 0; r < rowsPerColumn; r++){
            int i = (c * rowsPerColumn) + r;
            if (i >= totalItems) break;

            const DirChild& child = dirChildren[i];
            
            bool isSelected = activeTab.isSelected(child.Hash(directory.parent.pidl));
            ImGui::PushID(i);
            ImGui::SetCursorPos(ImVec2(currentColOffset, (f32)r * cellHeight));
            ImVec2 cellScreenPos = ImGui::GetCursorScreenPos();
            ImRect fullRect(cellScreenPos, ImVec2(cellScreenPos.x + currentColWidth, cellScreenPos.y + cellHeight));
            ImGuiID id = window->GetID((void*)(intptr_t)i);
            ItemInteraction ia = HandleItemInteraction(app, directory.parent, child, id, fullRect);
            DrawSelectableBg(dl, fullRect, ia.hovered, isSelected, 4.0f * dpi);

            f32 iconY = cellScreenPos.y + (cellHeight - iconSize) * 0.5f;
            DrawItemIcon(dl, app, directory.parent, child, ImVec2(cellScreenPos.x + xGap, iconY), iconSize, SHIL_SMALL);
            f32 textStartX = cellScreenPos.x + xGap + iconSize + xGap;
            f32 maxTextWidth = currentColWidth - (xGap * 3.0f) - iconSize;
            if (maxTextWidth > 0.0f){
                ImRect textRect(ImVec2(textStartX, cellScreenPos.y), ImVec2(textStartX + maxTextWidth, cellScreenPos.y + cellHeight));
                DrawTextEllipsisSingleLine(dl, textRect, child.name.c_str(), textCol);
            }
            ImGui::PopID();
        }
    }
    ImGui::EndChild();
}

inline void RenderDetailsView(f32 dpi, App& app){
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (!window || window->SkipItems) return;
    ImDrawList* dl = window->DrawList;
    auto& activeTab = app.window.GetActiveTab();

    GridViewParams p = GetGridParamsForMode(ViewMode::Details);
    const f32 cellHeight = p.height * dpi;
    const f32 iconSize = 16.0f * dpi;
    const f32 nameWidth = p.width * dpi;
    const f32 dateWidth = 150.0f * dpi;
    const f32 typeWidth = 120.0f * dpi;
    const f32 sizeWidth = 100.0f * dpi;
    const ImU32 textCol = ToImU32(Theme::Current.palette.Text);

    // FIX 4: Removed ImGuiTableFlags_ScrollY. 
    // Tables with ScrollY fight with ImGuiListClipper. We wrap in a Child instead.
    ImGuiTableFlags flags =
        ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable |
        ImGuiTableFlags_PadOuterX | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoBordersInBody;

    ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, IM_COL32(0, 0, 0, 0));
    
    if (ImGui::BeginChild("DetailsChild", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_None)) {
        if (ImGui::BeginTable("ExplorerDetails", 4, flags)){
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, nameWidth);
            ImGui::TableSetupColumn("Date modified", ImGuiTableColumnFlags_WidthFixed, dateWidth);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, typeWidth);
            ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, sizeWidth);
            ImGui::TableHeadersRow();


            const Directory& directory = activeTab.directory;
            const std::vector<DirChild>& dirChildren = directory.children;

            ImGuiListClipper clipper;
            clipper.Begin((int)dirChildren.size(), cellHeight);

            while (clipper.Step()){
                for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++){
                    const DirChild& child = dirChildren[row];
                    bool isFolder = (child.attributes & SFGAO_FOLDER) != 0;
                    bool isSelected = activeTab.isSelected(child.Hash(directory.parent.pidl));
                    
                    ImGui::PushID(row);
                    ImGui::TableNextRow(ImGuiTableRowFlags_None, cellHeight);
                    ImGui::TableNextColumn();

                    ImVec2 cellPos = ImGui::GetCursorScreenPos();
                    f32 availWidth = ImGui::GetContentRegionAvail().x;
                    ImRect rowRect(cellPos, ImVec2(cellPos.x + ImGui::GetWindowWidth(), cellPos.y + cellHeight));
                    
                    ImGuiID id = window->GetID((void*)(intptr_t)row);
                    ItemInteraction ia = HandleItemInteraction(app, directory.parent, child, id, rowRect);
                    DrawSelectableBg(dl, rowRect, ia.hovered, isSelected);

                    f32 iconY = cellPos.y + (cellHeight - iconSize) * 0.5f;
                    DrawItemIcon(dl, app, directory.parent, child, ImVec2(cellPos.x + 4.0f * dpi, iconY), iconSize, SHIL_SMALL);
                    
                    f32 textX = cellPos.x + 4.0f * dpi + iconSize + 6.0f * dpi;
                    f32 maxTextWidth = availWidth - (textX - cellPos.x);
                    ImRect textRect(ImVec2(textX, cellPos.y), ImVec2(textX + maxTextWidth, cellPos.y + cellHeight));
                    DrawTextEllipsisSingleLine(dl, textRect, child.name.c_str(), textCol);

                    ImGui::TableNextColumn();
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (cellHeight - ImGui::GetTextLineHeight()) * 0.5f);
                    ImGui::TextColored(ImColor(ToImU32(Theme::Current.palette.TextMuted)), "-");

                    ImGui::TableNextColumn();
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (cellHeight - ImGui::GetTextLineHeight()) * 0.5f);
                    ImGui::TextUnformatted(KindLabel(child));

                    ImGui::TableNextColumn();
                    if (!isFolder){
                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (cellHeight - ImGui::GetTextLineHeight()) * 0.5f);
                        ImGui::TextColored(ImColor(ToImU32(Theme::Current.palette.TextMuted)), "-");
                    }
                    ImGui::PopID();
                }
            }
            ImGui::EndTable();
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleColor();
}

inline void RenderTilesView(f32 dpi, App& app){
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (!window || window->SkipItems) return;
    ImDrawList* dl = window->DrawList;
    auto& activeTab = app.window.GetActiveTab();

    GridViewParams p = GetGridParamsForMode(ViewMode::Tiles);
    const f32 cellW = p.width * dpi;
    const f32 cellH = p.height * dpi;
    const f32 iconSize = cellH * 0.7f;
    const f32 lineHeight = ImGui::GetTextLineHeight();
    const ImU32 textCol = ToImU32(Theme::Current.palette.Text);
    const ImU32 mutedCol = ToImU32(Theme::Current.palette.TextMuted);

    const Directory& directory = activeTab.directory;
    const std::vector<DirChild>& dirChildren = directory.children;

    ForEachGridCell(dirChildren.size(), cellW, cellH, [&](size_t i, ImVec2 cellPos){
        const DirChild& child = dirChildren[i];
        bool isSelected = activeTab.isSelected(child.Hash(directory.parent.pidl));
        ImRect fullRect(cellPos, ImVec2(cellPos.x + cellW, cellPos.y + cellH));

        ImGuiID id = window->GetID((void*)(intptr_t)i);
        ItemInteraction ia = HandleItemInteraction(app, directory.parent, child, id, fullRect);
        DrawSelectableBg(dl, fullRect, ia.hovered, isSelected, 4.0f * dpi);

        f32 iconX = cellPos.x + 6.0f * dpi;
        f32 iconY = cellPos.y + (cellH - iconSize) * 0.5f;
        DrawItemIcon(dl, app, directory.parent, child, ImVec2(iconX, iconY), iconSize, ShilSizeForMode(ViewMode::Tiles));

        f32 textX = iconX + iconSize + 8.0f * dpi;
        f32 textMaxWidth = cellPos.x + cellW - textX - 8.0f * dpi;
        if (textMaxWidth > 0.0f){
            ImRect nameRect(ImVec2(textX, cellPos.y + 4.0f * dpi), ImVec2(textX + textMaxWidth, cellPos.y + 4.0f * dpi + lineHeight));
            DrawTextEllipsisSingleLine(dl, nameRect, child.name.c_str(), textCol);
            dl->AddText(ImGui::GetFont(), ImGui::GetFontSize(),
                ImVec2(textX, cellPos.y + 4.0f * dpi + lineHeight + 2.0f * dpi),
                mutedCol, KindLabel(child));
        }
    });
}

inline void RenderFileGrid(f32 dpi, App& app){
    ViewMode mode = app.window.GetActiveTab().viewState.viewMode;
    app.window.GetActiveTab().directory.UpdateChildren();
    switch (mode){
        case ViewMode::ExtraLarge:
        case ViewMode::Large:
        case ViewMode::Medium:
            RenderGridView(dpi, app, mode);
            break;
        case ViewMode::Small:
            RenderSmallView(dpi, app);
            break;
        case ViewMode::List:
            RenderListView(dpi, app);
            break;
        case ViewMode::Details:
            RenderDetailsView(dpi, app);
            break;
        case ViewMode::Tiles:
            RenderTilesView(dpi, app);
            break;
    }
}