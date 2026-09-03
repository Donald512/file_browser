#pragma once

#include "BasicTypes.h"
#include <unordered_set>
#include <optional>
#include <vector>
#include "CtxMenu.h"
#include <ShlObj.h>


struct SelectionState {
    // bool justNavigated = false;
    std::unordered_set<u64> selectedHashes;
    std::optional<u64> focusHash = std::nullopt;
    std::optional<u64> anchorHash = std::nullopt;
    int anchorVisualIndex = -1; // for Shift-Click range calculations
    bool isAnyItemHovered = false;
};

struct CtxMenuState{
    bool openMenu = false;
    bool forChildren = false;

    std::vector<ContextMenuItem> ctxMenuItems;
    ComPtr<IContextMenu> ctxMenuInterface;
    std::vector<PCITEMID_CHILD> selectedPidls;  // empty if the menu is for background
};


enum class SortMode { Name, DateModified, Type, Size};
enum class SortDirection {Ascending, Descending };
enum class ViewMode { Icons, Small, List, Details, Tiles}; // feel like this belongs to  UI


struct FileViewState {
    // Change to user's last choice, or a setttings
    ViewMode viewMode = ViewMode::Details;
    f32 iconSize = 104.0f;
    SortMode sortMode = SortMode::Name;
    SortDirection sortDir = SortDirection::Ascending;
    bool showHidden = false;
    f32 gridIconSize = 64.0f; 
    
    
    // UI Directives
    std::optional<u64> scrollToItemId = std::nullopt;
    float scrollY = 0.0f;
    
};

struct RenameState{
    std::optional<u64> renamingItemId = std::nullopt;
    std::optional<u64> renameFocusHandledFor = std::nullopt;   // which item's initial focus we've already applied
    char renameBuffer[512] = {0};

    std::optional<u64> pendingHash = std::nullopt;
    double singleClickedAtTime = 0.0f;
};

struct NewState{
    bool expectingNewItem = false;
    std::optional<u64> itemHash = std::nullopt;
    bool hasScrolledToNewItem = false;
    std::string itemName;
};