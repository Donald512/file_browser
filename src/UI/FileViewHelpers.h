#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include "imgui.h"
#include "ImGuiHelpers.h"

#include "WinFramework.h"

#include <string>
#include <unordered_set>
#include <algorithm>
#include <cstdio>

#include "BasicTypes.h"
#include "Tab.h"
#include "App.h"
#include "global.h"


#define ExploraMax(a, b) ((a) > (b) ? (a) : (b))
#define ExploraMin(a, b) ((a) < (b) ? (a) : (b))

#define ExploraCeil(numerator, denominator) (((numerator) + (denominator) - 1) /  (denominator))

inline f32 CenterX(f32 offsetX, f32 fullWidth, f32 usedWidth) {return offsetX + (fullWidth - usedWidth) * 0.5f;}

// Change selection state to a U64 
inline bool isFileCutOnClipBoard(std::unordered_set<u64>& clipboardCutItems, u64 hashedPidl){
    return clipboardCutItems.find(hashedPidl) != clipboardCutItems.end();
}
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

// Computes the number of columns based on the fullWidth and the cellWidths
inline int ComputeGridColumns(f32 availW, f32 cellW){
    int columns = (int)(availW / cellW);
    return ExploraMax(1, columns);
}

inline GridVisibleRows LayoutGrid(size_t itemCount, f32 availWidth, f32 cellW, f32 cellH){
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

inline void ScrollToFocusedRow(int focusedIndex, size_t itemCount, size_t numColumns, f32 cellH, f32 reservedHeight){
    if (focusedIndex < 0 /*Was not changed, default is -1, which represents invalid*/ || focusedIndex >= (int)itemCount) return;
    int row = (int)(focusedIndex / numColumns);
    f32 itemMinY = row * cellH;
    f32 scrollY = ImGui::GetScrollY();
    f32 viewHeight = ImGui::GetWindowHeight() - reservedHeight;
    f32 newScrollY = KeepRectVisible(itemMinY, cellH, scrollY, viewHeight);
    if (newScrollY != scrollY) ImGui::SetScrollY(newScrollY);
}

inline void ScrollToFocusedColListMode(f32 itemMinX, f32 colWidth){
    f32 scrollX = ImGui::GetScrollX();
    f32 viewWidth = ImGui::GetWindowWidth();

    f32 newScrollX = KeepRectVisible(itemMinX, colWidth, scrollX, viewWidth);
    if (newScrollX != scrollX) ImGui::SetScrollX(newScrollX);
}


inline GridViewParams GetGridParamsForMode(ViewMode mode){
    switch (mode){
        case ViewMode::Small:      return {308.0f, 30.0f};
        case ViewMode::List:       return {308.0f, 30.0f};
        case ViewMode::Details:    return {308.0f, 30.0f};
        case ViewMode::Tiles:      return {250.0f, 52.0f};
    }
    return {250.0f, 52.0f};
}

// delete this
inline int ShilSizeForMode(ViewMode mode){
    switch (mode){
        case ViewMode::Small: case ViewMode::List:  case ViewMode::Details:    return SHIL_SMALL;
        case ViewMode::Tiles: default: return SHIL_EXTRALARGE;
    }
}
inline int ShiLSizeForIconSize(f32 iconSize){
    if (iconSize < 16) return SHIL_SMALL;
    if (iconSize < 32) return SHIL_LARGE;
    if (iconSize < 48) return SHIL_EXTRALARGE;
    return SHIL_JUMBO;
}


constexpr f32 kIconsWidthMultiplier = 1.5f; // Icons view: cell width = icon size * this

// Calculates the physical distance between items in a grid, item-width + empty gap
// Horizontal stride (cell width + gap) for the wrapping-grid style views (Icons / Small / Tiles). List and Details lay out differently
inline f32 GetGridItemStride(ViewMode mode, f32 dpi, f32 userIconSize){
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

inline int ComputeListRowsPerColumn(f32 dpi, f32 yGap, f32 availY){
    GridViewParams p = GetGridParamsForMode(ViewMode::List);
    const f32 rowStride = (p.height * dpi) + yGap;
    const ImGuiStyle& style = ImGui::GetStyle();
    // const f32 availY = ImGui::GetWindowSize().y - (style.WindowPadding.y * 2.0f) - style.ScrollbarSize;  // wrong, ugly
    // const f32 availY = ImGui::GetContentRegionAvail().y; // correct
 
    int rows = (int)(availY / rowStride);
    return ExploraMax(1, rows);
}

struct DirListing {
    const Directory& dir;
    const DirChildren* PChildren = nullptr;
    const std::vector<u32>& refs;
};

inline DirListing GetVisibleListing(App& app){
    auto& activeTab = app.window.GetActiveTab();
    const Directory& dir = activeTab.dir;
    const DirChildren* PChildren = app.directory.Get(dir.HChildren);
    const std::vector<u32>& refs = dir.VisibleIndices(activeTab.viewState.showHidden);
    return {dir, PChildren, refs};
}

struct ItemInteraction {
    bool hovered;
    bool clicked;
};


std::vector<PCITEMID_CHILD> GetSelectedItems(Tab& tab, const DirChildren& children){
    std::vector<PCITEMID_CHILD> childPidls = {};
    auto& selSet = tab.selState.selectedHashes;

    for (auto index : tab.dir.VisibleIndices(tab.viewState.showHidden)){
        if (childPidls.size() > selSet.size()) break;   // no need to continue searching

        u64 hash = children.hashes[index];
        if (selSet.find(hash) != selSet.end()){
            childPidls.push_back(children.GetChildPidl(index));
        }
    }   
    return childPidls;
}

inline ItemInteraction HandleItemInteraction(App& app, const DirParent& parent, const ItemView& child, int visualIndex, ImGuiID id, const ImRect& rect){
    Interaction ia = MakeInteractive(id, rect);
    bool doubleClicked = IsDoubleClick(id, ia.pressed);

    // Detect Modifiers
    bool isCtrl  = ImGui::GetIO().KeyCtrl;
    bool isShift = ImGui::GetIO().KeyShift;
    bool isLeftClick  = ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ia.hovered;
    bool isRightClick = ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ia.hovered;

    auto& activeTab = app.window.GetActiveTab();

    auto& selState = activeTab.selState; 
    auto& ctxState = activeTab.ctxState;

    bool isCurrentlySelected = activeTab.isSelected(child.hash);

    // track hover state for dead space  clicking
    if (ia.hovered) {
        activeTab.selState.isAnyItemHovered = true;
    }

    auto& renameState = activeTab.renameState;
    // ============================
    // LEFT CLICK
    // ============================
    if (isLeftClick && !doubleClicked){
        // Shift-Click: Range Selection of continuous items between anchor and current
        if (isShift && selState.anchorVisualIndex != -1){

            // determine index boundaries regardless of click direction, (up or down)
            int start = ExploraMin(selState.anchorVisualIndex, visualIndex);
            int end   = ExploraMax(selState.anchorVisualIndex, visualIndex);

            if (!isCtrl){   // if ctrl isnt held, wipe the current selection first, ctrl + shift allows expanding an existing selection
                selState.selectedHashes.clear(); // Clear unless Ctrl+Shift
            }

            DirListing listing = GetVisibleListing(app);
            for (int i = start; i <= end; i++) {
                auto c = listing.PChildren->GetItem(listing.refs[i], app.typeStore);
                selState.selectedHashes.insert(c.hash);
            }

        

            selState.focusHash = child.hash;
            // Anchor does NOT change on Shift-Click, allowing further Shift-Clicks
        }
        else if (isCtrl){
            // Ctrl Click: Toggle
            if (isCurrentlySelected){
                activeTab.DeselectItem(child.hash);
            }
            else{
                activeTab.AddItemToSelection(child.hash);
            }

            selState.focusHash = child.hash;
            selState.anchorHash = child.hash;
            selState.anchorVisualIndex = visualIndex;
        }
        else{
            if (isCurrentlySelected && activeTab.selState.selectedHashes.size() == 1){ // its the only one selected
                // enter rename mode
                renameState.pendingHash = child.hash;
                renameState.singleClickedAtTime = ImGui::GetTime();
                
                strncpy(renameState.renameBuffer, child.name, sizeof(renameState.renameBuffer) - 1);
                renameState.renameBuffer[sizeof(renameState.renameBuffer) - 1] = '\0';

                selState.focusHash = child.hash;
                selState.anchorHash = child.hash;
                selState.anchorVisualIndex = visualIndex;
            }
            else{
                activeTab.DeselectAllItemsAndSelect(child.hash);
                selState.focusHash = child.hash;
                selState.anchorHash = child.hash;
                selState.anchorVisualIndex = visualIndex;
                renameState.pendingHash = std::nullopt;
            }
        }
    } 
    // ============================
    // DOUBLE CLICK
    // ============================
    if (doubleClicked) {
        renameState.pendingHash = std::nullopt; // cancel - this was a double click, not a rename trigger
        PCIDLIST_ABSOLUTE newPidl = GetFullPidl(parent.pidl.get(), child.pidl);
        // WShell::Pidl steals ownership
        if (child.IsFolder()) app.QueueCommand(Cmd_GoTo{app.window.activeTabIndex, WShell::Pidl(newPidl) });
        else{
            app.QueueCommand(Cmd_OpenFile{ WShell::Pidl(newPidl) });
            activeTab.DeselectAllItemsAndSelect(child.hash);
            selState.focusHash = child.hash;
            selState.anchorHash = child.hash;
            selState.anchorVisualIndex = visualIndex;
        }
    } 
    // ============================
    // RIGHT CLICK
    // ============================
    if (isRightClick) {
        if (!isCurrentlySelected) {
            // Right-clicked an UNSELECTED item: Clear everything else, select this one
            activeTab.DeselectAllItemsAndSelect(child.hash);
            selState.focusHash = child.hash;
            selState.anchorHash = child.hash;
            selState.anchorVisualIndex = visualIndex;
        }
        ctxState.openMenu = true;
        ctxState.forChildren = true;
    }

    // check if renameMode is active
    if (renameState.pendingHash.has_value()){
        double elapsed = ImGui::GetTime() - renameState.singleClickedAtTime;
        if (elapsed > ImGui::GetIO().MouseDoubleClickTime){
            renameState.renamingItemId = renameState.pendingHash;
            renameState.pendingHash = std::nullopt;
        }
    }

    return {ia.hovered || ia.pressed};
}


inline void RenderRenameWidget(const char* strId, ImVec2 pos, ImVec2 baseSize, ImVec2 maxSize, GrowAxis axis, HWND hwnd, RenameState& renameState, PCIDLIST_ABSOLUTE parentPidl, PCIDLIST_ABSOLUTE childPidl, const char* childName, ImU32 bgCol){
    AutoInputColors cols;
    cols.bg = ImGui::ColorConvertU32ToFloat4(bgCol);
    cols.border = {};
    cols.selectionBg = {};
    cols.text = ImGui::ColorConvertU32ToFloat4(Theme::Current.palette.Text);

    bool justOpened = (renameState.renameFocusHandledFor != renameState.renamingItemId.value());
    if (justOpened) renameState.renameFocusHandledFor = renameState.renamingItemId.value();

    InputResult res = RenderAutoResizingInputText(strId, pos, baseSize, maxSize, renameState.renameBuffer, sizeof(renameState.renameBuffer), axis, false, &cols, justOpened);
    
    if (res == InputResult::Committed){
        WShell::CommitRename(hwnd, parentPidl, {childPidl, childName}, renameState.renameBuffer);
        renameState.renamingItemId = std::nullopt;
        renameState.renameFocusHandledFor = std::nullopt;
    }
    else if (res == InputResult::Cancelled){
        renameState.renamingItemId = std::nullopt;
        renameState.renameFocusHandledFor = std::nullopt;
    }
}


ItemInteraction DrawItemChrome(ImDrawList* dl, ImGuiWindow* window, App& app, f32 dpi, const DirParent& parent, const ItemView& child, int visualIndex, const ImRect& fullRect, f32 rounding, bool drawFocusRing = true, bool drawBg = true){
    auto& activeTab = app.window.GetActiveTab();

    ImGuiID id = window->GetID((void*)(intptr_t)child.hash);
    ItemInteraction ia = HandleItemInteraction(app, parent, child, visualIndex, id, fullRect);

    bool isSelected = activeTab.isSelected(child.hash);
    bool isFocused  = activeTab.selState.focusHash == child.hash;

    if (drawBg) DrawSelectableBg(dl, fullRect, ia.hovered, isSelected, rounding);

    if (drawFocusRing && isFocused){
        // todo change it from white to palette.focused
        dl->AddRect(fullRect.Min, fullRect.Max, 0xFFFFFFFF, rounding, 1.0f * dpi);
    }
    return ia;
}


void DrawItemIcon(ImDrawList* dl, App& app, const DirParent& parent, const ItemView& child, ImVec2 pos, f32 iconSize, int shilSize){
    u32 iconIndex = app.icons.GetIconIndex(parent.pidl.get(), child.pidl, child.hash);
    
    auto iconFallback = [&](){
        const char* fallback = child.IsFolder() ? ICON_REG_FOLDER : ICON_REG_DOCUMENT;
        DrawTextCenteredSingleLine(dl, pos, ImVec2(pos.x + iconSize, pos.y + iconSize), fallback, Theme::Current.palette.TextMuted, iconSize);
    };

    if (iconIndex == UINT32_MAX){
        iconFallback();
        return;
    }

    ImTextureID iconTex = app.textures.GetTexture({iconIndex, shilSize});
    if (!iconTex){
        iconFallback();
        return;
    }

    bool isCut =  isFileCutOnClipBoard(app.clipBoardCutItems, child.hash);

    ImU32 tint = (child.IsHidden() || isCut) ? IM_COL32(255, 255, 255, 128) : IM_COL32(255, 255, 255, 255);

    dl->AddImage(iconTex, pos, ImVec2(pos.x + iconSize, pos.y + iconSize), ImVec2(0, 0), ImVec2(1, 1), tint);
}

enum class TextRenderMode {WrappedCentered, SingleLineEllipsis};

inline void DrawItemText(HWND hwnd, ImDrawList* dl, Tab& activeTab, const DirParent& parent, const ItemView& child, const ImRect& textRect, TextRenderMode textRenderMode, int maxLines){
    auto& renameState = activeTab.renameState;
    if (renameState.renamingItemId == child.hash) {
        bool isSelected = activeTab.isSelected(child.hash);
        ImU32 bgCol = isSelected ? Theme::Current.palette.SurfaceActive : Theme::Current.palette.Surface;
 
        const f32 textMaxWidth = textRect.GetWidth();
        ImVec2 baseSize = textRect.GetSize();
        
        RenderRenameWidget("iconsRename", textRect.Min, baseSize, baseSize, GrowAxis::X, hwnd, renameState, parent.pidl.get(), child.pidl, child.name, bgCol);
        return; 
    }

    switch(textRenderMode){
        case TextRenderMode::WrappedCentered:       RenderTextWrappedCenteredEllipsis(dl, textRect, child.name, nullptr, maxLines);
        break;
        case TextRenderMode::SingleLineEllipsis:    DrawTextEllipsisSingleLine(dl, textRect, child.name, Theme::Current.palette.Text);
        break;
        default: assert(false);
    }
}

int GetFocusedItemIndex(App& app){
    DirListing listing = GetVisibleListing(app);
    auto& activeTab = app.window.GetActiveTab();

    int focusedItemIndex = -1;
    for (size_t i = 0; i < listing.refs.size(); i++){
        auto c = listing.PChildren->GetItem(listing.refs[i], app.typeStore);
        if (c.hash == activeTab.selState.focusHash) {
            focusedItemIndex = (int)i; 
            break; 
        }
    }
    return focusedItemIndex;
}

int GetScrollToItemIndex(DirListing& listing, u64 id){
    int itemIndex = -1;
    for (size_t i = 0; i < listing.refs.size(); i++){
        auto hash = listing.PChildren->hashes[listing.refs[i]];
        if (hash == id) {
            itemIndex = (int)i; 
            break; 
        }
    }
    return itemIndex;
}

int ResolvePendingScrollItemIndex(FileViewState& vs, DirListing& listing){
    int focusedItemIndex = -1;
    if (vs.scrollToItemId.has_value()){
        focusedItemIndex = GetScrollToItemIndex(listing, vs.scrollToItemId.value());
        vs.scrollToItemId = std::nullopt;
    }
    return focusedItemIndex;
}



inline void CalculateColumnWidthsForListView(f32 basePadding, const int totalColumns, const f32 minColumnWidth, const f32 maxColumnWidth, const int rowsPerColumn, const int totalItems, const DirListing& listing, App& app, std::vector<f32>& result){
    
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

inline void CalculateColumnStartsForListView(const int totalColumns, const std::vector<f32>& columnWidths, std::vector<f32>& result){
    // Ensure columnWidths has enough elements to read from
    // Ensure result has enough capacity for totalColumns + 1 (starts + total width right edge)
    
    assert((int)columnWidths.size() >= totalColumns && "columnWidths size smaller than totalColumns");

    result.assign(totalColumns + 1, 0.0f);   // important to set it to 0, or the garbage values will cascade
    for (int column = 0; column < totalColumns; column++) {
        result[column + 1] = result[column] + columnWidths[column];
    }
    return;
}

inline void DEBUGPrintFocusedItems(App& app){
    auto& activeTab = app.window.GetActiveTab();
    // FileViewState& vs = activeTab.viewState;
    // ViewMode mode = vs.viewMode;
    SelectionState& selState = activeTab.selState;
    
    DirListing listing = GetVisibleListing(app);
    int totalItems = (int)listing.refs.size();
    
    for (int i = 0; i < totalItems; i++){
        auto c = listing.PChildren->GetItem(listing.refs[i], app.typeStore);
        if (selState.focusHash == c.hash){
            std::cout << "Item name: " << c.name << " Item hash: " << c.hash << " Item visualIndex = " << i << std::endl;
        }
    }
}


// for list clipper
struct VisibleColumnRange {
    int first;
    int last;
};

inline VisibleColumnRange GetVisibleListColumns( f32 scrollX, f32 windowWidth, const std::vector<f32>& columnStarts, int totalColumns) {
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

void ClearFocusState(SelectionState& selState){
    selState.focusHash = std::nullopt;
    selState.anchorVisualIndex = -1;
    selState.anchorHash = std::nullopt;
}


inline void KeyboardNavigationInteraction(f32 dpi, App& app){
    auto& activeTab = app.window.GetActiveTab();
    FileViewState& vs = activeTab.viewState;
    auto& renameState = activeTab.renameState;
    ViewMode mode = vs.viewMode;
    SelectionState& selState = activeTab.selState;

    DirListing listing = GetVisibleListing(app);

    int totalItems = (int)listing.refs.size();

    // Early exit and clean reset if empty
    if (totalItems == 0) {
        ClearFocusState(selState);
        return;
    }

    // Find current focus index strictly by hash
    int focusedItemIndex = -1;
    if (activeTab.selState.focusHash.has_value()){
        focusedItemIndex = GetFocusedItemIndex(app);
    }
        
    // If focus is lost, invalid, or 0, reset it
    if (focusedItemIndex == -1) {
        ClearFocusState(selState);
    }

    bool shift = ImGui::GetIO().KeyShift;
    bool ctrl = ImGui::GetIO().KeyCtrl;
    int newFocusIdx = focusedItemIndex;
    bool navOccurred = false;


    int columns = 1;
    int rowsPerColumn = 1;
    f32 availW = ImGui::GetContentRegionAvail().x;

    if (mode == ViewMode::List){
        FileviewLayout layout = GetFileviewLayoutForMode(ViewMode::List, dpi);
        f32 availHeight = ImGui::GetContentRegionAvail().y - layout.padY * 2;
        rowsPerColumn = ComputeListRowsPerColumn(dpi, layout.yGap, availHeight);

    } 
    else if (mode == ViewMode::Details) columns = 1;
    else {
        f32 itemStride = GetGridItemStride(mode, dpi, vs.iconSize);
        if (itemStride > 0.0f) columns = ComputeGridColumns(availW, itemStride);
    }
    if (renameState.renamingItemId.has_value()) return; 

    if (ImGui::IsKeyPressed(ImGuiKey_F2)){
        if (focusedItemIndex >= 0 && selState.selectedHashes.size() == 1){
            auto actualItemIndex = listing.refs[focusedItemIndex];

            renameState.renamingItemId = listing.PChildren->hashes[actualItemIndex];
            vs.scrollToItemId = listing.PChildren->hashes[actualItemIndex];

            strncpy(renameState.renameBuffer, listing.PChildren->GetChildName(actualItemIndex), sizeof(renameState.renameBuffer) - 1);
            renameState.renameBuffer[sizeof(renameState.renameBuffer) - 1] = '\0';  

            return;
        }
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Escape) && !ImGui::IsPopupOpen("ItemContextMenu")) {
        activeTab.DeselectAllItems();
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

            newFocusIdx = std::clamp(newFocusIdx, 0, totalItems - 1);
            auto newChild = listing.PChildren->GetItem(listing.refs[newFocusIdx], app.typeStore);
             
            if (shift) {
                int start = (std::min)(selState.anchorVisualIndex, newFocusIdx);
                int end = (std::max)(selState.anchorVisualIndex, newFocusIdx);
                if (!ctrl) activeTab.DeselectAllItems();
                for (int i = start; i <= end; i++) {
                    auto c = listing.PChildren->GetItem(listing.refs[i], app.typeStore);
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
            vs.scrollToItemId = newChild.hash;
        }

        // Spacebar: Toggle selection of focused item
        if (focusedItemIndex >= 0){
            if (ImGui::IsKeyPressed(ImGuiKey_Space)) {
                auto focusChild = listing.PChildren->GetItem(listing.refs[focusedItemIndex], app.typeStore);
                if (activeTab.isSelected(focusChild.hash)){
                    activeTab.DeselectItem(focusChild.hash);
                } else {
                    activeTab.AddItemToSelection(focusChild.hash);
                }
            }
            
            // Enter: Open / Navigate
            if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) {
                auto focusChild = listing.PChildren->GetItem(listing.refs[focusedItemIndex], app.typeStore);
                PCIDLIST_ABSOLUTE newPidl = GetFullPidl(listing.dir.parent.pidl.get(), focusChild.pidl);
                if (focusChild.IsFolder()){
                    app.QueueCommand(Cmd_GoTo{app.window.activeTabIndex, WShell::Pidl(newPidl)}); 
                } else {
                    app.QueueCommand(Cmd_OpenFile{WShell::Pidl(newPidl)});
                }
            }
        }
    }
}

