#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include "imgui.h"

#include <string>
#include <algorithm>
#include <vector>

#include "WinFramework.h"
#include "ImGuiHelpers.h" 
#include "UIglobals.h" 
#include "theme.h"
// #include "iconRegular.h" 
#include "TypenameManager.h"
#include "CtxMenu.h"
#include "CtxMenuUI.h"

#include "FileView.h"
#include "FileViewHelpers.h"

/*
Grid: Horizontal flow, wrap down, icon on top, multi-line text below.
Small: Horizontal flow, wrap down, icon on left, single-line text right.
Tiles: Horizontal flow, wrap down, icon on left, 2-line title + subtitle block.
List: Vertical flow, wrap into columns to the right, dynamic per-column widths.
Details: Not even a grid—it's an ImGuiTable with multi-column header clipping and row virtualization
*/

// In views where the text rect, eg, List, Small, Details is the full chrome rect, the text rect has to be made smaller than the full rect so that in rename mode, it doesnt cover the focus ring
// Fixed by adding 2dpi px inset from the full rect, top and bottom, for small view
static void RenderGridView(f32 dpi, App& app, DirListing& listing){
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (!window || window->SkipItems) return;
    ImDrawList* dl = window->DrawList;

    const auto layout = GetFileviewLayoutForMode(ViewMode::Icons, dpi);

    auto& activeTab = app.window.GetActiveTab();
    auto& vs = activeTab.viewState;

    const int shilSize = ShiLSizeForIconSize(vs.iconSize);
    const f32 imageSize = vs.iconSize * dpi;
    const f32 yTextPadding = 4.0f * dpi;
    const f32 lineHeight = ImGui::GetFontSize();
    constexpr int maxLines = 3;
    const f32 itemWidth = imageSize * kIconsWidthMultiplier;
    const f32 framePadX = 4.0f * dpi;
    const f32 framePadY = framePadX;
    const f32 totalFramePadX = framePadX * 2;
    const f32 cellH = imageSize + yTextPadding + (maxLines * lineHeight) + yTextPadding + 2 * framePadY;

    const f32 cellStrideX = itemWidth + layout.xGap;
    const f32 cellStrideY = cellH + layout.yGap;
    

    int focusedItemIndex = ResolvePendingScrollItemIndex(vs, listing);

    GridVisibleRows grid = LayoutGrid(listing.refs.size(), cellStrideX, cellStrideY);   // Also gets and Saves screen start Pos before dummy 
    ScrollToFocusedRow(focusedItemIndex, listing.refs.size(), grid.numColumns, cellStrideY, 0);
    const ImVec2 fullVirtualContentSize (ImGui::GetContentRegionAvail().x, grid.totalRows * cellStrideY);
    ImGui::Dummy(ImVec2(fullVirtualContentSize));   // this reserves a non interactive region to keep ImGui's cursor in sync
    
    ImGui::SetCursorScreenPos(grid.screenStartPos);    // Go back to start

    const f32 totalTextHeight = maxLines * lineHeight;
    // Naturally clips, lastRow is calculated based on current scroll and viewHeight. Exclusive
    for (int row = grid.firstRow; row < grid.lastRow; row++){
        const size_t rowStartIndex = row * grid.numColumns;
        const f32 rowOffsetY = row * cellStrideY;
        for (int col = 0; col < grid.numColumns; col++){
            const size_t currentIndex = rowStartIndex + col;
            if (currentIndex >= listing.refs.size()) break;

            ImVec2 cellPos(grid.screenStartPos.x + col * cellStrideX, grid.screenStartPos.y + rowOffsetY);

            const auto child = listing.PChildren->GetItem(listing.refs[currentIndex]);
            const ImRect fullRect(cellPos, cellPos + ImVec2(itemWidth, cellH));
            DrawItemChrome(dl, window, app, dpi, listing.dir.parent, child, (int)currentIndex, fullRect, 4.0f * dpi); 

            const f32 iconX = CenterX(cellPos.x, itemWidth, imageSize);
            const f32 iconY = cellPos.y + framePadX;    // todo add 4.0f * dpi 
            DrawItemIcon(dl, app, listing.dir.parent, child, ImVec2(iconX, iconY), imageSize, shilSize);

            const f32 textX = cellPos.x + framePadX;
            const f32 textY = iconY + imageSize + yTextPadding;
            const f32 textMaxWidth = itemWidth - totalFramePadX;

            const ImRect textRect(
                ImVec2(textX, textY), 
                ImVec2(textX, textY) + ImVec2(textMaxWidth, totalTextHeight)
            );
            // const ImRect textRect(ImVec2(textX, textY), ImVec2(textX + itemWidth - framePadX, textY + totalTextHeight));
            DrawItemText(app.gfx.hwnd, dl, activeTab, listing.dir.parent, child, textRect, TextRenderMode::WrappedCentered, maxLines);
        }
    }
}


