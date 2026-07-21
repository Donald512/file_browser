#include "Navigation.h"
#include <wrl/client.h>
#include "..\Str\Str.h"

using Microsoft::WRL::ComPtr;
using namespace Navigation;


bool History::Push(PCIDLIST_ABSOLUTE folder){
    visited.push_back(WShell::Pidl(ILClone(folder)));
    return true;
}   

bool NavigationController::NavigateTo(PCIDLIST_ABSOLUTE dest, Actions action){
    if (!dest) return false;
    // change currentFolder, update directory, and push new path to history
    // preventing currentFolder from being null, becasue it will crash ILIsEqual
    if (currentFolder && ILIsEqual(dest, currentFolder)){
            return false;
    }
    currentFolder =WShell::Pidl(ILClone(dest));

    if (action == Actions::Normal){
        paths.Push(currentFolder.get());

    }
    breadcrumbs.Generate(currentFolder.get());
    contents.Load(currentFolder.get());

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

    return NavigateTo(parentPidl);
}
