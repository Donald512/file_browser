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
#include "FileViewHelpers.h"


inline void RenderGridView(f32 dpi, App& app){
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (!window || window->SkipItems) return;
    ImDrawList* dl = window->DrawList;

    const auto layout = GetFileviewLayoutForMode(ViewMode::Icons, dpi);

    auto& vs = app.window.GetActiveTab().viewState;
    const int shilSize = ShiLSizeForIconSize(vs.iconSize);
    const f32 imageSize = vs.iconSize * dpi;
    const f32 yTextPadding = 4.0f * dpi;
    const f32 lineHeight = ImGui::GetFontSize();
    const int maxLines = 3;
    const f32 itemWidth = imageSize * kIconsWidthMultiplier;
    const f32 cellH = imageSize + yTextPadding + (maxLines * lineHeight) + yTextPadding;

    DirListing listing = GetVisibleListing(app);

    int focusedItemIndex = GetFocusedItemIndex(app);

    // Cells are itemWidth wide but stride by itemWidth + xGap, so there's a visible gap between them.
    ForEachGridCell(listing.refs.size(), itemWidth + layout.xGap, cellH + layout.yGap, [&](size_t i, ImVec2 cellPos){
        auto child = listing.dir.children->GetItem(listing.refs[i], app.typeStore);
        ImRect fullRect(cellPos, ImVec2(cellPos.x + itemWidth, cellPos.y + cellH));

        DrawItemChrome(dl, window, app, dpi, listing.dir.parent, child, (int)i, fullRect, 4.0f * dpi);

        f32 iconX = cellPos.x + (itemWidth - imageSize) * 0.5f;
        DrawItemIcon(dl, app, listing.dir.parent, child, ImVec2(iconX, cellPos.y), imageSize, shilSize);

        f32 textY = cellPos.y + imageSize + yTextPadding;
        RenderTextWrappedCenteredEllipsis(dl,
            ImVec2(cellPos.x + 4.0f * dpi, textY),
            ImVec2(itemWidth - 8.0f * dpi, maxLines * lineHeight),
            child.name, nullptr, maxLines
        );
    }, focusedItemIndex);
}

inline void RenderSmallView(f32 dpi, App& app){
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (!window || window->SkipItems) return;
    ImDrawList* dl = window->DrawList;

    const auto layout = GetFileviewLayoutForMode(ViewMode::Small, dpi);

    const f32 iconPad = 6.0f * dpi;
    const f32 textGap = 8.0f * dpi;
    const ImU32 textCol = Theme::Current.palette.Text;

    DirListing listing = GetVisibleListing(app);
    auto& activeTab = app.window.GetActiveTab();

    int focusedItemIndex = GetFocusedItemIndex(app);

    ForEachGridCell(listing.refs.size(), layout.cellWidth + layout.xGap, layout.cellWidth + layout.yGap, [&](size_t i, ImVec2 cellPos){
        auto child = listing.dir.children->GetItem(listing.refs[i], app.typeStore);
        ImRect fullRect(cellPos, ImVec2(cellPos.x + layout.cellWidth, cellPos.y + layout.cellWidth));

        DrawItemChrome(dl, window, app, dpi, listing.dir.parent, child, (int)i, fullRect, 4.0f * dpi);

        f32 iconX = cellPos.x + iconPad;
        f32 iconY = cellPos.y + (layout.cellWidth - layout.iconSize) * 0.5f;
        DrawItemIcon(dl, app, listing.dir.parent, child, ImVec2(iconX, iconY), layout.iconSize, SHIL_SMALL);

        ImRect textRect(ImVec2(iconX + layout.iconSize + textGap, cellPos.y), ImVec2(cellPos.x + layout.cellWidth - 8.0f * dpi, cellPos.y + layout.cellWidth));
        DrawTextEllipsisSingleLine(dl, textRect, child.name, textCol);
    }, focusedItemIndex);
}