static void RenderSmallView(f32 dpi, App& app, DirListing& listing){
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (!window || window->SkipItems) return;
    ImDrawList* dl = window->DrawList;

    const auto layout = GetFileviewLayoutForMode(ViewMode::Small, dpi);

    const f32 iconPad = 6.0f * dpi;
    const f32 textGap = 8.0f * dpi;

    auto& activeTab = app.window.GetActiveTab();
    auto& vs = activeTab.viewState;

    // todo, change from fixed cellHeight to lineHeight + some padding, to accomodate font size growth
    const f32 cellStrideX = layout.cellWidth + layout.xGap; 
    const f32 cellStrideY = layout.cellHeight + layout.yGap;
    

    const int focusedItemIndex = ResolvePendingScrollItemIndex(vs, listing);
    GridVisibleRows grid = LayoutGrid(listing.refs.size(), cellStrideX, cellStrideY);
    ScrollToFocusedRow(focusedItemIndex, listing.refs.size(), grid.numColumns, cellStrideY, 0);
    const ImVec2 fullVirtualContentSize (ImGui::GetContentRegionAvail().x, grid.totalRows * cellStrideY);
    ImGui::Dummy(ImVec2(fullVirtualContentSize));

    ImGui::SetCursorPos(grid.screenStartPos);

    for (int row = grid.firstRow; row < grid.lastRow; row++){
        const size_t rowStartIndex = row * grid.numColumns;
        const f32 rowOffsetY = row * cellStrideY;

        for (int col = 0; col < grid.numColumns; col++){
            const size_t currentIndex = rowStartIndex + col;
            if (currentIndex >= listing.refs.size()) break;

            const ImVec2 cellPos = grid.screenStartPos + ImVec2(col * cellStrideX, rowOffsetY);

            const auto child = listing.PChildren->GetItem(listing.refs[currentIndex]);
            const ImRect fullRect(cellPos, cellPos + ImVec2(layout.cellWidth, layout.cellHeight));
            DrawItemChrome(dl, window, app, dpi, listing.dir.parent, child, (int)currentIndex, fullRect, 4.0f * dpi); 

            const f32 iconX =  fullRect.Min.x + iconPad;
            const f32 iconY =  cellPos.y + (layout.cellHeight - layout.iconSize) * 0.5f;
            DrawItemIcon(dl, app, listing.dir.parent, child, ImVec2(iconX, iconY), layout.iconSize, SHIL_SMALL);

            const ImRect textRect(ImVec2(iconX + layout.iconSize + textGap, fullRect.Min.y + 2.0f * dpi), ImVec2(fullRect.Max.x - textGap, fullRect.Max.y - 2.0f * dpi));

            DrawItemText(app.gfx.hwnd, dl, activeTab, listing.dir.parent, child, textRect, TextRenderMode::SingleLineEllipsis, 0);
        }
    }
}

