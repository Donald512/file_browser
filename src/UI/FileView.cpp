#pragma once
#include <string>
#include "WinFramework.h"
#include "imgui.h"
// #include "imgui_internal.h"
#include "ImGuiHelpers.h"
#include "UIglobals.h"
#include "theme.h"
// #include "iconRegular.h"
#include <cfloat>
#include <cmath>
#include <algorithm>
#include <vector>
#include <string>
#include "TypenameManager.h"
#include "CtxMenu.h"
#include "CtxMenuUI.h"
#include "FileViewHelpers.h"

#include "FileView.h"


static void RenderGridView(f32 dpi, App& app){
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

    auto& activeTab = app.window.GetActiveTab();
    DirListing listing = GetVisibleListing(app);


    int focusedItemIndex = -1;
    if (activeTab.selState.justNavigated && activeTab.selState.focusHash.has_value()){
        focusedItemIndex = GetFocusedItemIndex(app);
    }

    // Cells are itemWidth wide but stride by itemWidth + xGap, so there's a visible gap between them.
    ForEachGridCell(listing.refs.size(), itemWidth + layout.xGap, cellH + layout.yGap, [&](size_t i, ImVec2 cellPos){
        auto child = listing.PChildren->GetItem(listing.refs[i], app.typeStore);
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

static void RenderSmallView(f32 dpi, App& app){
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (!window || window->SkipItems) return;
    ImDrawList* dl = window->DrawList;

    const auto layout = GetFileviewLayoutForMode(ViewMode::Small, dpi);

    const f32 iconPad = 6.0f * dpi;
    const f32 textGap = 8.0f * dpi;
    const ImU32 textCol = Theme::Current.palette.Text;

    DirListing listing = GetVisibleListing(app);
    auto& activeTab = app.window.GetActiveTab();

    int focusedItemIndex = -1;
    if (activeTab.selState.justNavigated && activeTab.selState.focusHash.has_value()){
        focusedItemIndex = GetFocusedItemIndex(app);
    }

    ForEachGridCell(listing.refs.size(), layout.cellWidth + layout.xGap, layout.cellHeight + layout.yGap, [&](size_t i, ImVec2 cellPos){
        auto child = listing.PChildren->GetItem(listing.refs[i], app.typeStore);
        ImRect fullRect(cellPos, ImVec2(cellPos.x + layout.cellWidth, cellPos.y + layout.cellHeight));

        DrawItemChrome(dl, window, app, dpi, listing.dir.parent, child, (int)i, fullRect, 4.0f * dpi);

        f32 iconX = cellPos.x + iconPad;
        f32 iconY = cellPos.y + (layout.cellHeight - layout.iconSize) * 0.5f;
        DrawItemIcon(dl, app, listing.dir.parent, child, ImVec2(iconX, iconY), layout.iconSize, SHIL_SMALL);

        ImRect textRect(ImVec2(iconX + layout.iconSize + textGap, cellPos.y), ImVec2(cellPos.x + layout.cellWidth - 8.0f * dpi, cellPos.y + layout.cellHeight));
        DrawTextEllipsisSingleLine(dl, textRect, child.name, textCol);
    }, focusedItemIndex);
}

static void RenderListViewContent(f32 dpi, App& app){
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
    auto& renameState = activeTab.renameState;
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
    
    f32 scrollX = ImGui::GetScrollX();
    if (selState.justNavigated && activeTab.selState.focusHash.has_value()){

        int focusedItemIndex = GetFocusedItemIndex(app);
        
        int focusCol = focusedItemIndex / rowsPerColumn;
        f32 colLeft  = columnStarts[focusCol];
        f32 colRight = columnStarts[focusCol + 1];
        f32 colWidth = colRight - colLeft;

        f32 newScrollX = KeepRectVisible(colLeft, colWidth, scrollX, windowWidth);
        if (newScrollX != scrollX) ImGui::SetScrollX(newScrollX);

    }

    const auto visibleColumns = GetVisibleListColumns(scrollX, windowWidth, columnStarts, totalColumns);

    for (int c = visibleColumns.first; c < visibleColumns.last; c++){
        f32 currentColWidth = columnWidths[c];
        f32 currentColOffset = columnStarts[c];
        for (int r = 0; r < rowsPerColumn; r++){

            int i = (c * rowsPerColumn) + r;
            if (i >= totalItems) break;
            auto child = listing.PChildren->GetItem(listing.refs[i], app.typeStore);

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
                if (renameState.renamingItemId == child.hash){
                    bool isSelected = activeTab.isSelected(child.hash);
                    ImU32 bgCol = isSelected ? Theme::Current.palette.SurfaceActive : Theme::Current.palette.Surface;

                    ImVec2 baseSize = textRect.GetSize();
                    ImVec2 maxSize = ImVec2(maxTextWidth, baseSize.y);

                    RenderRenameWidget("listRename", textRect.Min, baseSize, maxSize, GrowAxis::X, app.gfx.hwnd, renameState, listing.dir.parent.pidl.get(), child.pidl, child.name, bgCol);
                }
                else DrawTextEllipsisSingleLine(dl, textRect, child.name, textCol);
            }
            ImGui::PopID();
        }
    }
}

static void RenderDetailsView(f32 dpi, App& app){
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (!window || window->SkipItems) return;

    const auto layout = GetFileviewLayoutForMode(ViewMode::Details, dpi);
    auto& activeTab = app.window.GetActiveTab();
    auto& renameState = activeTab.renameState;

    const ImU32 textCol = Theme::Current.palette.Text;
    const ImU32 mutedCol = Theme::Current.palette.TextMuted;

    ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_PadOuterX | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_ScrollY;

    ImVec2 tableSize(0.0f, ImGui::GetContentRegionAvail().y);
        
    ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_TableBorderLight,  IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, IM_COL32(0, 0, 0, 0));

    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, Theme::Current.palette.SurfaceHover);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive,  Theme::Current.palette.SurfaceActive);
    
    if (ImGui::BeginTable("ExplorerDetails", 4, flags, tableSize)){
        ImGui::GetCurrentWindow()->Flags |= ImGuiWindowFlags_NoNavInputs; 
        ImGui::TableSetupScrollFreeze(0, 1); // Freezes the header row perfectly
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 0.5f);
        ImGui::TableSetupColumn("Date modified", ImGuiTableColumnFlags_WidthStretch, 0.1f);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthStretch, 0.3f);
        ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthStretch, 0.1f);
        ImGui::TableHeadersRow();

        DirListing listing = GetVisibleListing(app);

        int focusRow = -1;

        f32 headerHeight = ImGui::GetFrameHeight() + ImGui::GetStyle().CellPadding.y * 2.0f;
        f32 headerBottomY = window->DC.CursorStartPos.y + headerHeight;

        f32 actualViewH = ImGui::GetWindowHeight() - headerHeight;

        if (activeTab.selState.justNavigated && activeTab.selState.focusHash.has_value()) {
            focusRow = GetFocusedItemIndex(app);    // in details view, focused item is same as focused row
            
            f32 rowStride = layout.cellHeight + layout.yGap;
            f32 itemMinY = focusRow * rowStride;
            f32 scrollY = ImGui::GetScrollY();

            f32 newScrollY = KeepRectVisible(itemMinY, rowStride, scrollY, actualViewH);
            if (newScrollY != scrollY) ImGui::SetScrollY(newScrollY);
        }

        ImGuiListClipper clipper;
        clipper.Begin((int)listing.refs.size(), layout.cellHeight + layout.yGap);
        
        while (clipper.Step()){
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++){
                auto child = listing.PChildren->GetItem(listing.refs[row], app.typeStore);
                
                bool isSelected = activeTab.isSelected(child.hash);
                
                ImGui::PushID(row);
                ImGui::TableNextRow(ImGuiTableRowFlags_None, layout.cellHeight);
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
                ImGui::PushClipRect(
                    ImVec2(rowRect.Min.x, ImMax(rowRect.Min.y, headerBottomY)), 
                    ImVec2(tableMaxX, ImMin(rowRect.Max.y, actualViewH + window->Pos.y)), false);
                ImU32 bgCol = 0;
                ImGuiID id = currentWindow->GetID((void*)(intptr_t)child.hash);
                ItemInteraction ia = HandleItemInteraction(app, listing.dir.parent, child, row, id, rowRect);
                
                bool isFocused = activeTab.selState.focusHash == child.hash;
                if (isFocused){
                    ImVec2 minP = ImVec2(rowRect.Min.x + 1.0f, rowRect.Min.y + 1.0f);
                    ImVec2 maxP = ImVec2(rowRect.Max.x - 1.0f, rowRect.Max.y - 1.0f);
                    dl->AddRect(minP, maxP, Theme::Current.palette.SurfaceActive, 4.0f * dpi, 0, 2.0f * dpi);
                }
                ImGui::PopClipRect();
                if (isSelected)      bgCol = Theme::Current.palette.SurfaceActive;
                else if (ia.hovered) bgCol = Theme::Current.palette.SurfaceHover;
                else if (row & 1)    bgCol = ImGui::GetColorU32(ImGuiCol_TableRowBgAlt);
                
                // if (bgCol != 0)      dl->AddRectFilled(rowRect.Min, rowRect.Max, bgCol);
                if (bgCol != 0)      ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, bgCol);
                
                // NAME ---
                f32 iconY = cellPos.y + (layout.cellHeight - layout.iconSize) * 0.5f;
                DrawItemIcon(dl, app, listing.dir.parent, child, ImVec2(cellPos.x + 4.0f * dpi, iconY), layout.iconSize, SHIL_SMALL);
                
                f32 textX = cellPos.x + 4.0f * dpi + layout.iconSize + 6.0f * dpi;
                f32 maxTextWidth = ImGui::GetContentRegionAvail().x - (textX - cellPos.x);
                if (renameState.renamingItemId == child.hash){
                    ImVec2 baseSize(maxTextWidth, rowRect.GetHeight());
                    ImVec2 maxSize(ImGui::GetContentRegionMax().x - textX, rowRect.GetHeight()); // grow into the row, not downward
                    RenderRenameWidget("detailsRename", ImVec2(textX, rowRect.Min.y), baseSize, maxSize, GrowAxis::X, app.gfx.hwnd, renameState, listing.dir.parent.pidl.get(), child.pidl, child.name, bgCol);
                }
                else{
                    ImRect textRect(ImVec2(textX, rowRect.Min.y), ImVec2(textX + maxTextWidth, rowRect.Max.y));   
                    DrawTextEllipsisSingleLine(dl, textRect, child.name, textCol);
                }



                // DATE ---
                ImGui::TableNextColumn();                                                       
                dl = ImGui::GetWindowDrawList(); 
                ImVec2 datePos = ImGui::GetCursorScreenPos();
                f32 dateLiveWidth = ImGui::GetContentRegionAvail().x;
                const char* dateText = (child.lastWriteTime.dwLowDateTime != 0 || child.lastWriteTime.dwHighDateTime != 0) ? FormatFileTime(child.lastWriteTime) : "--";
                ImRect dateRect(
                    ImVec2(datePos.x, rowRect.Min.y), 
                    ImVec2(datePos.x + dateLiveWidth, rowRect.Max.y)
                );
                DrawTextEllipsisSingleLine(dl, dateRect, dateText, mutedCol);
                
                // TYPE ---
                ImGui::TableNextColumn();
                dl = ImGui::GetWindowDrawList();
                ImVec2 typePos = ImGui::GetCursorScreenPos();
                f32 typeLiveWidth = ImGui::GetContentRegionAvail().x;
                const char* typeName = child.typeName[0] ? child.typeName : "--"; 
                ImRect typeRect(
                    ImVec2(typePos.x, rowRect.Min.y), 
                    ImVec2(typePos.x + typeLiveWidth, rowRect.Max.y)
                );
                DrawTextEllipsisSingleLine(dl, typeRect, typeName, mutedCol);


                // SIZE ---
                ImGui::TableNextColumn();
                dl = ImGui::GetWindowDrawList();
                ImVec2 sizePos = ImGui::GetCursorScreenPos();
                f32 liveSizeColWidth = ImGui::GetContentRegionAvail().x - layout.xGap;
                const char* sizeText = (child.size != 0) ? FormatFileSize(child.size) : "--";
                ImGui::PushStyleColor(ImGuiCol_Text, mutedCol);
                ImGui::RenderTextClipped(sizePos, ImVec2(sizePos.x + liveSizeColWidth, rowRect.Max.y), sizeText, nullptr, nullptr, ImVec2(1.0f, 0.5f), nullptr);
                ImGui::PopStyleColor();

                ImGui::PopID();

                ImGui::TableNextRow(ImGuiTableRowFlags_None, layout.yGap);
            }
        }
        ImGui::EndTable();
    }
    ImGui::PopStyleColor(5);
}

