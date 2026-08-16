#pragma once

#include <string>
#include "Str.h"
#include "Shell.h"
#include <ShlObj.h>
#include "BasicTypes.h"
#include <vector>
#include "IconManager.h"
#include "Lazy.h"

struct Breadcrumb{
    std::string displayName;
    WShell::Pidl pidl;
    u64 hash;
    
    // ! delete thhis
    mutable Lazy<u32> iconKey;
    u32 IconKey(const IconManager& icons) const {
        return iconKey.Get([&]{
            return icons.GetIconIndex(pidl.get(), hash, 0, SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
        });
    }
};

struct Breadcrumbs{
    std::string fullPath;
    bool hasSubFolders = false;
    std::vector<Breadcrumb> crumbs; //  list of active crumbs
        
};


Breadcrumbs GenerateBreadcrumbs(PCIDLIST_ABSOLUTE folder){
    Breadcrumbs breadcrumbs{};
    if (!folder) return breadcrumbs;

    WShell::Pidl accumulatedPidl; 
    PCIDLIST_ABSOLUTE pCurrentHop = folder;

    while (pCurrentHop && pCurrentHop->mkid.cb > 0){
        PIDLIST_RELATIVE pSingleItem = ILCloneFirst(pCurrentHop);
        PIDLIST_ABSOLUTE pNewAccumulated = ILCombine(accumulatedPidl.get(), pSingleItem);        
        accumulatedPidl = WShell::Pidl(pNewAccumulated);
        ILFree(pSingleItem); // free temporary split single item
        Breadcrumb crumb;

        crumb.displayName = WShell::GetDisplayName(accumulatedPidl.get());
        
        crumb.pidl = WShell::Pidl(ILClone(accumulatedPidl.get()));
        crumb.hash = HashPidl(crumb.pidl.get());
        breadcrumbs.crumbs.push_back(std::move(crumb));
        pCurrentHop = ILGetNext(pCurrentHop);
    }    
    breadcrumbs.fullPath = WShell::GetFullPath(accumulatedPidl.get());   
    breadcrumbs.hasSubFolders =WShell::PidlHasSubFolders(accumulatedPidl.get());
    
    return breadcrumbs;
}
