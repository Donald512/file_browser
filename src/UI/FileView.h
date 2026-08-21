
#pragma once
#include <string>
#include "WinFramework.h"
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
#include "TypenameManager.h"
#include "CtxMenuUI.h"

struct GridViewParams {
    f32 width;
    f32 height;
};

inline GridViewParams GetGridParamsForMode(ViewMode mode){
    switch (mode){
        case ViewMode::Small:      return {308.0f, 30.0f};
        case ViewMode::List:       return {308.0f, 30.0f};
        case ViewMode::Details:    return {308.0f, 30.0f};
        case ViewMode::Tiles:      return {250.0f, 52.0f};
    }
    return {250.0f, 52.0f};
}

inline int ShilSizeForMode(ViewMode mode){
    switch (mode){
        case ViewMode::Small:      return SHIL_SMALL;
        case ViewMode::List:       return SHIL_SMALL;
        case ViewMode::Details:    return SHIL_SMALL;
        case ViewMode::Tiles:      return SHIL_EXTRALARGE;
        default:                   return SHIL_EXTRALARGE;
    }
}

inline int ShiLSizeForIconSize(f32 iconSize){
    if (iconSize < 16) return SHIL_SMALL;
    if (iconSize < 32) return SHIL_LARGE;
    if (iconSize < 48) return SHIL_EXTRALARGE;
    return SHIL_JUMBO;
}


struct ItemInteraction {
    bool hovered;
    bool clicked;
};


inline ItemInteraction HandleItemInteraction(App& app, const DirParent& parent, const ItemView& child, ImGuiID id, const ImRect& rect){
    Interaction ia = MakeInteractive(id, rect);
    bool doubleClicked = IsDoubleClick(id, ia.pressed);
    bool rightClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ia.hovered;
    
    if (doubleClicked) {
        PCIDLIST_ABSOLUTE newPidl = GetFullPidl(parent.pidl.get(), child.pidl);
        // WShell::Pidl steals ownership
        if (child.IsFolder()){
            app.QueueCommand({CmdType::GoTo, WShell::Pidl(newPidl), 0, app.window.activeTabIndex, L""}); 
        }
        else app.QueueCommand({CmdType::OpenFile, WShell::Pidl(newPidl), {}, {}, L""});
    } 
    else if (ia.pressed) {
        app.window.GetActiveTab().SelectItem(child.hash, SelectMode::OneItem);
    }
    else if (rightClicked){
        if (app.window.GetActiveTab().isSelected(child.hash)){
            PCIDLIST_ABSOLUTE fullPidl = GetFullPidl(parent.pidl.get(), child.pidl);
            ctxMenuItems = GetContextMenu(activeContextMenu, fullPidl, app.gfx.d3dDevice.Get());
        }
        else{
            ctxMenuItems = GetBackgroundContextMenu(activeContextMenu, parent.pidl, app.gfx.d3dDevice.Get());
        }
        openRightClickMenu = true;
    }
    return {ia.hovered || ia.pressed};
}


inline void DrawItemIcon(ImDrawList* dl, App& app, const DirParent& parent, const ItemView& child, ImVec2 pos, f32 iconSize, int shilSize){
    ImTextureID iconTex = app.textures.GetTexture({app.icons.GetIconIndex(parent.shellFolder.Get(), child.pidl, child.hash), shilSize});
    
    if (iconTex){
        bool isHidden = (child.attributes & SFGAO_HIDDEN) != 0;
        ImU32 tint = isHidden ? IM_COL32(255, 255, 255, 128) : IM_COL32(255, 255, 255, 255);

        dl->AddImage(iconTex, pos, ImVec2(pos.x + iconSize, pos.y + iconSize), ImVec2(0, 0), ImVec2(1, 1), tint);
    } else {
        const char* fallback = child.IsFolder() ? ICON_REG_FOLDER : ICON_REG_DOCUMENT;
        DrawTextCenteredSingleLine(dl, pos, ImVec2(pos.x + iconSize, pos.y + iconSize), fallback, Theme::Current.palette.TextMuted, iconSize);
    }
}

// inline const char* KindLabel(const DirChild& item){
//     return (item.attributes & SFGAO_FOLDER) ? "File folder" : "File";
// }

