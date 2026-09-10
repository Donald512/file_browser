#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS

#include <ShlObj.h>

#include <unordered_set>
#include <vector>

#include "BasicTypes.h"


struct App;
enum class ViewMode;
class Directory;
class DirChildren;
struct FileViewState;
class Tab;

struct DirListing {
    const Directory& dir;
    const DirChildren* PChildren = nullptr;
    const std::vector<u32>& refs;
};

// Change selection state to a U64 
inline bool isFileCutOnClipBoard(std::unordered_set<u64>& clipboardCutItems, u64 hashedPidl){
    return clipboardCutItems.find(hashedPidl) != clipboardCutItems.end();
}

DirListing GetVisibleListing(App& app);

int ResolvePendingScrollItemIndex(FileViewState& vs, DirListing& listing);
std::vector<PCITEMID_CHILD> GetSelectedItems(DirListing& listing, Tab& tab);
int GetScrollToItemIndex(DirListing& listing, u64 id);
int GetFocusedItemIndex(App& app);
size_t GetVisualIndexFromHash(DirListing& listing, u64 hash);


