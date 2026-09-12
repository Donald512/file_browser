#pragma once

#include "BasicTypes.h"
#include <unordered_set>
#include <optional>
#include <ShlObj.h>
#include <vector>

#include "CtxMenu.h"
#include "Bitmask.h"

struct SelectionState {
    // todo, move this to an InteractionState
    enum class InteractionMode {
        Idle,
        PendingClick,      // Mouse went down on an item, waiting for release or drag threshold
        PendingMarquee,    // Mouse went down on empty space, waiting for release or drag threshold
        DraggingItems,     // User is actively dragging selected files
        SelectingMarquee   // User is actively drawing a selection box
    };

    // bool justNavigated = false;
    Bitmask selectedMask;
    std::optional<u64> focusHash = std::nullopt;
    std::optional<u64> anchorHash = std::nullopt;
    int anchorVisualIndex = -1; // for Shift-Click range calculations
    bool isAnyItemHovered = false;

    InteractionMode mode = InteractionMode::Idle;
    ImVec2 mouseDownPos = ImVec2(0, 0);
    std::optional<u64> mouseDownItemHash = std::nullopt;
    double singleClickedAtTime = 0.0f;
    std::optional<u64> mouseDownVisualIndex = std::nullopt;
    bool mouseDownWasSoleSelection = false;  // captured at press time, for click vs rename
    bool marqueCtrlHeld = false;    // was ctrl held when the marque drag started
    Bitmask marqueeBaseMask;    // snapshot of selection at marque start

    bool mouseDownItemWasSelected = false; // was the pressed item selected *at all* when clicked (not just "sole")
    std::vector<PITEMID_CHILD> dragPidls;       // snapshot of items being dragged — match whatever type ctxState.selectedPidls actually is
    std::optional<u64> dragHoverTargetHash; // folder currently hovered as a drop target, if any


    size_t NumSelected(){ return selectedMask.SetBitCount();}
    bool IsSelected(size_t rawEntryIndex){return selectedMask.IsSet(rawEntryIndex);}
    void AddItemToSelection(size_t rawEntryIndex){selectedMask.Set(rawEntryIndex);}
    void ToggleItemSelection(size_t rawEntryIndex){selectedMask.Toggle(rawEntryIndex);}
    void DeselectItem(size_t rawEntryIndex){selectedMask.Unset(rawEntryIndex);}
    void DeselectAllItems(){selectedMask.Clear();}
    
    void DeselectAllItemsAndSelect(size_t rawEntryIndex){    
        selectedMask.Clear();
        selectedMask.Set(rawEntryIndex);
    }

    void Clear(){
        selectedMask.Clear();
        focusHash = std::nullopt;
        anchorHash = std::nullopt;
        anchorVisualIndex = -1;
        isAnyItemHovered = false;
    }
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

    std::vector<f32> columnStarts;  // for List mode, to prevent allocing and freeing at 144Hz
    std::vector<f32> columnWidths;

    void Clear(){
        scrollToItemId = std::nullopt;
        columnStarts.clear();
        columnWidths.clear();
        scrollY = 0.0f;
    }
};

struct RenameState{
    std::optional<u64> renamingItemId = std::nullopt;
    std::optional<u64> renameFocusHandledFor = std::nullopt;   // which item's initial focus we've already applied
    char renameBuffer[512] = {0};

    std::optional<u64> pendingHash = std::nullopt;
    double singleClickedAtTime = 0.0f;

    void Clear(){
        renamingItemId = std::nullopt;
        renameFocusHandledFor = std::nullopt;
        pendingHash = std::nullopt;

        renameBuffer[0] = 0;
    }
};

struct NewState{
    bool expectingNewItem = false;
    std::optional<u64> itemHash = std::nullopt;
    bool hasScrolledToNewItem = false;
    std::string itemName;
};