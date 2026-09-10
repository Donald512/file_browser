#include "FileViewLayout.h"
#include "TabStates.h"
#include "global.h"
#include "App.h"
#include "FileViewHelpers.h"



VisibleColumnRange GetVisibleListColumns( f32 scrollX, f32 windowWidth, const std::vector<f32>& columnStarts, int totalColumns) {
    int firstColumn = 0;
    while (firstColumn < totalColumns && columnStarts[firstColumn + 1] < scrollX) {
        firstColumn++;
    }

    int lastColumn = firstColumn;
    while (lastColumn < totalColumns && columnStarts[lastColumn] < scrollX + windowWidth) {
        lastColumn++;
    }

    return { firstColumn, lastColumn };
}

// Computes the number of columns based on the fullWidth and the cellWidths
int ComputeGridColumns(f32 availW, f32 cellW){
    int columns = (int)(availW / cellW);
    return ExploraMax(1, columns);
}



GridVisibleRows LayoutGrid(size_t itemCount, f32 availWidth, f32 cellW, f32 cellH){
    GridVisibleRows g{};
    if (itemCount == 0 || cellW <= 0.0f) return g;
    cellH = ExploraMax(cellH, 1.0f);
    

    g.numColumns = ComputeGridColumns(availWidth, cellW);
    g.totalRows = ExploraCeil(itemCount, g.numColumns);
    
    f32 scrollY = ImGui::GetScrollY();
    f32 viewHeight = ImGui::GetWindowHeight();

    g.firstRow = (int)ExploraMax(0, scrollY / cellH);
    g.lastRow  = (int)ExploraMin(g.totalRows, (scrollY + viewHeight) / cellH + 1);   // exclusive, so + 1
    
    g.screenStartPos = ImGui::GetCursorScreenPos();
    return g;
}


// itemExtent is the dimension of the item along that axis, eg height on Y, width on X
// ViewExtent is the height or the width of the viewport
f32 KeepRectVisible(f32 itemMin, f32 itemExtent, f32 scroll, f32 viewExtent){
    if (itemMin < scroll) return itemMin;
    else if (itemMin + itemExtent > scroll + viewExtent) return itemMin + itemExtent - viewExtent;
    else return scroll; // already in view
}


void ScrollToFocusedRow(int focusedIndex, size_t itemCount, size_t numColumns, f32 cellH, f32 reservedHeight){
    if (focusedIndex < 0 /*Was not changed, default is -1, which represents invalid*/ || focusedIndex >= (int)itemCount) return;
    int row = (int)(focusedIndex / numColumns);
    f32 itemMinY = row * cellH;
    f32 scrollY = ImGui::GetScrollY();
    f32 viewHeight = ImGui::GetWindowHeight() - reservedHeight;
    f32 newScrollY = KeepRectVisible(itemMinY, cellH, scrollY, viewHeight);
    if (newScrollY != scrollY) ImGui::SetScrollY(newScrollY);
}

void ScrollToFocusedColListMode(f32 itemMinX, f32 colWidth){
    f32 scrollX = ImGui::GetScrollX();
    f32 viewWidth = ImGui::GetWindowWidth();

    f32 newScrollX = KeepRectVisible(itemMinX, colWidth, scrollX, viewWidth);
    if (newScrollX != scrollX) ImGui::SetScrollX(newScrollX);
}

GridViewParams GetGridParamsForMode(ViewMode mode){
    switch (mode){
        case ViewMode::Small:      return {308.0f, 30.0f};
        case ViewMode::List:       return {308.0f, 30.0f};
        case ViewMode::Details:    return {308.0f, 30.0f};
        case ViewMode::Tiles:      return {250.0f, 52.0f};
    }
    return {250.0f, 52.0f};
}


// Calculates the physical distance between items in a grid, item-width + empty gap
// Horizontal stride (cell width + gap) for the wrapping-grid style views (Icons / Small / Tiles). List and Details lay out differently
f32 GetGridItemStride(ViewMode mode, f32 dpi, f32 userIconSize){
    const f32 xGap = 8.0f * dpi;
    switch (mode){
        case ViewMode::Small: case ViewMode::Tiles: return GetGridParamsForMode(mode).width * dpi + xGap;
        case ViewMode::Icons:
        default: {
            const f32 imageSize = userIconSize * dpi;
            const f32 itemWidth = imageSize * kIconsWidthMultiplier;
            return itemWidth + xGap;
        }
    }
}


