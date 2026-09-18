#include "FileViewHelpers.h"

#include "App.h"
#include "imgui_internal.h"


std::vector<PCITEMID_CHILD> GetSelectedItems(DirListing& listing, Tab& tab){
    std::vector<PCITEMID_CHILD> childPidls = {};
    auto& selState = tab.selState;
    if (!listing.PChildren) return childPidls;
    childPidls.reserve(selState.NumSelected());

    for (size_t bitPos : selState.selectedMask){
        // the bitPos is directly mapped to rawEntryIndex, same thing
        if (bitPos < listing.PChildren->ItemCount()){
            childPidls.push_back(listing.PChildren->GetChildPidl(bitPos));
        }
    }
    return childPidls;
}

std::vector<PITEMID_CHILD> CloneSelectedItems(DirListing& listing, Tab& tab){
    std::vector<PITEMID_CHILD> childPidls = {};
    auto& selState = tab.selState;
    if (!listing.PChildren) return childPidls;
    childPidls.reserve(selState.NumSelected());

    for (size_t bitPos : selState.selectedMask){
        if (bitPos < listing.PChildren->ItemCount()){
            childPidls.push_back(ILClone(listing.PChildren->GetChildPidl(bitPos)));
        }
    }
    return childPidls;
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


int GetFocusedItemIndex(App& app){
    DirListing listing = GetVisibleListing(app);
    auto& activeTab = app.window.GetActiveTab();

    int focusedItemIndex = -1;
    for (size_t i = 0; i < listing.refs.size(); i++){
        auto c = listing.PChildren->GetItem(listing.refs[i]);
        if (c.hash == activeTab.selState.focusHash) {
            focusedItemIndex = (int)i; 
            break; 
        }
    }
    return focusedItemIndex;
}



void OnSingleClickOnOneItem(SelectionState& selState, u64 itemHash, int visualIndex){
    selState.focusHash = itemHash;
    selState.anchorHash = itemHash;
    selState.anchorVisualIndex = visualIndex;
}

void ExecutePendingClick(SelectionState& selState, RenameState& renameState, DirListing& listing) {
    if (!selState.mouseDownItemHash.has_value()) return;
    size_t itemHash = selState.mouseDownItemHash.value();
    auto visualIndex = GetVisualIndexFromHash(listing, itemHash);
    assert(visualIndex != SIZE_MAX);    // will prolly segfault below anyway
    if (visualIndex >= listing.refs.size()){selState.mouseDownItemHash = std::nullopt; return; }
    auto rawEntryIndex = listing.refs[visualIndex];
    auto child = listing.PChildren->GetItem(rawEntryIndex);
    selState.singleClickedAtTime = ImGui::GetTime();
    

    if (selState.mouseDownWasSoleSelection) renameState.pendingHash = itemHash;
    else{
        selState.DeselectAllItemsAndSelect(rawEntryIndex);
        OnSingleClickOnOneItem(selState, child.hash, (int)visualIndex);
    }
    selState.mouseDownItemHash = std::nullopt;
}


// Resolve pending -> dragging/marquee once the mouse has actually moved.
void ResolvePendingInteractionSelState(DragInfo& dragInfo, Tab& activeTab, SelectionState& selState, DirListing& listing){
    using Mode = SelectionState::InteractionMode;
    f32 dragThreshold = ImGui::GetIO().MouseDragThreshold;
    ImVec2 mousePos = ImGui::GetMousePos();
    if (selState.mode == Mode::PendingClick || selState.mode == Mode::PendingMarquee){
        f32 dist = ImLengthSqr(mousePos - selState.mouseDownPos);
        if (dist > dragThreshold * dragThreshold){
            if (selState.mode == Mode::PendingClick){
                dragInfo.parentPidl = ILClone(listing.dir.parent.pidl.get());
                dragInfo.pendingDragPidls = CloneSelectedItems(listing, activeTab);
                dragInfo.pendingInternalDrag = true;

                selState.mode = Mode::Idle;
            }
            else selState.mode = Mode::SelectingMarquee;
        }
    }
}

void ResolveLeftMouseRelease(SelectionState& selState,RenameState& renameState, DirListing& listing){
    using Mode = SelectionState::InteractionMode;   
    bool leftReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Left);
    if (leftReleased){
        if (selState.mode == Mode::PendingClick){
            selState.singleClickedAtTime = ImGui::GetTime();
            ExecutePendingClick(selState, renameState, listing);
        }
        else if (selState.mode == Mode::PendingMarquee){    // Didnt move far enough to become a marquee, so interpret as dead space
            selState.DeselectAllItems();
            renameState.Clear();
        }
        selState.mode = Mode::Idle;
    }
}