inline void RenderListViewContent(f32 dpi, App& app){
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    ImVec2 winPos = ImGui::GetWindowPos();
    ImVec2 winContentMin = ImGui::GetWindowContentRegionMin();

    ImVec2 origin = ImVec2(winPos.x + winContentMin.x, winPos.y + winContentMin.y);
    
    if (ImGui::IsWindowHovered() && ImGui::GetIO().MouseWheel != 0.0f){
        f32 scrollAmount = ImGui::GetIO().MouseWheel * (60.0f * dpi);
        ImGui::SetScrollX(ImGui::GetScrollX() - scrollAmount);
    }

    DirListing listing = GetVisibleListing(app);
    auto& activeTab = app.window.GetActiveTab(); 
    auto& selState = activeTab.selState; 
    const auto layout = GetFileviewLayoutForMode(ViewMode::List, dpi);


    const f32 rowStride = layout.cellHeight + layout.yGap;

    const ImU32 textCol = Theme::Current.palette.Text;
    const f32 minColWidth = 120.0f * dpi;
    const f32 maxColWidth = layout.cellWidth;

    // Same function keyboard nav calls - guarantees the two can't disagree.
    const int rowsPerColumn = ComputeListRowsPerColumn(dpi);

    const int totalItems = (int)listing.refs.size();
    if (totalItems == 0) return;

    const int totalColumns = (totalItems + rowsPerColumn - 1) / rowsPerColumn;

    const f32 basePadding = layout.iconSize + (layout.xGap* 3.0f) + layout.padX;
    std::vector<f32> columnWidths = CalculateColumnWidthsForListView(basePadding, totalColumns, minColWidth, maxColWidth, rowsPerColumn, totalItems, listing, app);

    std::vector<f32> columnStarts = CalculateColumnStartsForListView(totalColumns, columnWidths);

    const f32 totalContentWidth = columnStarts[totalColumns];
    const f32 totalContentHeight = rowsPerColumn * rowStride;   // a row is top to bottom this time, not left to right
    ImGui::Dummy(ImVec2(totalContentWidth, totalContentHeight));

    f32 windowWidth = ImGui::GetWindowWidth();
    
    if (selState.justNavigated && activeTab.selState.focusHash.has_value()){
        KeepFocusedListColumnInView( windowWidth, columnStarts, rowsPerColumn, totalItems, listing, app, selState);
    }
    f32 scrollX = ImGui::GetScrollX();

    const auto visibleColumns = GetVisibleListColumns(scrollX, windowWidth, columnStarts, totalColumns);

    for (int c = visibleColumns.first; c < visibleColumns.last; c++){
        f32 currentColWidth = columnWidths[c];
        f32 currentColOffset = columnStarts[c];
        for (int r = 0; r < rowsPerColumn; r++){

            int i = (c * rowsPerColumn) + r;
            if (i >= totalItems) break;
            auto child = listing.dir.children->GetItem(listing.refs[i], app.typeStore);

            ImGui::PushID(i);
            ImGui::SetCursorPos(ImVec2(currentColOffset, (f32)r * rowStride));
            ImVec2 cellScreenPos = ImGui::GetCursorScreenPos();

            ImRect fullRect(cellScreenPos, ImVec2(cellScreenPos.x + currentColWidth - layout.xGap, cellScreenPos.y + layout.cellHeight));
            DrawItemChrome(dl, window, app, dpi, listing.dir.parent, child, i, fullRect, 4.0f * dpi);

            f32 iconY = cellScreenPos.y + (layout.cellHeight - layout.iconSize) * 0.5f;
            DrawItemIcon(dl, app, listing.dir.parent, child, ImVec2(cellScreenPos.x + layout.xGap, iconY), layout.iconSize, SHIL_SMALL);
            f32 textStartX = cellScreenPos.x + layout.xGap + layout.iconSize + layout.xGap;
            f32 maxTextWidth = currentColWidth - (layout.xGap * 3.0f) - layout.iconSize;
            if (maxTextWidth > 0.0f){
                ImRect textRect(ImVec2(textStartX, cellScreenPos.y), ImVec2(textStartX + maxTextWidth, cellScreenPos.y + layout.cellHeight));
                DrawTextEllipsisSingleLine(dl, textRect, child.name, textCol);
            }
            ImGui::PopID();
        }
    }
}

