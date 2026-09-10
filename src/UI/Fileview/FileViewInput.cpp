#pragma once

#include "TabStates.h"

#include "FileViewInput.h"

#include "ImGuiHelpers.h"
#include "global.h"
#include "DirectoryManager.h"
#include "FileViewHelpers.h"
#include "FileViewLayout.h"
#include "App.h"


ItemInteraction HandleItemInteraction(App& app, const DirParent& parent, const ItemView& child, int visualIndex, ImGuiID id, const ImRect& rect){
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


void KeyboardNavigationInteraction(f32 dpi, App& app){
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


void ClearFocusState(SelectionState& selState){
    selState.focusHash = std::nullopt;
    selState.anchorVisualIndex = -1;
    selState.anchorHash = std::nullopt;
}