static void RenderListViewContent(f32 dpi, App& app, DirListing& listing){
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    HandleHorizontalMouseWheelScroll(dpi);

    auto& activeTab = app.window.GetActiveTab(); 
    const auto layout = GetFileviewLayoutForMode(ViewMode::List, dpi);

    const f32 rowStride = layout.cellHeight + layout.yGap;

    const f32 minColWidth = 120.0f * dpi;
    const f32 maxColWidth = layout.cellWidth;

    const int rowsPerColumn = ComputeListRowsPerColumn(dpi, layout.yGap);
    const int totalItems = (int)listing.refs.size();
    if (totalItems == 0) return;

    const int totalColumns = ExploraCeil(totalItems, rowsPerColumn);

    const f32 basePadding = layout.iconSize + (layout.xGap * 3.0f);
    std::vector<f32> columnWidths = CalculateColumnWidthsForListView(basePadding, totalColumns, minColWidth, maxColWidth, rowsPerColumn, totalItems, listing, app);
    
    std::vector<f32> columnStarts = CalculateColumnStartsForListView(totalColumns, columnWidths);
    
    f32 windowWidth = ImGui::GetWindowWidth();
    
    const f32 totalContentWidth = columnStarts[totalColumns];
    const f32 totalContentHeight = rowsPerColumn * rowStride;  
    ImGui::Dummy(ImVec2(totalContentWidth, totalContentHeight));

    auto& vs = activeTab.viewState;
    const int focusedItemIndex = ResolvePendingScrollItemIndex(vs, listing);
    
    if (focusedItemIndex >= 0){ 
        int focusCol = focusedItemIndex / rowsPerColumn;    // vertical
        if (focusCol < totalColumns){
            f32 colLeft  = columnStarts[focusCol];
            f32 colRight = columnStarts[focusCol + 1];
            f32 colWidth = colRight - colLeft;  // includes padding
            
            ScrollToFocusedColListMode(colLeft, colWidth);
        }
    }

    const f32 scrollX = ImGui::GetScrollX();
    const auto visibleColumns = GetVisibleListColumns(scrollX, windowWidth, columnStarts, totalColumns);

    for (int column = visibleColumns.first; column < visibleColumns.last; column++){
        const f32 currentColWidth = columnWidths[column];
        const f32 currentColOffset = columnStarts[column];
        int columnStartIndex = column * rowsPerColumn;
        for (int row = 0; row < rowsPerColumn; row++){

            int currentIndex = columnStartIndex + row;
            if (currentIndex >= totalItems) break;
            auto child = listing.PChildren->GetItem(listing.refs[currentIndex], app.typeStore);

            ImGui::PushID(currentIndex);
            ImGui::SetCursorPos(ImVec2(currentColOffset, (f32)row * rowStride));
            const ImVec2 cellScreenPos = ImGui::GetCursorScreenPos();

            const ImRect fullRect(cellScreenPos, cellScreenPos + ImVec2(currentColWidth - layout.xGap, layout.cellHeight));

            DrawItemChrome(dl, window, app, dpi, listing.dir.parent, child, currentIndex, fullRect, 4.0f * dpi);

            f32 iconY = fullRect.Min.y + (layout.cellHeight - layout.iconSize) * 0.5f;

            DrawItemIcon(dl, app, listing.dir.parent, child, ImVec2(cellScreenPos.x + layout.xGap, iconY), layout.iconSize, SHIL_SMALL);
            f32 textStartX = fullRect.Min.x + layout.xGap + layout.iconSize + layout.xGap;
            f32 maxTextWidth = currentColWidth - (layout.xGap * 4.0f) - layout.iconSize;    // 3 * xGap: L edge - icon, icon - text, text - R edge. Dont know why i changed to 4, just looks better
            
            ImRect textRect(ImVec2(textStartX, fullRect.Min.y + 2.0f * dpi), ImVec2(textStartX + maxTextWidth, fullRect.Max.y - 2.0f * dpi));
           
            DrawItemText(app.gfx.hwnd, dl, activeTab, listing.dir.parent, child, textRect, TextRenderMode::SingleLineEllipsis, 0);
            ImGui::PopID();
        }
    }
}

