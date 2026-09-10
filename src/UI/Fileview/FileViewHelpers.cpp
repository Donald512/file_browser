#include "FileViewHelpers.h"

#include "App.h"

DirListing GetVisibleListing(App& app){
    auto& activeTab = app.window.GetActiveTab();
    const Directory& dir = activeTab.dir;
    const DirChildren* PChildren = app.directory.Get(dir.HChildren);
    const std::vector<u32>& refs = dir.VisibleIndices(activeTab.viewState.showHidden);
    return {dir, PChildren, refs};
}

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
        auto c = listing.PChildren->GetItem(listing.refs[i], app.typeStore);
        if (c.hash == activeTab.selState.focusHash) {
            focusedItemIndex = (int)i; 
            break; 
        }
    }
    return focusedItemIndex;
}


