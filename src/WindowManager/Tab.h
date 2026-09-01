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
#include "Watcher.h"
#include <algorithm>
#include "CtxMenu.h"

// maybe this for multiple different windows
struct WindowManager{};

enum class Actions {Normal, Back, Forward, Refresh};
struct SelectionState {
    bool justNavigated = false;
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


class Tab{
    public:
    
    Tab(DirectoryManager& dirManager, DirectoryWatcher& watcher, PCIDLIST_ABSOLUTE startFolder) : dirManager(&dirManager), watcher(&watcher){
        GoTo(startFolder);
    }
    bool GoTo(PCIDLIST_ABSOLUTE dest, Actions action = Actions::Normal);
    
    bool CanGoBack() const {return history.CanGoBack();}
    bool CanGoForward() const {return history.CanGoForward();}
    bool CanGoParent() const {
        if (!dir.parent.pidl || ILIsEmpty(dir.parent.pidl.get())) return false;
        return true;
    }
    bool GoBack(){
        if (!history.Back()) return false;
        return GoTo(history.Current(),  Actions::Back);
    }
    bool GoForward(){
        if (!history.Forward()) return false;
        return GoTo(history.Current(), Actions::Forward);      
    }
    bool GoParent();
    bool Refresh(){
        return GoTo(history.Current(), Actions::Refresh);
    }
    
    // void SelectItem(u64 i, SelectMode mode);
    void DeselectAllItemsAndSelect(u64 i);
    void DeselectItem(u64 i);
    void AddItemToSelection(u64 i);
    void DeselectAllItems();
    
    bool isSelected(u64 i) const{
        return selState.selectedHashes.find(i) != selState.selectedHashes.end();
    }
    
    void ReSort();
    void ToggleShowHidden(){
        viewState.showHidden = !viewState.showHidden;
        if (viewState.showHidden == false){
            const DirChildren* PChildren = dirManager->Get(dir.HChildren);
            if (PChildren){
                dir.RebuildNonHiddenIndices(*PChildren);
            }
        }
    }

    void ClearSelState();
    void ClearViewState();
    
    Breadcrumbs breadcrumbs{};
    History history{};
    Directory dir{};
    
    FileViewState viewState;
    SelectionState selState;
    CtxMenuState ctxState;
    
    private:
        DirectoryManager* dirManager;
        DirectoryWatcher* watcher;
};

// Maybe Window manager handles multiple windows, or maybe its not neccessary
struct Window{
    DirectoryManager& directory;
    DirectoryWatcher& watcher;
    Window(DirectoryManager& dm, DirectoryWatcher& dw) : directory(dm), watcher(dw){}

    std::vector<Tab> tabs{};
    size_t activeTabIndex = 0;  // or maybe vector for tile windows
    
    void NewTab(PCIDLIST_ABSOLUTE startFolder = SpecialFolders::defaultStartupFolder){
        if (!startFolder) startFolder = SpecialFolders::defaultStartupFolder;
        tabs.emplace_back(directory, watcher, startFolder);
        activeTabIndex = tabs.size() - 1;
    }

    void CloseTab(size_t tabIndex){
        u64 hash = tabs[tabIndex].dir.parent.hash;
        watcher.Stop(hash);
        tabs.erase(tabs.begin() + tabIndex);
        activeTabIndex = std::clamp(activeTabIndex, (size_t) 0, tabs.size() - 1);
    }

    Tab& GetActiveTab(){ return tabs[activeTabIndex];}

    void SetActiveTab(size_t tabIndex){
        if (tabIndex < tabs.size()){ activeTabIndex = tabIndex; }
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

    dir.ClearForNav();
    ClearSelState();
    ClearViewState();

    dir.UpdateChildren(*dirManager, viewState);
    watcher->Watch(dir.parent.pidl.get(), dir.parent.hash);


    return true;
}

inline bool Tab::GoParent(){
    if (!CanGoParent()) return false;
    PIDLIST_ABSOLUTE parentPidl = ILClone(dir.parent.pidl.get());
    if(!parentPidl) return false;
    ILRemoveLastID(parentPidl);


    // NavigateTo only ever reads from what we give it, does not clone, makes it own copy
    WShell::Pidl owned(parentPidl); // wrap in RAII, taking ownership, not cloning again
    return GoTo(owned.get());
}

inline void Tab::ReSort(){
    const DirChildren* PChildren = dirManager->Get(dir.HChildren);
    if (!PChildren) return;
    dir.Sort(*PChildren, dirManager->GetTypeStore(), viewState);

    if (!viewState.showHidden) dir.RebuildNonHiddenIndices(*PChildren);
}


inline void Tab::DeselectAllItemsAndSelect(u64 i){
    selState.selectedHashes.clear();
    selState.selectedHashes.insert(i);
}

inline void Tab::AddItemToSelection(u64 i){    selState.selectedHashes.insert(i);}
inline void Tab::DeselectItem(u64 i){    selState.selectedHashes.erase(i);}
inline void Tab::DeselectAllItems(){    selState.selectedHashes.clear();}

inline void Tab::ClearSelState(){
    selState.justNavigated = false;
    selState.selectedHashes.clear();
    selState.focusHash = std::nullopt;
    selState.anchorHash = std::nullopt;
    selState.anchorVisualIndex = -1;
}

inline void Tab::ClearViewState(){
    viewState.renamingItemId = std::nullopt;
    viewState.renameBuffer[0] = 0;
    viewState.renameFocusHandledFor = 0;
}