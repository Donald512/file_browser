#include "Navigation.h"
#include <wrl/client.h>
#include "Str.h"
#include <atomic>

#include "TaskSystem.h"

using Microsoft::WRL::ComPtr;
using namespace Navigation;


bool History::Push(PCIDLIST_ABSOLUTE folder){
    visited.push_back(WShell::Pidl(ILClone(folder)));
    currentIndex++;
    return true;
}   

bool NavigationController::NavigateTo(PCIDLIST_ABSOLUTE dest, Actions action){
    if (!dest) return false;
    // change currentFolder, update directory, and push new path to history
    // preventing currentFolder from being null, becasue it will crash ILIsEqual

    // skip check for Actions::Refresh so that it always navigates
    if (action != Actions::Refresh && currentFolder && ILIsEqual(dest, currentFolder)){
        return false;
    }
    currentFolder = WShell::Pidl(ILClone(dest));

    if (action == Actions::Normal){
        paths.Push(currentFolder.get());

    }

    u64 myGeneration = ++currentGeneration;
    loading = true;

    // A freshly-constructed Directory defaults to SortMode::Name/Ascending. Without carrying these forward, switching folders resets the current Sort mode back to name
    WShell::SortMode sortMode = contents.GetSort();
    WShell::SortDirection sortDir = contents.GetSortDir();

    WShell::Pidl target = currentFolder.Clone();
    tasks->RunAsync(
        // runs on worker thread. touches only its own locals, never 'this', never anything another thread might be looking at
        [target = std::move(target), sortMode, sortDir]() mutable {
            struct Result {
                Navigation::Breadcrumbs breadcrumbs;
                WShell::Directory contents;
            };
            Result r;
            r.breadcrumbs.Generate(target.get());
            r.contents.Load(target.get());
            r.contents.SetSort(sortMode, sortDir);
            return r;
        },
        // runs on main thread
        [this, myGeneration] (auto r) mutable {
            // A newer navigation has started since this one was kicked off - e.g. the user clicked through several folders faster than this one lloaded. Discard; whatever the newer navigation delivers, ends up on screen.
            if (myGeneration != currentGeneration.load()) return;

            r.contents.SetLoadGeneration(myGeneration);
            breadcrumbs = std::move(r.breadcrumbs);
            contents = std::move(r.contents);
            loading = false;
        }
    );

    // Update new menu

    return true;
}

// =======================================

bool Navigation::Breadcrumbs::Generate(PCIDLIST_ABSOLUTE folder){

    if (!folder) return false;

    crumbs.clear();
    fullPath.clear();
    hasSubFolders = false;

    WShell::Pidl accumulatedPidl; 
    PCIDLIST_ABSOLUTE pCurrentHop = folder;

    while (pCurrentHop && pCurrentHop->mkid.cb > 0){
        PIDLIST_RELATIVE pSingleItem = ILCloneFirst(pCurrentHop);
        PIDLIST_ABSOLUTE pNewAccumulated = ILCombine(accumulatedPidl.get(), pSingleItem);        
        accumulatedPidl =WShell::Pidl(pNewAccumulated);
        ILFree(pSingleItem); // free temporary split single item
        Breadcrumb crumb;

        wchar_t* pAllocatedName = nullptr;
        if (SUCCEEDED(SHGetNameFromIDList(accumulatedPidl.get(), SIGDN_NORMALDISPLAY, &pAllocatedName))) {
            crumb.displayName = Str::WideToString(pAllocatedName);
            CoTaskMemFree(pAllocatedName); 
        }
        crumb.pidl =WShell::Pidl(ILClone(accumulatedPidl.get()));
        crumb.hash = WShell::HashPidl(crumb.pidl.get());
        crumbs.push_back(std::move(crumb));
        pCurrentHop = ILGetNext(pCurrentHop);
    }    
    fullPath =WShell::PidlToTypeablePath(accumulatedPidl.get());   
    hasSubFolders =WShell::PidlHasSubFolders(accumulatedPidl.get());
    
    return true;
}

// =======================================

bool NavigationController::GoParent(){
    if (!CanGoParent()){
        return false;
    }
    PIDLIST_ABSOLUTE parentPidl = ILClone(currentFolder.get());
    if(!parentPidl) return false;
    ILRemoveLastID(parentPidl);


    // NavigateTo only ever reads from what we give it, does not clone, makes it own copy
    WShell::Pidl owned(parentPidl); // wrap in RAII, taking ownership, not cloning again
    return NavigateTo(owned.get());
}