void OnLeftClickOnDeadSpace(SelectionState& selState){
    using Mode = SelectionState::InteractionMode;   
    bool leftClick = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    bool isViewDirectlyHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
    if (!(leftClick && isViewDirectlyHovered)) return;
    bool ctxMenuPopupOpen = ImGui::IsPopupOpen(kItemContextMenuID);
    ImVec2 mousePos = ImGui::GetMousePos();
    if (!selState.isAnyItemHovered && !ImGui::IsAnyItemHovered() && !ctxMenuPopupOpen){
        if (selState.mode == Mode::Idle){
            selState.mode = Mode::PendingMarquee;
            selState.mouseDownPos = mousePos;
            selState.marqueCtrlHeld = ImGui::GetIO().KeyCtrl;
            selState.marqueeBaseMask = selState.selectedMask;
        }
    }
}

void OnRightClickOnDeadSpace(SelectionState& selState, CtxMenuState& ctxState, PCIDLIST_ABSOLUTE parentPidl, ID3D11Device* dev){
    using Mode = SelectionState::InteractionMode;   
    bool rightClick = ImGui::IsMouseClicked(ImGuiMouseButton_Right);
    bool isViewDirectlyHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
    if (!(rightClick && isViewDirectlyHovered)) return;
    bool ctxMenuPopupOpen = ImGui::IsPopupOpen(kItemContextMenuID);
    ImVec2 mousePos = ImGui::GetMousePos();
    if (!selState.isAnyItemHovered && !ImGui::IsAnyItemHovered() && !ctxMenuPopupOpen){
        selState.DeselectAllItems(); 
        ctxState.ctxMenuItems = GetBackgroundContextMenu(ctxState.ctxMenuInterface, parentPidl, dev);
        ctxState.openMenu = true;
        ctxState.forChildren = false;
    }
}

void ResolvePendingNewState(NewState& newState, RenameState& renameState, FileViewState& vs, DirListing& listing){
    if (newState.expectingNewItem){
        size_t visualIndex = GetVisualIndexFromHash(listing, newState.itemHash.value());
        bool isValid = visualIndex < listing.refs.size();
        assert(isValid);
        if (isValid) StartRename(renameState, vs, listing, (int)visualIndex);
        newState.expectingNewItem = false;
    }
}

void ResolvePendingRenameState(SelectionState& selState, RenameState& renameState, FileViewState& vs, DirListing& listing){
    if (renameState.pendingHash.has_value() && ImGui::GetTime() - selState.singleClickedAtTime > ImGui::GetIO().MouseDoubleClickTime){
        auto visualIndex = GetVisualIndexFromHash(listing, *renameState.pendingHash);

        StartRename(renameState, vs, listing, (int)visualIndex);
        renameState.pendingHash = std::nullopt;
    }
}

void ExecuteItem(CommandQueue& cmdQueue, DirListing& listing, int visualIndex, size_t activeTabIndex){
    assert(visualIndex >= 0 && visualIndex < listing.refs.size());

    auto rawIndex = listing.refs[visualIndex];
    auto child = listing.PChildren->GetItem(rawIndex);
    PCIDLIST_ABSOLUTE newPidl = GetFullPidl(listing.dir.parent.pidl.get(), child.pidl);
    
    if (child.IsFolder()) cmdQueue.QueueCommand(Cmd_GoTo{activeTabIndex, WShell::Pidl(newPidl)});
    else cmdQueue.QueueCommand(Cmd_OpenFile{WShell::Pidl(newPidl)});
}

void StartRename(RenameState& renameState, FileViewState& vs, DirListing& listing, int visualIndex) {
    auto rawIndex = listing.refs[visualIndex];
    renameState.renamingItemId = listing.PChildren->hashes[rawIndex];
    vs.scrollToItemId = renameState.renamingItemId;
    strncpy(renameState.renameBuffer, listing.PChildren->GetChildName(rawIndex), sizeof(renameState.renameBuffer) - 1);
    renameState.renameBuffer[sizeof(renameState.renameBuffer) - 1] = '\0';
    renameState.selectAll = true;
    renameState.setFocus = true;
}

void UpdateMarqueSelection(SelectionState& selState, size_t rawEntryIndex, ImRect rect){
    ImVec2 mousePos = ImGui::GetMousePos();
    ImRect marqueRect(ImMin(selState.mouseDownPos, mousePos), ImMax(selState.mouseDownPos, mousePos));
    bool inRect = marqueRect.Overlaps(rect);
    bool wasInBase = selState.marqueCtrlHeld && selState.marqueeBaseMask.IsSet(rawEntryIndex); 
    if (inRect || wasInBase) selState.AddItemToSelection(rawEntryIndex);
    else selState.DeselectItem(rawEntryIndex);
}