inline void RenderDetailsView(f32 dpi, App& app){
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (!window || window->SkipItems) return;


    const auto layout = GetFileviewLayoutForMode(ViewMode::Details, dpi);

    const ImU32 textCol = Theme::Current.palette.Text;
    const ImU32 mutedCol = Theme::Current.palette.TextMuted;


    ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_PadOuterX | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoHostExtendX |ImGuiTableFlags_ScrollX;

        
    ImVec2 tableSize(0.0f, ImGui::GetContentRegionAvail().y);
        
    ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_TableBorderLight,  IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, IM_COL32(0, 0, 0, 0));

    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, Theme::Current.palette.SurfaceHover);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive,  Theme::Current.palette.SurfaceActive);
    

    if (ImGui::BeginTable("ExplorerDetails", 4, flags, tableSize)){
        ImGui::TableSetupScrollFreeze(0, 1); // Freezes the header row perfectly
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableSetupColumn("Date modified", ImGuiTableColumnFlags_WidthStretch, 0.1f);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthStretch, 0.3f);
        ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthStretch, 0.1f);
        ImGui::TableHeadersRow();

        DirListing listing = GetVisibleListing(app);
        auto& activeTab = app.window.GetActiveTab();

        int focusRow = -1;
        if (activeTab.selState.justNavigated && activeTab.selState.focusHash.has_value()){
            for (int i = 0; i < (int)listing.refs.size(); i++){
                auto c = listing.dir.children->GetItem(listing.refs[i], app.typeStore);
                if (c.hash == activeTab.selState.focusHash) { focusRow = i; break; }
            }
        }

        if (focusRow >= 0) {
            f32 rowStride = layout.cellHeight + layout.yGap;
            f32 itemY = focusRow * rowStride;
            f32 scrollY = ImGui::GetScrollY();
            // Subtract roughly the header height so it doesn't hide under the frozen header
            f32 viewH = ImGui::GetWindowHeight() - ImGui::GetFrameHeight(); 
            
            if (itemY < scrollY) {
                ImGui::SetScrollY(itemY);
            } else if (itemY + layout.cellHeight > scrollY + viewH) {
                ImGui::SetScrollY(itemY + layout.cellHeight - viewH);
            }
        }

        ImGuiListClipper clipper;
        clipper.Begin((int)listing.refs.size(), layout.cellHeight + layout.yGap);
        
        while (clipper.Step()){
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++){
                auto child = listing.dir.children->GetItem(listing.refs[row], app.typeStore);

                bool isSelected = activeTab.isSelected(child.hash);
                
                ImGui::PushID(row);
                ImGui::TableNextRow(ImGuiTableRowFlags_None, layout.cellHeight + layout.yGap);
                ImGui::TableNextColumn();
                
                ImDrawList* dl = ImGui::GetWindowDrawList(); 
                ImGuiWindow* currentWindow = ImGui::GetCurrentWindow();
                ImVec2 cellPos = ImGui::GetCursorScreenPos();
                
                f32 tableMaxX = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
                ImVec2 cellPadding = ImGui::GetStyle().CellPadding;

                // restrict rowRect to cell Height ignoring layout.yGap
                ImRect rowRect(
                    ImVec2(cellPos.x - cellPadding.x, cellPos.y - cellPadding.y), 
                    ImVec2(tableMaxX, cellPos.y + layout.cellHeight - cellPadding.y)
                );
                
                // Expanded so ButtonBehavior sees the whole row
                ImGui::PushClipRect(rowRect.Min, ImVec2(tableMaxX, rowRect.Max.y), false);
                ImGuiID id = currentWindow->GetID((void*)(intptr_t)child.hash);
                ItemInteraction ia = HandleItemInteraction(app, listing.dir.parent, child, row, id, rowRect);
                ImGui::PopClipRect();

                if (isSelected) {
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, Theme::Current.palette.SurfaceActive);
                } 
                else if (ia.hovered) { 
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, Theme::Current.palette.SurfaceHover);
                }

                
                // NAME ---
                f32 iconY = cellPos.y + (layout.cellHeight - layout.iconSize) * 0.5f;
                DrawItemIcon(dl, app, listing.dir.parent, child, ImVec2(cellPos.x + 4.0f * dpi, iconY), layout.iconSize, SHIL_SMALL);
                
                f32 textX = cellPos.x + 4.0f * dpi + layout.iconSize + 6.0f * dpi;
                f32 maxTextWidth = ImGui::GetContentRegionAvail().x - (textX - cellPos.x);
                ImRect textRect(ImVec2(textX, cellPos.y), ImVec2(textX + maxTextWidth, cellPos.y + layout.cellHeight));
                DrawTextEllipsisSingleLine(dl, textRect, child.name, textCol);

                // DATE ---
                ImGui::TableNextColumn();                                                       
                dl = ImGui::GetWindowDrawList(); 
                ImVec2 datePos = ImGui::GetCursorScreenPos();
                f32 dateLiveWidth = ImGui::GetContentRegionAvail().x;
                const char* dateText = (child.lastWriteTime.dwLowDateTime != 0 || child.lastWriteTime.dwHighDateTime != 0) ? FormatFileTime(child.lastWriteTime) : "--";
                ImRect dateRect(
                    ImVec2(datePos.x, datePos.y), 
                    ImVec2(datePos.x + dateLiveWidth, datePos.y + layout.cellHeight)
                );
                DrawTextEllipsisSingleLine(dl, dateRect, dateText, mutedCol);
                
                // TYPE ---
                ImGui::TableNextColumn();
                dl = ImGui::GetWindowDrawList();
                ImVec2 typePos = ImGui::GetCursorScreenPos();
                f32 typeLiveWidth = ImGui::GetContentRegionAvail().x;
                const char* typeName = child.typeName[0] ? child.typeName : "--"; 
                ImRect typeRect(
                    ImVec2(typePos.x, typePos.y), 
                    ImVec2(typePos.x + typeLiveWidth, typePos.y + layout.cellHeight)
                );
                DrawTextEllipsisSingleLine(dl, typeRect, typeName, mutedCol);


                // SIZE ---
                ImGui::TableNextColumn();
                dl = ImGui::GetWindowDrawList();
                ImVec2 sizePos = ImGui::GetCursorScreenPos();
                f32 liveSizeColWidth = ImGui::GetContentRegionAvail().x;
                const char* sizeText = (child.size != 0) ? FormatFileSize(child.size) : "--";
                ImGui::PushStyleColor(ImGuiCol_Text, mutedCol);
                ImGui::RenderTextClipped(sizePos, ImVec2(sizePos.x + liveSizeColWidth, sizePos.y + layout.cellHeight), sizeText, nullptr, nullptr, ImVec2(1.0f, 0.5f), nullptr);
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

    const auto layout = GetFileviewLayoutForMode(ViewMode::Tiles, dpi);

    const f32 lineHeight = ImGui::GetTextLineHeight();
    const ImU32 textCol = Theme::Current.palette.Text;
    const ImU32 mutedCol = Theme::Current.palette.TextMuted;

    DirListing listing = GetVisibleListing(app);
    auto& activeTab = app.window.GetActiveTab();

    int focusedItemIndex = -1;
    if (activeTab.selState.justNavigated && activeTab.selState.focusHash.has_value()){
        for (size_t i = 0; i < listing.refs.size(); i++){
            auto c = listing.dir.children->GetItem(listing.refs[i], app.typeStore);
            if (c.hash == activeTab.selState.focusHash) { focusedItemIndex = (int)i; break; }
        }
    }

    ForEachGridCell(listing.refs.size(), layout.cellWidth + layout.xGap, layout.cellHeight + layout.yGap, [&](size_t i, ImVec2 cellPos){
        auto child = listing.dir.children->GetItem(listing.refs[i], app.typeStore);
        ImRect fullRect(cellPos, ImVec2(cellPos.x + layout.cellWidth, cellPos.y + layout.cellHeight));

        DrawItemChrome(dl, window, app, dpi, listing.dir.parent, child, (int)i, fullRect, 4.0f * dpi);

        f32 iconX = cellPos.x + 6.0f * dpi;
        f32 iconY = cellPos.y + (layout.cellHeight - layout.iconSize) * 0.5f;
        DrawItemIcon(dl, app, listing.dir.parent, child, ImVec2(iconX, iconY), layout.iconSize, ShilSizeForMode(ViewMode::Tiles));

        f32 textX = iconX + layout.iconSize + 8.0f * dpi;
        f32 textMaxWidth = cellPos.x + layout.cellWidth - textX - 8.0f * dpi;
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
        }
    }, focusedItemIndex);
}


