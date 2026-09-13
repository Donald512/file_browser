#include "App.h"
#include "Tab.h"
DirListing GetVisibleListing(App& app){
    auto& activeTab = app.window.GetActiveTab();
    const Directory& dir = activeTab.dir;
    const DirChildren* PChildren = app.directory.Get(dir.HChildren);
    const std::vector<u32>& refs = dir.VisibleIndices(activeTab.viewState.showHidden);
    return {dir, PChildren, refs};
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