static void RenderDetailsView(f32 dpi, App& app, DirListing& listing){
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (!window || window->SkipItems) return;

    const auto layout = GetFileviewLayoutForMode(ViewMode::Details, dpi);
    auto& activeTab = app.window.GetActiveTab();

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

        f32 headerHeight = ImGui::GetFrameHeight() + ImGui::GetStyle().CellPadding.y * 2.0f;
        // f32 headerBottomY = window->DC.CursorStartPos.y + headerHeight;
        // f32 actualViewH = ImGui::GetWindowHeight() - headerHeight;

        auto& vs = activeTab.viewState;

        int focusedItemIndex = ResolvePendingScrollItemIndex(vs, listing);
        // in details view, focused item is same as focused row
        f32 rowStride = layout.cellHeight + layout.yGap;
        ScrollToFocusedRow(focusedItemIndex, listing.refs.size(), 1, rowStride, headerHeight);

        ImDrawList* dl = ImGui::GetWindowDrawList(); 
        ImGuiWindow* currentWindow = ImGui::GetCurrentWindow();
        // Expanded so ButtonBehavior sees the whole row
        const f32 tableTopY    = currentWindow->ClipRect.Min.y;
        const f32 tableBottomY = currentWindow->ClipRect.Max.y;
    
        enum class TextRenderModeInDrawColumn {SingleLineEllipsis, RightAlignedClipped};
        auto drawColumn = [&dl, &mutedCol](const char* desiredText, const char* fallback, f32 minY, f32 maxY, TextRenderModeInDrawColumn textMode){
            ImGui::TableNextColumn();
            ImVec2 pos = ImGui::GetCursorScreenPos();
            f32 width = ImGui::GetContentRegionAvail().x;
            const char* text = (desiredText && desiredText[0]) ? desiredText : fallback;
            ImRect rect(ImVec2(pos.x, minY), ImVec2(pos.x + width, maxY));
            
            switch(textMode){
                case TextRenderModeInDrawColumn::SingleLineEllipsis:    DrawTextEllipsisSingleLine(dl, rect, text, mutedCol);
                break;
                
                case TextRenderModeInDrawColumn::RightAlignedClipped:{
                    ImGui::PushStyleColor(ImGuiCol_Text, mutedCol);
                    ImGui::RenderTextClipped(pos, ImVec2(pos.x + width, maxY), text, nullptr, nullptr, ImVec2(1.0f, 0.5f), nullptr);
                    ImGui::PopStyleColor();
                }
                break;
                
                default: assert(false);
            }
            
        };
        
        ImGuiListClipper clipper;
        clipper.Begin((int)listing.refs.size(), layout.cellHeight + layout.yGap);
        
        while (clipper.Step()){
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++){
                auto child = listing.PChildren->GetItem(listing.refs[row], app.typeStore);
                
                bool isSelected = activeTab.isSelected(child.hash);
                
                ImGui::PushID(row);
                ImGui::TableNextRow(ImGuiTableRowFlags_None, layout.cellHeight);
                ImGui::TableNextColumn();
                
                ImVec2 cellPos = ImGui::GetCursorScreenPos();
                
                f32 tableMaxX = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
                ImVec2 cellPadding = ImGui::GetStyle().CellPadding;
                
                // restrict rowRect to cell Height ignoring layout.yGap
                ImRect rowRect(
                    cellPos - cellPadding, 
                    ImVec2(tableMaxX, cellPos.y + layout.cellHeight - cellPadding.y)
                );
                
                ImGui::PushClipRect(ImVec2(rowRect.Min.x, ImMax(rowRect.Min.y, tableTopY)), ImVec2(tableMaxX, ImMin(rowRect.Max.y, tableBottomY)), false);
                
                ItemInteraction ia = DrawItemChrome(dl, window, app, dpi, listing.dir.parent, child, row, rowRect, 4.0f * dpi, true, false);
                ImGui::PopClipRect();

                ImU32 bgCol = 0;
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
                ImRect textRect(ImVec2(textX, rowRect.Min.y + 2.0f * dpi), ImVec2(textX + maxTextWidth, rowRect.Max.y - 2.0f * dpi));   

                DrawItemText(app.gfx.hwnd, dl, activeTab, listing.dir.parent, child, textRect, TextRenderMode::SingleLineEllipsis, 0);
  
                const char* dateText = (child.lastWriteTime.dwLowDateTime != 0 || child.lastWriteTime.dwHighDateTime != 0) ? FormatFileTime(child.lastWriteTime) : "";
                drawColumn(dateText, "--", rowRect.Min.y, rowRect.Max.y, TextRenderModeInDrawColumn::SingleLineEllipsis);

                drawColumn(child.typeName, "--", rowRect.Min.y, rowRect.Max.y, TextRenderModeInDrawColumn::SingleLineEllipsis);

                const char* sizeText = (child.size != 0) ? FormatFileSize(child.size) : "";
                drawColumn(sizeText, "--", rowRect.Min.y, rowRect.Max.y, TextRenderModeInDrawColumn::RightAlignedClipped);

                ImGui::PopID();

                ImGui::TableNextRow(ImGuiTableRowFlags_None, layout.yGap);
            }
        }
        ImGui::EndTable();
    }
    ImGui::PopStyleColor(5);
}

