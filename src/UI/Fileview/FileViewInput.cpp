#pragma once


#include "TabStates.h"
#include "FileViewInput.h"
#include "ImGuiHelpers.h"
#include "global.h"
#include "DirectoryManager.h"
#include "FileViewHelpers.h"
#include "FileViewLayout.h"
#include "App.h"


/*
Idle
 ├─ mouse down on an item, no modifier ──▶ PendingClick
 │      ├─ released without crossing drag threshold ─▶ resolve as a click (select / arm rename)
 │      └─ moved past drag threshold ─────────────────▶ DraggingItems
 │
 └─ mouse down on empty space ──────────▶ PendingMarquee
        ├─ released without crossing drag threshold ─▶ resolve as "clear selection"
        └─ moved past drag threshold ─────────────────▶ SelectingMarquee
Shift+click and ctrl-click never enter this ambigous state, so they are immediate 
*/


// Must be activeTab, if that turns out to be false, change the parameter to size_t tabIndex, because the .activeTabIndex is passed to command queue
ItemInteraction HandleItemInteraction(CommandQueue& cmdQueue, Tab& activeTab, size_t activeTabIndex, DirListing& listing, int visualIndex, ImGuiID id, const ImRect& rect){
    auto& selState = activeTab.selState;   
    auto& renameState = activeTab.renameState;
    Interaction ia = MakeInteractive(id, rect);
    auto rawEntryIndex = listing.refs[visualIndex];
    auto child = listing.PChildren->GetItem(rawEntryIndex);
    
    if (ia.hovered) activeTab.selState.isAnyItemHovered = true; // for dead space  clicking

    if (selState.mode == SelectionState::InteractionMode::SelectingMarquee){
        ImVec2 mousePos = ImGui::GetMousePos();
        ImRect marqueRect(ImMin(selState.mouseDownPos, mousePos), ImMax(selState.mouseDownPos, mousePos));
        bool inRect = marqueRect.Overlaps(rect);
        bool wasInBase = selState.marqueCtrlHeld && selState.marqueeBaseMask.IsSet(rawEntryIndex); 
        if (inRect || wasInBase) selState.AddItemToSelection(rawEntryIndex);
        else selState.DeselectItem(rawEntryIndex);
        return {ia.hovered || ia.pressed};   // nothing else should happen when mid marque

    }
        
    bool isCtrl  = ImGui::GetIO().KeyCtrl;
    bool isShift = ImGui::GetIO().KeyShift;
    bool isCurrentlySelected = selState.IsSelected(rawEntryIndex);
    bool doubleClicked = IsDoubleClick(id, ia.pressed);

    if (ia.pressed && !doubleClicked && selState.mode == SelectionState::InteractionMode::Idle){
        renameState.pendingHash = std::nullopt; // any fresh press cancels a stale rename-arm

        if (isShift && selState.anchorVisualIndex != -1){
            int start = ExploraMin(selState.anchorVisualIndex, visualIndex);
            int end   = ExploraMax(selState.anchorVisualIndex, visualIndex);
            if (!isCtrl) selState.selectedMask.Clear();
            for (int i = start; i <= end; i++) selState.AddItemToSelection(listing.refs[i]);
            selState.focusHash = child.hash;
        }
        else if (isCtrl){
            if (isCurrentlySelected) selState.DeselectItem(rawEntryIndex);
            else selState.AddItemToSelection(rawEntryIndex);
            OnSingleClickOnOneItem(selState, child.hash, visualIndex);
        }
        else{
            selState.mode = SelectionState::InteractionMode::PendingClick;
            selState.mouseDownPos = ImGui::GetMousePos();
            selState.mouseDownItemHash = child.hash;
            selState.mouseDownVisualIndex = visualIndex;
            selState.mouseDownWasSoleSelection = isCurrentlySelected && selState.NumSelected() == 1;
            selState.singleClickedAtTime = ImGui::GetTime();
        }
    }

    if (doubleClicked){
        selState.mode = SelectionState::InteractionMode::Idle;
        renameState.pendingHash = std::nullopt;
        ExecuteItem(cmdQueue, listing, visualIndex, activeTabIndex);
        if (!child.IsFolder()){
            selState.DeselectAllItemsAndSelect(rawEntryIndex);
            OnSingleClickOnOneItem(selState, child.hash, visualIndex);
        }
        return {ia.hovered || ia.pressed};
    }

    bool isRightClick = ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ia.hovered;
    auto& ctxState = activeTab.ctxState;
    if (isRightClick){
        if (!isCurrentlySelected){
            selState.DeselectAllItemsAndSelect(rawEntryIndex);
            OnSingleClickOnOneItem(selState, child.hash, visualIndex);
        }
        ctxState.openMenu = true;
        ctxState.forChildren = true;
        return{ia.hovered || ia.pressed};
    }
    return{ia.hovered || ia.pressed};
}