int ComputeListRowsPerColumn(f32 dpi, f32 yGap, f32 availY){
    GridViewParams p = GetGridParamsForMode(ViewMode::List);
    const f32 rowStride = (p.height * dpi) + yGap;
    
    int rows = (int)(availY / rowStride);
    return ExploraMax(1, rows);
}


void CalculateColumnWidthsForListView(f32 basePadding, const int totalColumns, const f32 minColumnWidth, const f32 maxColumnWidth, const int rowsPerColumn, const int totalItems, const DirListing& listing, App& app, std::vector<f32>& result){
    
    if (totalColumns <= 0) {result.clear(); return;}

    result.assign(totalColumns, minColumnWidth);

    for (int column = 0; column < totalColumns; column++) {
        const int topOfColumn = column * rowsPerColumn;    // item at the very top of the column visually, 
        const int bottomOfColumn = ExploraMin(topOfColumn + rowsPerColumn, totalItems);     // exclusive

        for (int itemIndex = topOfColumn; itemIndex < bottomOfColumn; itemIndex++) {
            auto child = listing.PChildren->GetItem(listing.refs[itemIndex], app.typeStore);

            const f32 textWidth = ImGui::CalcTextSize(child.name).x;
            const f32 requiredWidth = textWidth + basePadding;

            if (requiredWidth >= maxColumnWidth) {
                result[column] = maxColumnWidth;
                break; // Stop checking this column immediately! Column already at max width
            }

            if (requiredWidth > result[column]) {
                result[column] = requiredWidth;
            }
        }
    }
    return;
}

void CalculateColumnStartsForListView(const int totalColumns, const std::vector<f32>& columnWidths, std::vector<f32>& result){
    // Ensure columnWidths has enough elements to read from
    // Ensure result has enough capacity for totalColumns + 1 (starts + total width right edge)
    
    assert((int)columnWidths.size() >= totalColumns && "columnWidths size smaller than totalColumns");

    result.assign(totalColumns + 1, 0.0f);   // important to set it to 0, or the garbage values will cascade
    for (int column = 0; column < totalColumns; column++) {
        result[column + 1] = result[column] + columnWidths[column];
    }
    return;
}

int ShiLSizeForIconSize(f32 iconSize){
    if (iconSize < 16) return SHIL_SMALL;
    if (iconSize < 32) return SHIL_LARGE;
    if (iconSize < 48) return SHIL_EXTRALARGE;
    return SHIL_JUMBO;
}



// delete this
int ShilSizeForMode(ViewMode mode){
    switch (mode){
        case ViewMode::Small: case ViewMode::List:  case ViewMode::Details:    return SHIL_SMALL;
        case ViewMode::Tiles: default: return SHIL_EXTRALARGE;
    }
}


FileviewLayout GetFileviewLayoutForMode(ViewMode mode, f32 dpi) {
    FileviewLayout layout;
    switch(mode) {
        case ViewMode::Icons: {
            layout.xGap = 8.0f * dpi; layout.yGap = 12.0f * dpi; layout.padX = 16.0f * dpi; layout.padY = 12.0f * dpi; 
            break;
        }
        case ViewMode::Small: {
            layout.xGap = 8.0f * dpi; layout.yGap = 4.0f * dpi; layout.padX = 16.0f * dpi; layout.padY = 8.0f * dpi; layout.cellWidth = 308.0f * dpi; layout.cellHeight = 30.0f * dpi; layout.iconSize = 16.0f * dpi;
            break;
        }
        case ViewMode::List: {
            layout.xGap = 8.0f * dpi; layout.yGap = 4.0f * dpi; layout.padX = 0.0f * dpi; layout.padY = 16.0f * dpi; layout.cellWidth = 308.0f * dpi; layout.cellHeight = 30.0f * dpi; layout.iconSize = 16.0f * dpi;
            break;
        }
        case ViewMode::Details: {
            layout.xGap = 16.0f * dpi; layout.yGap = 5.0f * dpi; layout.padX = 16.0f * dpi; layout.cellHeight = 30.0f * dpi; layout.iconSize = 16.0f * dpi;
            break;
        }
        case ViewMode::Tiles: {
            layout.xGap = 8.0f * dpi; layout.yGap = 4.0f * dpi; layout.padX = 16.0f * dpi; layout.padY = 12.0f * dpi; layout.cellWidth = 250.0f * dpi; layout.cellHeight = 52.0f * dpi; layout.iconSize = layout.cellHeight * 0.7f;
            break;
        }
        default:
        return GetFileviewLayoutForMode(ViewMode::Icons, dpi);  // exception for grid view
    }
    layout.shilSize = ShiLSizeForIconSize(layout.iconSize);
    return layout;
}