static void RenderTilesView(f32 dpi, App& app, DirListing& listing){
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (!window || window->SkipItems) return;
    ImDrawList* dl = window->DrawList;

    const auto layout = GetFileviewLayoutForMode(ViewMode::Tiles, dpi);

    const f32 lineHeight = ImGui::GetTextLineHeight();
    const ImU32 textCol = Theme::Current.palette.Text;
    const ImU32 mutedCol = Theme::Current.palette.TextMuted;

    const f32 cellStrideX = layout.cellWidth + layout.xGap;
    const f32 cellStrideY = layout.cellHeight + layout.yGap;

    auto& activeTab = app.window.GetActiveTab();
    auto& renameState = activeTab.renameState;

    FileViewState& vs = activeTab.viewState;
    int focusedItemIndex = ResolvePendingScrollItemIndex(vs, listing);


    GridVisibleRows grid = LayoutGrid(listing.refs.size(), cellStrideX, cellStrideY); 
    ScrollToFocusedRow(focusedItemIndex, listing.refs.size(), grid.numColumns, cellStrideY, 0);

    const ImVec2 fullVirtualContentSize (ImGui::GetContentRegionAvail().x, grid.totalRows * cellStrideY);
    ImGui::Dummy(ImVec2(fullVirtualContentSize));

    ImGui::SetCursorScreenPos(grid.screenStartPos);

    for (int row = grid.firstRow; row < grid.lastRow; row++){
        const size_t rowStartIndex = row * grid.numColumns;
        const f32 rowOffsetY = row * cellStrideY;
        for (int col = 0; col < grid.numColumns; col++){
            const size_t currentIndex = rowStartIndex + col;
            if (currentIndex >= listing.refs.size()) break;

            auto child = listing.PChildren->GetItem(listing.refs[currentIndex], app.typeStore);

            ImVec2 cellPos(grid.screenStartPos.x + col * cellStrideX, grid.screenStartPos.y + rowOffsetY);

            ImRect fullRect(cellPos, ImVec2(cellPos.x + layout.cellWidth, cellPos.y + layout.cellHeight));

            DrawItemChrome(dl, window, app, dpi, listing.dir.parent, child, (int)currentIndex, fullRect, 4.0f * dpi);

            f32 iconX = cellPos.x + 6.0f * dpi;
            f32 iconY = cellPos.y + (layout.cellHeight - layout.iconSize) * 0.5f;
            DrawItemIcon(dl, app, listing.dir.parent, child, ImVec2(iconX, iconY), layout.iconSize, ShilSizeForMode(ViewMode::Tiles));

            f32 textX = iconX + layout.iconSize + 8.0f * dpi;
            f32 textMaxWidth = cellPos.x + layout.cellWidth - textX - 8.0f * dpi;
                ImRect textRect(ImVec2(textX, cellPos.y + 4.0f * dpi), ImVec2(textX + textMaxWidth, cellPos.y + 4.0f * dpi + lineHeight));
                
                DrawItemText(app.gfx.hwnd, dl, activeTab, listing.dir.parent, child, textRect, TextRenderMode::SingleLineEllipsis, 0);

                const char* typeName = child.typeName[0] ? child.typeName : "--";
                f32 typeY = cellPos.y + 4.0f * dpi + lineHeight + 2.0f * dpi; 
                ImRect typeRect(
                    ImVec2(textX, typeY),
                    ImVec2(textX + textMaxWidth, typeY + lineHeight)
                );
                DrawTextEllipsisSingleLine(dl, typeRect, typeName, mutedCol);
                
        }
    }
}


