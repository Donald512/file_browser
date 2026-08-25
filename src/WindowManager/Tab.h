#pragma once
#include "BasicTypes.h"
#include "History.h"
#include "Breadcrumbs.h"
#include "Item.h"
#include <unordered_set>
#include "Enum.h"
#include "Directory.h"
#include "TypenameManager.h"
#include <optional>
#include "KnownSpecialFolders.h"
#include "Breadcrumbs.h"



template <typename T>
inline T ExploraClamp(T value, T minValue, T maxValue){
    if (value < minValue) return minValue;
    else if (value > maxValue) return maxValue;
    return value;
}

// maybe this for multiple different windows
struct WindowManager{};

enum class Actions {Normal, Back, Forward, Refresh};
enum class ViewMode { Icons, Small, List, Details, Tiles}; // feel like this belongs to  UI
enum class SelectMode {OneItem};

struct FileViewState {
    // Change to user's last choice, or a setttings
    ViewMode viewMode = ViewMode::Details;
    f32 iconSize = 104.0f;
    SortMode sortMode = SortMode::Name;
    SortDirection sortDir = SortDirection::Ascending;
    bool showHidden = false;

    
    // UI Directives (The "Magic" variables)
    std::optional<u64> scrollToItemId = std::nullopt;
    std::optional<u64> renamingItemId = std::nullopt;
    float scrollY = 0.0f;
    
    f32 gridIconSize = 64.0f; 
};

struct SelectionState {
    bool justNavigated = false;
    std::unordered_set<u64> selectedHashes;
    std::optional<u64> focusHash = std::nullopt;
    std::optional<u64> anchorHash = std::nullopt;
    int anchorVisualIndex = -1; // for Shift-Click range calculations
    bool isAnyItemHovered = false;
    double lastKeyboardNavTime = 0.0;
};


class Tab{
    public:
        bool GoTo(PCIDLIST_ABSOLUTE dest, Actions action = Actions::Normal);
        
        bool CanGoBack() const {return history.CanGoBack();}
        bool CanGoForward() const {return history.CanGoForward();}
        bool CanGoParent() const {
            if (!dir.parent.pidl || ILIsEmpty(dir.parent.pidl.get())) return false;
            return true;
        }
        bool GoBack(){
            if (!history.Back()) return false;
            return GoTo(history.Current(), Actions::Back);
        }
        bool GoForward(){
            if (!history.Forward()) return false;
            return GoTo(history.Current(), Actions::Forward);      
        }
        bool GoParent();
        bool Refresh(){
            return GoTo(history.Current(), Actions::Refresh);
        }

        Tab(PCIDLIST_ABSOLUTE startFolder){
            GoTo(startFolder);
        }

        // void SelectItem(u64 i, SelectMode mode);
        void DeselectAllItemsAndSelect(u64 i);
        void DeselectItem(u64 i);
        void AddItemToSelection(u64 i);
        void DeselectAllItems();
        
        bool isSelected(u64 i) const{
            return selState.selectedHashes.find(i) != selState.selectedHashes.end();
        }

        void ReSort(TypenameStore& typeStore);
        void ToggleShowHidden(){
            viewState.showHidden = !viewState.showHidden;
            if (viewState.showHidden == false){
                dir.RebuildNonHiddenIndices();
            }
        }

        Breadcrumbs breadcrumbs{};
        History history{};
        Directory dir{};
        
        FileViewState viewState;
        SelectionState selState;

};

// Maybe Window manager handles multiple windows, or maybe its not neccessary
struct Window{
    std::vector<Tab> tabs{};
    size_t activeTabIndex = 0;  // or maybe vector for tile windows
    
    void NewTab(PCIDLIST_ABSOLUTE startFolder = SpecialFolders::defaultStartupFolder){
        if (!startFolder) startFolder = SpecialFolders::defaultStartupFolder;
        tabs.emplace_back(startFolder);
        activeTabIndex = tabs.size() - 1;
    }

    void CloseTab(size_t tabIndex){
        tabs.erase(tabs.begin() + tabIndex);
        activeTabIndex = ExploraClamp(activeTabIndex, (size_t) 0, tabs.size() - 1);
    }

    Tab& GetActiveTab(){
        return tabs[activeTabIndex];
    }

    void SetActiveTab(size_t tabIndex){
        if (tabIndex < tabs.size()){
            activeTabIndex = tabIndex;
        }
    }
};
    

inline bool Tab::GoTo(PCIDLIST_ABSOLUTE dest, Actions action){
    if (!dest) return false;
    // change currentFolder, update directory, and push new path to history
    // preventing currentFolder from being null, becasue it will crash ILIsEqual

    // skip check for Actions::Refresh so that it always navigates
    if (action != Actions::Refresh && dir.parent.pidl && ILIsEqual(dest, dir.parent.pidl)){
        return false;
    }
    dir.UpdateParent(dest);

    if (action == Actions::Normal) history.Push(dir.parent.pidl.get()); 
    breadcrumbs = GenerateBreadcrumbs(dir.parent.pidl.get());
    selState.selectedHashes.clear();

    dir.updatedChildren = false;

    return true;
}

inline bool Tab::GoParent(){
    if (!CanGoParent()){
        return false;
    }
    PIDLIST_ABSOLUTE parentPidl = ILClone(dir.parent.pidl.get());
    if(!parentPidl) return false;
    ILRemoveLastID(parentPidl);


    // NavigateTo only ever reads from what we give it, does not clone, makes it own copy
    WShell::Pidl owned(parentPidl); // wrap in RAII, taking ownership, not cloning again
    return GoTo(owned.get());
}

inline void Tab::ReSort(TypenameStore& typeStore){
    dir.Sort(typeStore,viewState.sortMode, viewState.sortDir);
    if (!viewState.showHidden){
        dir.RebuildNonHiddenIndices();
    }
}


inline void Tab::DeselectAllItemsAndSelect(u64 i){
    selState.selectedHashes.clear();
    selState.selectedHashes.insert(i);
}

inline void Tab::AddItemToSelection(u64 i){
    selState.selectedHashes.insert(i);
}

inline void Tab::DeselectItem(u64 i){
    selState.selectedHashes.erase(i);
}
inline void Tab::DeselectAllItems(){
    selState.selectedHashes.clear();
}