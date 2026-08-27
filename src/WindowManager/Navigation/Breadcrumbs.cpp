#include "Breadcrumbs.h"
#include "Shell.h"


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