inline void KeyboardNavigationInteraction(f32 dpi, App& app){
    auto& activeTab = app.window.GetActiveTab();
    FileViewState& vs = activeTab.viewState;
    ViewMode mode = vs.viewMode;
    SelectionState& selState = activeTab.selState;

    selState.justNavigated = false; 

    DirListing listing = GetVisibleListing(app);
    int totalItems = (int)listing.refs.size();

    // Early exit and clean reset if empty
    if (totalItems == 0) {
        selState.focusHash = 0;
        selState.anchorVisualIndex = -1;
        selState.anchorHash = 0;
        return;
    }

    // Find current focus index strictly by hash
    int focusIdx = -1;
    bool foundFocusIdx = false;
    if (selState.focusHash.has_value()){
        for (int i = 0; i < totalItems; i++) {
            auto c = listing.dir.children->GetItem(listing.refs[i], app.typeStore);
            if (c.hash == selState.focusHash) { 
                focusIdx = i; 
                foundFocusIdx = true;
                break; 
            }
        }
    }
        
    // If focus is lost, invalid, or 0, reset it
    if (!foundFocusIdx) {
        selState.focusHash = std::nullopt;
        selState.anchorVisualIndex = -1;
        selState.anchorHash = std::nullopt;
    }

    bool shift = ImGui::GetIO().KeyShift;
    bool ctrl = ImGui::GetIO().KeyCtrl;
    int newFocusIdx = focusIdx;
    bool navOccurred = false;


    int columns = 1;
    int rowsPerColumn = 1;
    f32 availW = ImGui::GetContentRegionAvail().x;

    if (mode == ViewMode::List) {
        rowsPerColumn = ComputeListRowsPerColumn(dpi);
    }
    else if (mode == ViewMode::Details) {
        columns = 1;
    } 
    else {
        f32 itemStride = GetGridItemStride(mode, dpi, vs.iconSize);
        if (itemStride > 0.0f) {
            columns = (int)(availW / itemStride);
            if (columns < 1) columns = 1;
        }
    }
    ImGuiKey keyPressed = ImGuiKey_None;

    // Handle Input (Only if window is hovered, including child windows)
    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_ChildWindows)){
        
        // Arrow Keys: Branch logic for List (vertical wrap) vs Grid (horizontal wrap)
        if (mode == ViewMode::List) {
            if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))  { 
                newFocusIdx += 1; 
                navOccurred = true; 
                keyPressed = ImGuiKey_DownArrow;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))    { 
                newFocusIdx -= 1; 
                navOccurred = true; 
                keyPressed = ImGuiKey_UpArrow;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) { 
                newFocusIdx += rowsPerColumn; 
                navOccurred = true; 
                keyPressed = ImGuiKey_RightArrow;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))  { 
                newFocusIdx -= rowsPerColumn; 
                navOccurred = true; 
                keyPressed = ImGuiKey_LeftArrow;
            }
        }
        else if (mode == ViewMode::Details) {
            if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) { newFocusIdx += 1; navOccurred = true; keyPressed = ImGuiKey_DownArrow; }
            if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))   { newFocusIdx -= 1; navOccurred = true; keyPressed = ImGuiKey_UpArrow; }
            // Left/Right: no horizontal axis in a single-column table, so no-op
        }
        else {
            if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))  { 
                newFocusIdx += columns; 
                navOccurred = true; 
                keyPressed = ImGuiKey_DownArrow;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))    { 
                newFocusIdx -= columns; 
                navOccurred = true; 
                keyPressed = ImGuiKey_UpArrow;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) { 
                newFocusIdx += 1; 
                navOccurred = true; 
                keyPressed = ImGuiKey_RightArrow;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))  { 
                newFocusIdx -= 1; 
                navOccurred = true; 
                keyPressed = ImGuiKey_LeftArrow;
            }
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Home))       { newFocusIdx = 0; navOccurred = true; }
        if (ImGui::IsKeyPressed(ImGuiKey_End))        { newFocusIdx = totalItems - 1; navOccurred = true; }
        if (ImGui::IsKeyPressed(ImGuiKey_PageDown))   { newFocusIdx += (mode == ViewMode::List ? rowsPerColumn : columns) * 10; navOccurred = true; }
        if (ImGui::IsKeyPressed(ImGuiKey_PageUp))     { newFocusIdx -= (mode == ViewMode::List ? rowsPerColumn : columns) * 10; navOccurred = true; }

        if (navOccurred){
            selState.justNavigated = true;
            selState.lastKeyboardNavTime = ImGui::GetTime();

            std::cout << "NAV OCCURED ====================================================>" << std::endl;
            const char* keyName = ImGui::GetKeyName(keyPressed);
            printf("Key Pressed: %s\n", keyName);
            DEBUGPrintFocusedItems(app);

            newFocusIdx = std::clamp(newFocusIdx, 0, totalItems - 1);
            auto newChild = listing.dir.children->GetItem(listing.refs[newFocusIdx], app.typeStore);
             
            if (shift) {
                int start = (std::min)(selState.anchorVisualIndex, newFocusIdx);
                int end = (std::max)(selState.anchorVisualIndex, newFocusIdx);
                if (!ctrl) activeTab.DeselectAllItems();
                for (int i = start; i <= end; i++) {
                    auto c = listing.dir.children->GetItem(listing.refs[i], app.typeStore);
                    activeTab.AddItemToSelection(c.hash);
                }
            }
            else if (!ctrl){
                activeTab.DeselectAllItemsAndSelect(newChild.hash);
                selState.anchorVisualIndex = newFocusIdx;
                selState.anchorHash = newChild.hash;
            }
            
            // Update focus hash ONLY on explicit navigation
            selState.focusHash = newChild.hash;
        }

        // Spacebar: Toggle selection of focused item
        if (ImGui::IsKeyPressed(ImGuiKey_Space)) {
            auto focusChild = listing.dir.children->GetItem(listing.refs[focusIdx], app.typeStore);
            if (activeTab.isSelected(focusChild.hash)){
                activeTab.DeselectItem(focusChild.hash);
            } else {
                activeTab.AddItemToSelection(focusChild.hash);
            }
        }
        
        // Enter: Open / Navigate
        if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) {
            auto focusChild = listing.dir.children->GetItem(listing.refs[focusIdx], app.typeStore);
            PCIDLIST_ABSOLUTE newPidl = GetFullPidl(listing.dir.parent.pidl.get(), focusChild.pidl);
            if (focusChild.IsFolder()){
                app.QueueCommand({CmdType::GoTo, WShell::Pidl(newPidl), 0, app.window.activeTabIndex, L""}); 
            } else {
                app.QueueCommand({CmdType::OpenFile, WShell::Pidl(newPidl), {}, {}, L""});
            }
        }
    }
}

