#pragma once
#include "BasicTypes.h"
#include "History.h"
#include "Breadcrumbs.h"
#include "Item.h"
#include <unordered_set>
#include "Enum.h"


template <typename T>
inline T ExploraClamp(T value, T minValue, T maxValue){
    if (value < minValue) return minValue;
    else if (value > maxValue) return maxValue;
    return value;
}

// maybe this for multiple different windows
struct WindowManager{};


enum class Actions {Normal, Back, Forward, Refresh};
enum class SortMode { Name, DateModified, Type, Size};
enum class ViewMode { ExtraLarge, Large, Medium, Small, List, Details, Tiles}; // feel like this belongs to  UI
enum class SortDirection {Ascending, Descending };
enum class SelectMode {OneItem};

struct FileViewState {
    // Change to user's last choice, or a setttings
    ViewMode viewMode = ViewMode::List;   
    SortMode sortMode = SortMode::Name;
    SortDirection sortDir = SortDirection::Ascending;
    bool showHidden = false;
    
    // UI Directives (The "Magic" variables)
    std::optional<u64> scrollToItemId = std::nullopt;
    std::optional<u64> renamingItemId = std::nullopt;
    float scrollY = 0.0f;
    
    f32 gridIconSize = 64.0f; 
};

class Tab{
    public:
        bool GoTo(PCIDLIST_ABSOLUTE dest, Actions action = Actions::Normal);
        
        bool CanGoBack() const {return history.CanGoBack();}
        bool CanGoForward() const {return history.CanGoForward();}
        bool CanGoParent() const {
            if (!currentFolder.pidl || ILIsEmpty(currentFolder.pidl.get())) return false;
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

        bool isSelected(u64 i) const{
            return selectedItems.find(i) != selectedItems.end();
        }

        void selectItem(u64 i, SelectMode mode){
            switch (mode){
                case SelectMode::OneItem:{
                    selectedItems.clear();
                    selectedItems.insert(i);
                    break;
                }
            }
        }
        DirItem currentFolder;
        
        Breadcrumbs breadcrumbs{};
        History history{};
        std::vector<DirItem> dirEntry{}; 
        std::unordered_set<u64> selectedItems{};
        
        FileViewState viewState;
    private:
        // WShell::Pidl currentFolder; // Prolly the same as Breadcrumbs.crumbs.back()

        void UpdateDirEntry();        
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
    

bool Tab::GoTo(PCIDLIST_ABSOLUTE dest, Actions action){
    if (!dest) return false;
    // change currentFolder, update directory, and push new path to history
    // preventing currentFolder from being null, becasue it will crash ILIsEqual

    // skip check for Actions::Refresh so that it always navigates
    if (action != Actions::Refresh && currentFolder.pidl && ILIsEqual(dest, currentFolder.pidl)){
        return false;
    }
    currentFolder.pidl = WShell::Pidl(ILClone(dest));
    
    if (action == Actions::Normal) history.Push(currentFolder.pidl.get()); 
    breadcrumbs = GenerateBreadcrumbs(currentFolder.pidl.get());
    selectedItems.clear();
    UpdateDirEntry();

    // Update new menu

    return true;
}

bool Tab::GoParent(){
    if (!CanGoParent()){
        return false;
    }
    PIDLIST_ABSOLUTE parentPidl = ILClone(currentFolder.pidl.get());
    if(!parentPidl) return false;
    ILRemoveLastID(parentPidl);


    // NavigateTo only ever reads from what we give it, does not clone, makes it own copy
    WShell::Pidl owned(parentPidl); // wrap in RAII, taking ownership, not cloning again
    return GoTo(owned.get());
}

void Tab::UpdateDirEntry(){
    dirEntry = EnumFolder(currentFolder.pidl.get(), &currentFolder);
}
