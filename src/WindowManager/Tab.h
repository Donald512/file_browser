#pragma once
#include "BasicTypes.h"
#include <unordered_set>
#include <optional>
#include <algorithm>

#include "History.h"
#include "Breadcrumbs.h"
#include "Item.h"
#include "Enum.h"
#include "Directory.h"
#include "TypenameManager.h"
#include "KnownSpecialFolders.h"
#include "Breadcrumbs.h"
#include "Watcher.h"
#include "CtxMenu.h"

// maybe this for multiple different windows
struct WindowManager{};

enum class Actions {Normal, Back, Forward, Refresh};

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
    
    Breadcrumbs breadcrumbs{};
    History history{};
    Directory dir{};
    
    FileViewState viewState;
    SelectionState selState;
    CtxMenuState ctxState;
    RenameState renameState;
    NewState newState;

    
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

    dir.ClearForNav();

    viewState.Clear();
    selState.Clear();
    renameState.Clear();

    // this is for the case, where its already cached, wont run in Fileview.cpp, because UpdateChildren, never returns true
    if (dir.UpdateChildren(*dirManager, viewState)){    
        if (const DirChildren* fresh = dirManager->Get(dir.HChildren)){
            selState.selectedMask.Resize(fresh->ItemCount());
        }
    }
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