// FIX 3: Removed ImGuiTable from Grid views. 
// ImGuiTable and ImGuiListClipper fight over Y-coordinates, causing overlaps and crashes.
inline void RenderGridView(f32 dpi, App& app){
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (!window || window->SkipItems) return;
    ImDrawList* dl = window->DrawList;
    auto& activeTab = app.window.GetActiveTab();
    
    const f32 padX = 16.0f * dpi;
    const f32 padY = 12.0f * dpi;

    ImVec2 areaMin = ImGui::GetCursorScreenPos();
    ImVec2 areaMax = ImVec2(areaMin.x + ImGui::GetContentRegionAvail().x, areaMin.y + ImGui::GetContentRegionAvail().y);

    ImVec2 contentMin = ImVec2(areaMin.x + padX, areaMin.y + padY);
    // f32 contentWidth = (areaMax.x - padX) - contentMin.x;


    auto& vs = app.window.GetActiveTab().viewState;
    const int shilSize = ShiLSizeForIconSize(vs.iconSize);
    const f32 imageSize = vs.iconSize * dpi;
    const f32 xGap = 8.0f * dpi;
    const f32 yTextPadding = 4.0f * dpi;
    const f32 lineHeight = ImGui::GetFontSize();
    const int maxLines = 3;
    const f32 itemWidth = imageSize * 1.2f;
    const f32 cellH = imageSize + yTextPadding + (maxLines * lineHeight) + yTextPadding;

    const Directory& tabDir = activeTab.dir;
    const auto& dirChildren = tabDir.children;
    const std::vector<u32>& dirChildrenRefs = tabDir.VisibleIndices(vs.showHidden);

    // Cells are itemWidth wide but stride by itemWidth + xGap, so there's a
    // visible gap between them.
    ForEachGridCell(dirChildrenRefs.size(), itemWidth + xGap, cellH, [&](size_t i, ImVec2 cellPos){
        auto child = dirChildren->GetItem(dirChildrenRefs[i], app.typeStore);

        bool isSelected = activeTab.isSelected(child.hash);
        ImRect fullRect(cellPos, ImVec2(cellPos.x + itemWidth, cellPos.y + cellH));

        ImGuiID id = window->GetID((void*)(intptr_t)child.hash);
        ItemInteraction ia = HandleItemInteraction(app, tabDir.parent, child, id, fullRect);
        DrawSelectableBg(dl, fullRect, ia.hovered, isSelected, 4.0f * dpi);


        f32 iconX = cellPos.x + (itemWidth - imageSize) * 0.5f;
        DrawItemIcon(dl, app, tabDir.parent, child, ImVec2(iconX, cellPos.y), imageSize, shilSize);

        f32 textY = cellPos.y + imageSize + yTextPadding;
        RenderTextWrappedCenteredEllipsis(
            dl,
            ImVec2(cellPos.x + 4.0f * dpi, textY),
            ImVec2(itemWidth - 8.0f * dpi, maxLines * lineHeight),
            child.name,
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
    const ImU32 textCol = Theme::Current.palette.Text;

    const Directory& tabDir = activeTab.dir;
    bool showHidden = activeTab.viewState.showHidden;
    const auto& dirChildren = tabDir.children;
    const std::vector<u32>& dirChildrenRefs = tabDir.VisibleIndices(showHidden);

    ForEachGridCell(dirChildrenRefs.size(), cellW, cellH, [&](size_t i, ImVec2 cellPos){
        auto child = dirChildren->GetItem(dirChildrenRefs[i], app.typeStore);

        bool isSelected = activeTab.isSelected(child.hash);
        ImRect fullRect(cellPos, ImVec2(cellPos.x + cellW, cellPos.y + cellH));

        ImGuiID id = window->GetID((void*)(intptr_t)child.hash);
        ItemInteraction ia = HandleItemInteraction(app, tabDir.parent, child, id, fullRect);
        DrawSelectableBg(dl, fullRect, ia.hovered, isSelected, 4.0f * dpi);

        f32 iconX = cellPos.x + iconPad;
        f32 iconY = cellPos.y + (cellH - iconSize) * 0.5f;
        DrawItemIcon(dl, app, tabDir.parent, child, ImVec2(iconX, iconY), iconSize, SHIL_SMALL);

        ImRect textRect(ImVec2(iconX + iconSize + textGap, cellPos.y),ImVec2(cellPos.x + cellW - 8.0f * dpi, cellPos.y + cellH));
        DrawTextEllipsisSingleLine(dl, textRect, child.name, textCol);
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

    const Directory& tabDir = activeTab.dir;
    bool showHidden = activeTab.viewState.showHidden;
    const auto& dirChildren = tabDir.children;
    const std::vector<u32>& dirChildrenRefs = tabDir.VisibleIndices(showHidden);

    const f32 xGap = 8.0f * dpi;
    const f32 cellHeight = p.height * dpi;
    const f32 iconSize = 16.0f * dpi;
    const ImU32 textCol = Theme::Current.palette.Text;
    const f32 minColWidth = 120.0f * dpi;
    const f32 maxColWidth = p.width * dpi;
    const f32 availY = ImGui::GetContentRegionAvail().y;
    int rowsPerColumn = (int)(availY / cellHeight);
    if (rowsPerColumn < 1) rowsPerColumn = 1;

    const int totalItems = (int)dirChildrenRefs.size();
    if (totalItems == 0){
        ImGui::EndChild();
        return;
    }
    const int totalColumns = (totalItems + rowsPerColumn - 1) / rowsPerColumn;
    std::vector<f32> colWidths(totalColumns, minColWidth);
    for (int i = 0; i < totalItems; i++){
        auto child = dirChildren->GetItem(dirChildrenRefs[i], app.typeStore);

        int c = i / rowsPerColumn;
        f32 textWidth = ImGui::CalcTextSize(child.name).x;
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
            auto child = dirChildren->GetItem(dirChildrenRefs[i], app.typeStore);

            bool isSelected = activeTab.isSelected(child.hash);
            ImGui::PushID(i);
            ImGui::SetCursorPos(ImVec2(currentColOffset, (f32)r * cellHeight));
            ImVec2 cellScreenPos = ImGui::GetCursorScreenPos();
            ImRect fullRect(cellScreenPos, ImVec2(cellScreenPos.x + currentColWidth, cellScreenPos.y + cellHeight));
            ImGuiID id = window->GetID((void*)(intptr_t)child.hash);
            ItemInteraction ia = HandleItemInteraction(app, tabDir.parent, child, id, fullRect);
            DrawSelectableBg(dl, fullRect, ia.hovered, isSelected, 4.0f * dpi);

            f32 iconY = cellScreenPos.y + (cellHeight - iconSize) * 0.5f;
            DrawItemIcon(dl, app, tabDir.parent, child, ImVec2(cellScreenPos.x + xGap, iconY), iconSize, SHIL_SMALL);
            f32 textStartX = cellScreenPos.x + xGap + iconSize + xGap;
            f32 maxTextWidth = currentColWidth - (xGap * 3.0f) - iconSize;
            if (maxTextWidth > 0.0f){
                ImRect textRect(ImVec2(textStartX, cellScreenPos.y), ImVec2(textStartX + maxTextWidth, cellScreenPos.y + cellHeight));
                DrawTextEllipsisSingleLine(dl, textRect, child.name, textCol);
            }
            ImGui::PopID();
        }
    }
    ImGui::EndChild();
}

inline const char* KindLabel(SFGAOF attributes) {
    return (attributes & SFGAO_FOLDER) ? "File folder" : "File";
}

inline const char* FormatFileSize(u64 size) {
    static thread_local char buf[64];
    if (size < 1024) snprintf(buf, sizeof(buf), "%llu B", size);
    else if (size < 1024*1024) snprintf(buf, sizeof(buf), "%.1f KB", size / 1024.0);
    else if (size < 1024*1024*1024) snprintf(buf, sizeof(buf), "%.1f MB", size / (1024.0*1024.0));
    else snprintf(buf, sizeof(buf), "%.2f GB", size / (1024.0*1024.0*1024.0));
    return buf;
}

inline const char* FormatFileTime(FILETIME ft) {
    static thread_local char buf[128];
    SYSTEMTIME st;
    FileTimeToSystemTime(&ft, &st);
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute);
    return buf;
}


inline void RenderDetailsView(f32 dpi, App& app){
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (!window || window->SkipItems) return;
    
    auto& activeTab = app.window.GetActiveTab();

    GridViewParams p = GetGridParamsForMode(ViewMode::Details);
    const f32 cellHeight = p.height * dpi;
    const f32 iconSize = 16.0f * dpi;
    f32 nameWidth = p.width * dpi;
    f32 dateWidth = 150.0f * dpi;
    f32 typeWidth = 120.0f * dpi;
    f32 sizeWidth = 100.0f * dpi;
    const ImU32 textCol = Theme::Current.palette.Text;
    const ImU32 mutedCol = Theme::Current.palette.TextMuted;

    ImGuiTableFlags flags =
        ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable |
        ImGuiTableFlags_PadOuterX | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoBordersInBody |
        ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoHostExtendX; // shouldnt NoH

        
    ImVec2 tableSize(0.0f, ImGui::GetContentRegionAvail().y);
        
    ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_TableBorderLight,  IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, IM_COL32(0, 0, 0, 0));

    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, Theme::Current.palette.SurfaceHover);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive,  Theme::Current.palette.SurfaceActive);
    
    if (ImGui::BeginTable("ExplorerDetails", 4, flags, tableSize)){
        ImGui::TableSetupScrollFreeze(0, 1); // Freezes the header row perfectly
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, nameWidth);
        ImGui::TableSetupColumn("Date modified", ImGuiTableColumnFlags_WidthFixed, dateWidth);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, typeWidth);
        ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, sizeWidth);
        ImGui::TableHeadersRow();


    const Directory& tabDir = activeTab.dir;
    bool showHidden = activeTab.viewState.showHidden;
    const auto& dirChildren = tabDir.children;
    const std::vector<u32>& dirChildrenRefs = tabDir.VisibleIndices(showHidden);

        ImGuiListClipper clipper;
        clipper.Begin((int)dirChildrenRefs.size(), cellHeight);
        
        while (clipper.Step()){
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++){
                auto child = dirChildren->GetItem(dirChildrenRefs[row], app.typeStore);

                bool isSelected = activeTab.isSelected(child.hash);
                
                ImGui::PushID(row);
                ImGui::TableNextRow(ImGuiTableRowFlags_None, cellHeight);
                ImGui::TableNextColumn();
                
                ImDrawList* dl = ImGui::GetWindowDrawList(); 
                ImGuiWindow* currentWindow = ImGui::GetCurrentWindow();
                ImVec2 cellPos = ImGui::GetCursorScreenPos();
                
                f32 tableMaxX = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
                ImVec2 cellPadding = ImGui::GetStyle().CellPadding;

                ImRect rowRect(
                    ImVec2(cellPos.x - cellPadding.x, cellPos.y - cellPadding.y), 
                    ImVec2(tableMaxX, cellPos.y + cellHeight - cellPadding.y)
                );
                
                // Expanded so ButtonBehavior sees the whole row
                ImGui::PushClipRect(rowRect.Min, ImVec2(tableMaxX, rowRect.Max.y), false);
                ImGuiID id = currentWindow->GetID((void*)(intptr_t)child.hash);
                ItemInteraction ia = HandleItemInteraction(app, tabDir.parent, child, id, rowRect);
                ImGui::PopClipRect();

                if (isSelected) ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, Theme::Current.palette.SurfaceActive);
                else if (ia.hovered) ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, Theme::Current.palette.SurfaceHover);

                // NAME ---
                f32 iconY = cellPos.y + (cellHeight - iconSize) * 0.5f;
                DrawItemIcon(dl, app, tabDir.parent, child, ImVec2(cellPos.x + 4.0f * dpi, iconY), iconSize, SHIL_SMALL);
                
                f32 textX = cellPos.x + 4.0f * dpi + iconSize + 6.0f * dpi;
                f32 maxTextWidth = ImGui::GetContentRegionAvail().x - (textX - cellPos.x);
                ImRect textRect(ImVec2(textX, cellPos.y), ImVec2(textX + maxTextWidth, cellPos.y + cellHeight));
                DrawTextEllipsisSingleLine(dl, textRect, child.name, textCol);

                // DATE ---
                ImGui::TableNextColumn();
                dl = ImGui::GetWindowDrawList(); 
                ImVec2 datePos = ImGui::GetCursorScreenPos();
                f32 dateLiveWidth = ImGui::GetContentRegionAvail().x;
                const char* dateText = (child.lastWriteTime.dwLowDateTime != 0 || child.lastWriteTime.dwHighDateTime != 0) ? FormatFileTime(child.lastWriteTime) : "--";
                ImRect dateRect(
                    ImVec2(datePos.x, datePos.y), 
                    ImVec2(datePos.x + dateLiveWidth, datePos.y + cellHeight)
                );
                DrawTextEllipsisSingleLine(dl, dateRect, dateText, mutedCol);
                
                // TYPE ---
                ImGui::TableNextColumn();
                dl = ImGui::GetWindowDrawList();
                ImVec2 typePos = ImGui::GetCursorScreenPos();
                f32 typeLiveWidth = ImGui::GetContentRegionAvail().x;
                const char* typeName = child.typeName[0] ? child.typeName : "--"; 
                // ImGui::RenderTextClipped(typePos, ImVec2(typePos.x + typeLiveWidth, typePos.y + cellHeight), typeName, nullptr, nullptr, ImVec2(0.0f, 0.5f), nullptr);
                ImRect typeRect(
                    ImVec2(typePos.x, typePos.y), 
                    ImVec2(typePos.x + typeLiveWidth, typePos.y + cellHeight)
                );
                DrawTextEllipsisSingleLine(dl, typeRect, typeName, mutedCol);


                // SIZE ---
                ImGui::TableNextColumn();
                dl = ImGui::GetWindowDrawList();
                ImVec2 sizePos = ImGui::GetCursorScreenPos();
                f32 liveSizeColWidth = ImGui::GetContentRegionAvail().x;
                const char* sizeText = (child.size != 0) ? FormatFileSize(child.size) : "--";
                ImGui::PushStyleColor(ImGuiCol_Text, mutedCol);
                ImGui::RenderTextClipped(sizePos, ImVec2(sizePos.x + liveSizeColWidth, sizePos.y + cellHeight), sizeText, nullptr, nullptr, ImVec2(1.0f, 0.5f), nullptr);
                ImGui::PopStyleColor();

                ImGui::PopID();
            }
        }
        ImGui::EndTable();
    }
    ImGui::PopStyleColor(5);
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
    const ImU32 textCol = Theme::Current.palette.Text;
    const ImU32 mutedCol = Theme::Current.palette.TextMuted;
    
    const Directory& tabDir = activeTab.dir;
    bool showHidden = activeTab.viewState.showHidden;
    const auto& dirChildren = tabDir.children;
    const std::vector<u32>& dirChildrenRefs = tabDir.VisibleIndices(showHidden);


    ForEachGridCell(dirChildrenRefs.size(), cellW, cellH, [&](size_t i, ImVec2 cellPos){
        auto child = dirChildren->GetItem(dirChildrenRefs[i], app.typeStore);

        bool isSelected = activeTab.isSelected(child.hash);
        ImRect fullRect(cellPos, ImVec2(cellPos.x + cellW, cellPos.y + cellH));

        ImGuiID id = window->GetID((void*)(intptr_t)child.hash);
        ItemInteraction ia = HandleItemInteraction(app, tabDir.parent, child, id, fullRect);
        DrawSelectableBg(dl, fullRect, ia.hovered, isSelected, 4.0f * dpi);

        f32 iconX = cellPos.x + 6.0f * dpi;
        f32 iconY = cellPos.y + (cellH - iconSize) * 0.5f;
        DrawItemIcon(dl, app, tabDir.parent, child, ImVec2(iconX, iconY), iconSize, ShilSizeForMode(ViewMode::Tiles));

        f32 textX = iconX + iconSize + 8.0f * dpi;
        f32 textMaxWidth = cellPos.x + cellW - textX - 8.0f * dpi;
        if (textMaxWidth > 0.0f){
            ImRect nameRect(ImVec2(textX, cellPos.y + 4.0f * dpi), ImVec2(textX + textMaxWidth, cellPos.y + 4.0f * dpi + lineHeight));
            DrawTextEllipsisSingleLine(dl, nameRect, child.name, textCol);
            const char* typeName = child.typeName[0] ? child.typeName : "--";
            f32 typeY = cellPos.y + 4.0f * dpi + lineHeight + 2.0f * dpi;
            ImRect typeRect(
                ImVec2(textX, typeY),
                ImVec2(textX + textMaxWidth, typeY + lineHeight)
            );
            DrawTextEllipsisSingleLine(dl, typeRect, typeName, mutedCol);
            // dl->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(textX, cellPos.y + 4.0f * dpi + lineHeight + 2.0f * dpi), mutedCol, typeName);
        }
    });
}

inline void RenderFileGrid(f32 dpi, App& app){
    FileViewState& vs = app.window.GetActiveTab().viewState;
    ViewMode mode = vs.viewMode;
    
    openRightClickMenu = false;
    
    app.window.GetActiveTab().dir.UpdateChildren(app.directory, app.typeStore, vs.sortMode, vs.sortDir, vs.showHidden);
    
    switch (mode){
        case ViewMode::Icons:
        RenderGridView(dpi, app);
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
    
    if (openRightClickMenu){
        ImGui::OpenPopup("ItemContextMenu");
        ImGui::SetNextWindowPos(ImGui::GetMousePos());
    }
    
    PushMenuTheme(dpi);
    if (ImGui::BeginPopup("ItemContextMenu")) {
        RenderContextMenuStructure(ctxMenuItems, app.gfx.hwnd, dpi);
        ImGui::EndPopup();
    }
    PopMenuTheme();
}