void RenderFileGrid(f32 dpi, App& app){
    auto& activeTab = app.window.GetActiveTab();

    DirListing listing = GetVisibleListing(app);
    if (!listing.PChildren) return; // for now;

    FileViewState& vs = activeTab.viewState;
    auto& ctxState = activeTab.ctxState;
    auto& newState = activeTab.newState;
    auto& renameState = activeTab.renameState;
    SelectionState& selState = activeTab.selState;
    ViewMode mode = vs.viewMode;

    // Reset hover state at the beginning of the frame
    activeTab.selState.isAnyItemHovered = false; 
    
    ctxState.openMenu = false;
    ctxState.forChildren = false;   // redundant
    
    activeTab.dir.UpdateChildren(app.directory, vs);    // needs to be polled every frame, in case data is ready 

    if (newState.expectingNewItem){
        vs.scrollToItemId = newState.itemHash;
        selState.focusHash = newState.itemHash;
        activeTab.DeselectAllItemsAndSelect(newState.itemHash.value());
        
        renameState.renamingItemId = newState.itemHash;
        strncpy(activeTab.renameState.renameBuffer, newState.itemName.c_str(), sizeof(activeTab.renameState.renameBuffer) - 1);

        newState.expectingNewItem = false;
    }

    if (mode == ViewMode::List){
        ImGuiChildFlags childFlags = ImGuiChildFlags_NavFlattened;
        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_HorizontalScrollbar;
        if (ImGui::BeginChild("FileViewList", ImVec2(0, 0), childFlags, windowFlags)){
            KeyboardNavigationInteraction(dpi, app);
            RenderListViewContent(dpi, app, listing);
        }
        ImGui::EndChild();
    } else {
        KeyboardNavigationInteraction(dpi, app);
        switch (mode){
            case ViewMode::Icons:   RenderGridView(dpi, app, listing); break;
            case ViewMode::Small:   RenderSmallView(dpi, app, listing); break;
            case ViewMode::Details: RenderDetailsView(dpi, app, listing); break;
            case ViewMode::Tiles:   RenderTilesView(dpi, app, listing); break;
            default: break;
        }
    } 

    //  Add ImGuiHoveredFlags_ChildWindows to catch clicks inside BeginChild (List) and Tables (Details) 
    // bool isWindowHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByPopup);
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
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) ImGui::CloseCurrentPopup();
        RenderContextMenuStructure(app, ctxState.ctxMenuInterface, ctxState.ctxMenuItems, activeTab.dir.parent.pidl.get(), ctxState.selectedPidls, app.gfx.hwnd, dpi);
        ImGui::EndPopup();
    }
    PopMenuTheme();
}
