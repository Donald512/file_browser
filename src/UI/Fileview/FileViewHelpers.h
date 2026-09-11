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
struct RenameState;
struct NewState;
struct SelectionState;
struct CtxMenuState;

struct DirListing {
    const Directory& dir;
    const DirChildren* PChildren = nullptr;
    const std::vector<u32>& refs;
};

static constexpr const char* kItemContextMenuID = "ItemContextMenu";

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
void ResolvePendingRenameState(RenameState& renameState);
void ResolvePendingInteractionSelState(SelectionState& selState);
void ResolveLeftMouseRelease(SelectionState& selState, RenameState& renameState,DirListing& listing);
void OnSingleClickOnOneItem(SelectionState& selState, u64 itemHash, int visualIndex);

void ExecutePendingClick(SelectionState& selState, RenameState& renameState, DirListing& listing);

void OnLeftClickOnDeadSpace(SelectionState& selState);
void OnRightClickOnDeadSpace(SelectionState& selState, CtxMenuState& ctxState, PCIDLIST_ABSOLUTE parentPidl, ID3D11Device* dev);
void ResolvePendingNewState(SelectionState& selState, NewState& newState, RenameState& renameState, FileViewState& vs, DirListing& listing);