void ProcessKeyboardInput(f32 dpi, CommandQueue& cmdQueue, DirListing& listing, Tab& activeTab, size_t activeTabIndex){
    SelectionState& selState = activeTab.selState;
    RenameState& renameState = activeTab.renameState;
    FileViewState& vs = activeTab.viewState;
    ViewMode& mode = vs.viewMode;
    if (listing.refs.empty()) {ClearFocusState(selState); return;}
    
    // === Global actions (Independent of focus item) ---
    if (ImGui::IsKeyPressed(ImGuiKey_Escape) && !ImGui::IsPopupOpen(kItemContextMenuID)){ selState.DeselectAllItems(); return;}

    // === Require a Focused Item ---
    int focusedVisualIndex = selState.focusHash.has_value() ? GetVisualIndexFromHash(listing, selState.focusHash.value()) : -1;

    if (focusedVisualIndex >= 0){
        if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) {
            ExecuteItem(cmdQueue, listing, focusedVisualIndex, activeTabIndex); return;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Space)){ selState.ToggleItemSelection(listing.refs[focusedVisualIndex]); return;}

        if (ImGui::IsKeyPressed(ImGuiKey_F2) && selState.NumSelected() == 1) {
            StartRename(renameState, vs, listing, focusedVisualIndex);
            return;
        }
    }

    // ===  NAVIGATION (Arrow keys, Home, End, etc.) ---
    if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_ChildWindows)) return;
    if (renameState.renamingItemId.has_value()) return; // Block nav while renaming

    int pendingFocusedVisualIndex = focusedVisualIndex;
    bool navOccurred = false;
    
    int columns = 1;
    int rowsPerColumn = 1;
    f32 availW = ImGui::GetContentRegionAvail().x;
    FileviewLayout layout = GetFileviewLayoutForMode(ViewMode::List, dpi);

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

    auto updatePendingFocusIdx = [&](int delta, bool isDelta = true){
        if (isDelta) pendingFocusedVisualIndex += delta; 
        else pendingFocusedVisualIndex = delta;
        navOccurred = true;
    };
    if (mode == ViewMode::List) {
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) updatePendingFocusIdx(1);
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) updatePendingFocusIdx(-1);
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) updatePendingFocusIdx(rowsPerColumn);
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) updatePendingFocusIdx(-rowsPerColumn);
    }
    else if (mode == ViewMode::Details) {
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) updatePendingFocusIdx(1);
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) updatePendingFocusIdx(-1);
    }
    else{
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) updatePendingFocusIdx(columns);
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) updatePendingFocusIdx(-columns);
        if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) updatePendingFocusIdx(1);
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) updatePendingFocusIdx(-1); 
    }
    size_t totalItems = listing.refs.size();
    if (ImGui::IsKeyPressed(ImGuiKey_Home)) updatePendingFocusIdx(0, false);
    if (ImGui::IsKeyPressed(ImGuiKey_End)) updatePendingFocusIdx(totalItems - 1, false);
    
    int pageStride = (mode == ViewMode::List) ? rowsPerColumn : columns;
    if (ImGui::IsKeyPressed(ImGuiKey_PageDown)) updatePendingFocusIdx(pageStride * 10);
    if (ImGui::IsKeyPressed(ImGuiKey_PageUp)) updatePendingFocusIdx(-pageStride * 10);

    if (navOccurred){
        bool shift = ImGui::GetIO().KeyShift;
        bool ctrl = ImGui::GetIO().KeyCtrl;
        focusedVisualIndex = ImClamp(pendingFocusedVisualIndex, 0, (int)totalItems - 1);
        
        u64 newFocusChildHash = listing.PChildren->hashes[focusedVisualIndex];
        if (shift){
            // prevent i starting at 0
            if (selState.anchorVisualIndex < 0) selState.anchorVisualIndex = focusedVisualIndex;
            int rangeStart = ExploraMin(selState.anchorVisualIndex, focusedVisualIndex);
            int rangeEnd   = ExploraMax(selState.anchorVisualIndex, focusedVisualIndex);
            if (!ctrl)  selState.DeselectAllItems(); // Replace entire selection with current range
            for (int i = rangeStart; i <= rangeEnd; i++) selState.AddItemToSelection(listing.refs[i]);
        }
        else if (!ctrl){
            selState.DeselectAllItemsAndSelect(listing.refs[focusedVisualIndex]);
            selState.anchorVisualIndex = focusedVisualIndex;
            selState.anchorHash = newFocusChildHash;    
        }
        selState.focusHash = newFocusChildHash;
        vs.scrollToItemId = newFocusChildHash;
    }
}


void ClearFocusState(SelectionState& selState){
    selState.focusHash = std::nullopt;
    selState.anchorVisualIndex = -1;
    selState.anchorHash = std::nullopt;
}

