#pragma once

#include "imgui.h"
#include "BasicTypes.h"
#include <vector>

enum class ViewMode;
struct App;
struct DirListing;

struct GridVisibleRows{
    size_t numColumns;
    size_t totalRows;
    int firstRow;   // inclusive
    int lastRow;    // exclusive
    ImVec2 screenStartPos;
};

struct GridViewParams {
    f32 width;
    f32 height;
};

struct FileviewLayout {
    f32 xGap       = 0.0f;
    f32 yGap       = 0.0f;
    f32 padX       = 0.0f;
    f32 padY       = 0.0f;
    f32 iconSize   = 0.0f;
    f32 cellWidth  = 0.0f;
    f32 cellHeight = 0.0f;
    int shilSize;
};


// for list clipper
struct VisibleColumnRange {
    int first;
    int last;
};

constexpr f32 kIconsWidthMultiplier = 1.5f; // Icons view: cell width = icon size * this
 

VisibleColumnRange GetVisibleListColumns( f32 scrollX, f32 windowWidth, const std::vector<f32>& columnStarts, int totalColumns);

int ComputeGridColumns(f32 availW, f32 cellW);

int ShiLSizeForIconSize(f32 iconSize);

FileviewLayout GetFileviewLayoutForMode(ViewMode mode, f32 dpi);

GridVisibleRows LayoutGrid(size_t itemCount, f32 availWidth, f32 cellW, f32 cellH);

f32 KeepRectVisible(f32 itemMin, f32 itemExtent, f32 scroll, f32 viewExtent);

int ShilSizeForMode(ViewMode mode);

void ScrollToFocusedRow(int focusedIndex, size_t itemCount, size_t numColumns, f32 cellH, f32 reservedHeight);

void ScrollToFocusedColListMode(f32 itemMinX, f32 colWidth);

GridViewParams GetGridParamsForMode(ViewMode mode);


// todo check if i use this function
f32 GetGridItemStride(ViewMode mode, f32 dpi, f32 userIconSize);

int ComputeListRowsPerColumn(f32 dpi, f32 yGap, f32 availY);

void CalculateColumnWidthsForListView(f32 basePadding, const int totalColumns, const f32 minColumnWidth, const f32 maxColumnWidth, const int rowsPerColumn, const int totalItems, const DirListing& listing, App& app, std::vector<f32>& result);


void CalculateColumnStartsForListView(const int totalColumns, const std::vector<f32>& columnWidths, std::vector<f32>& result);