inline void RenderFileGrid(f32 dpi, App& app){
    auto& activeTab = app.window.GetActiveTab();
    FileViewState& vs = activeTab.viewState;
    ViewMode mode = vs.viewMode;

    // Reset hover state at the beginning of the frame
    activeTab.selState.isAnyItemHovered = false; 
    
    openRightClickMenu = false;
    
    activeTab.dir.UpdateChildren(app.directory, app.typeStore, vs.sortMode, vs.sortDir, vs.showHidden);

    if (mode == ViewMode::List){
        // List view lives inside its own scrolling child, and only there is
        // the real available height (after child padding + scrollbar
        // reservation) known. Keyboard nav has to run INSIDE that same child
        // so its row math can never drift from what's actually drawn - that
        // drift was the source of the diagonal-jump / phantom-selection bug.
        ImGuiChildFlags childFlags = ImGuiChildFlags_NavFlattened;
        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_HorizontalScrollbar;
        if (ImGui::BeginChild("FileViewList", ImVec2(0, 0), childFlags, windowFlags)){
            KeyboardNavigationInteraction(dpi, app);
            RenderListViewContent(dpi, app);
        }
        ImGui::EndChild();
    } else {
        KeyboardNavigationInteraction(dpi, app);
        switch (mode){
            case ViewMode::Icons:   RenderGridView(dpi, app); break;
            case ViewMode::Small:   RenderSmallView(dpi, app); break;
            case ViewMode::Details: RenderDetailsView(dpi, app); break;
            case ViewMode::Tiles:   RenderTilesView(dpi, app); break;
            default: break;
        }
    }

    //  Add ImGuiHoveredFlags_ChildWindows to catch clicks inside BeginChild (List) and Tables (Details)
    bool isWindowHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByPopup);

    // Left Click on Empty Space
    if (isWindowHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)){
        if (!activeTab.selState.isAnyItemHovered) {
            activeTab.DeselectAllItems();
        }
    }
    if (isWindowHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)){
        if (!activeTab.selState.isAnyItemHovered) {
            activeTab.DeselectAllItems();
            ctxMenuItems = GetBackgroundContextMenu(activeContextMenu, activeTab.dir.parent.pidl, app.gfx.d3dDevice.Get());
            openRightClickMenu = true;
        }
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