static void RenderTilesView(f32 dpi, App& app){
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (!window || window->SkipItems) return;
    ImDrawList* dl = window->DrawList;

    const auto layout = GetFileviewLayoutForMode(ViewMode::Tiles, dpi);

    const f32 lineHeight = ImGui::GetTextLineHeight();
    const ImU32 textCol = Theme::Current.palette.Text;
    const ImU32 mutedCol = Theme::Current.palette.TextMuted;

    DirListing listing = GetVisibleListing(app);
    auto& activeTab = app.window.GetActiveTab();
    auto& renameState = activeTab.renameState;

    int focusedItemIndex = -1;
    if (activeTab.selState.justNavigated && activeTab.selState.focusHash.has_value()){
        focusedItemIndex = GetFocusedItemIndex(app);
    }

    ForEachGridCell(listing.refs.size(), layout.cellWidth + layout.xGap, layout.cellHeight + layout.yGap, [&](size_t i, ImVec2 cellPos){
        auto child = listing.PChildren->GetItem(listing.refs[i], app.typeStore);
        ImRect fullRect(cellPos, ImVec2(cellPos.x + layout.cellWidth, cellPos.y + layout.cellHeight));

        DrawItemChrome(dl, window, app, dpi, listing.dir.parent, child, (int)i, fullRect, 4.0f * dpi);

        f32 iconX = cellPos.x + 6.0f * dpi;
        f32 iconY = cellPos.y + (layout.cellHeight - layout.iconSize) * 0.5f;
        DrawItemIcon(dl, app, listing.dir.parent, child, ImVec2(iconX, iconY), layout.iconSize, ShilSizeForMode(ViewMode::Tiles));

        f32 textX = iconX + layout.iconSize + 8.0f * dpi;
        f32 textMaxWidth = cellPos.x + layout.cellWidth - textX - 8.0f * dpi;
        if (textMaxWidth > 0.0f){
            ImRect nameRect(ImVec2(textX, cellPos.y + 4.0f * dpi), ImVec2(textX + textMaxWidth, cellPos.y + 4.0f * dpi + lineHeight));
            if (renameState.renamingItemId == child.hash){
                
                bool isSelected = activeTab.isSelected(child.hash);
                ImU32 bgCol = isSelected ? Theme::Current.palette.SurfaceActive : Theme::Current.palette.Surface;

                ImVec2 baseSize = nameRect.GetSize();
                ImVec2 maxSize = ImVec2(textMaxWidth, baseSize.y);  // Grow horizontally

                RenderRenameWidget("tilesRename", nameRect.Min, baseSize, maxSize, GrowAxis::X, app.gfx.hwnd, renameState, listing.dir.parent.pidl.get(), child.pidl, child.name, bgCol );
            }
            else DrawTextEllipsisSingleLine(dl, nameRect, child.name, textCol);
            
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


void RenderFileGrid(f32 dpi, App& app){
    auto& activeTab = app.window.GetActiveTab();

    DirListing listing = GetVisibleListing(app);
    if (!listing.PChildren) return; // for now;

    FileViewState& vs = activeTab.viewState;
    auto& ctxState = activeTab.ctxState;
    // SelectionState& selState = activeTab.selState;
    ViewMode mode = vs.viewMode;

    // Reset hover state at the beginning of the frame
    activeTab.selState.isAnyItemHovered = false; 
    
    ctxState.openMenu = false;
    ctxState.forChildren = false;   // redundant
    
    activeTab.dir.UpdateChildren(app.directory, vs);    // needs to be polled every frame, in case data is ready 

    if (mode == ViewMode::List){
        // List view lives inside its own scrolling child, and only there is the real available height (after child padding + scrollbar reservation) known. Keyboard nav has to run INSIDE that same child so its row math can never drift from what's actually drawn - that drift was the source of the diagonal-jump.
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
    //  strict hovered flag: Returns false if a popup menu is covering the mouse
    bool isViewDirectlyHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
    bool ctxMenuPopupOpen = ImGui::IsPopupOpen("ItemContextMenu");


    // Left Click on Empty Space
    // ONLY clear selection if the user clicked the actual view, NOT a popup menu!

    if (isViewDirectlyHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)){
        if (!activeTab.selState.isAnyItemHovered && !ImGui::IsAnyItemHovered() && !ctxMenuPopupOpen) {
            activeTab.selState.selectedHashes.clear();
            activeTab.ClearRenameState();
        }
    }


    if (isViewDirectlyHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)){
        if (!activeTab.selState.isAnyItemHovered && !ImGui::IsAnyItemHovered() && !ctxMenuPopupOpen) {
            activeTab.DeselectAllItems();
            ctxState.ctxMenuItems = GetBackgroundContextMenu(ctxState.ctxMenuInterface, activeTab.dir.parent.pidl.get(), app.gfx.d3dDevice.Get());
            ctxState.openMenu = true;
            ctxState.forChildren = false;
        }
    }
    

    if (ctxState.openMenu){
        if (ctxState.forChildren){
            ctxState.selectedPidls = GetSelectedItems(activeTab, *listing.PChildren);
            ctxState.ctxMenuItems = GetContextMenu(ctxState.ctxMenuInterface, activeTab.dir.parent.pidl.get(), ctxState.selectedPidls, app.gfx.hwnd, app.gfx.d3dDevice.Get());
        }
        // else, do nothing, already gotten by isRightClick

        ImGui::SetNextWindowPos(ImGui::GetMousePos());
        ImGui::OpenPopup("ItemContextMenu");
    }
    
    PushMenuTheme(dpi);
    if (ImGui::BeginPopup("ItemContextMenu")) {

        RenderContextMenuStructure(app, ctxState.ctxMenuInterface, ctxState.ctxMenuItems, activeTab.dir.parent.pidl.get(), ctxState.selectedPidls, app.gfx.hwnd, dpi);
        ImGui::EndPopup();
    }
    PopMenuTheme();
}

