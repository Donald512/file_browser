#include "FileViewHelpers.h"

#include "App.h"

DirListing GetVisibleListing(App& app){
    auto& activeTab = app.window.GetActiveTab();
    const Directory& dir = activeTab.dir;
    const DirChildren* PChildren = app.directory.Get(dir.HChildren);
    const std::vector<u32>& refs = dir.VisibleIndices(activeTab.viewState.showHidden);
    return {dir, PChildren, refs};
}


// todo Optimize this to move set bit to set bit, instead of all the refs
std::vector<PCITEMID_CHILD> GetSelectedItems(DirListing& listing, Tab& tab){
    std::vector<PCITEMID_CHILD> childPidls = {};
    auto& selState = tab.selState;
    if (!listing.PChildren) return childPidls;

    for (size_t bitPos : selState.selectedMask){
        // the bitPos is directly mapped to rawEntryIndex, same thing
        if (bitPos < listing.PChildren->ItemCount()){
            childPidls.reserve(selState.NumSelected());
            childPidls.push_back(listing.PChildren->GetChildPidl(bitPos));
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

// Returns visualIndex, because rawIndex can be gotten from visualIndex
size_t GetVisualIndexFromHash(DirListing& listing, u64 hash){
    for (size_t visualIndex = 0; visualIndex < listing.refs.size(); visualIndex++){
        auto rawEntryindex = listing.refs[visualIndex];

        if (listing.PChildren->hashes[rawEntryindex] == hash){
            return visualIndex;
        }
    }
    return SIZE_MAX;
}


