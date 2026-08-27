#include "sidebarEnum.h"

#include "KnownSpecialFolders.h"
#include "Enum.h"
#include "Pidl.h"
#include "Shell.h"

DirItem MakeSidebarItem(std::string name, PCIDLIST_ABSOLUTE itemPidl){
    DirItem item;
    item.name = std::move(name);
    item.pidl = WShell::Pidl(itemPidl);   // clones — caller keeps ownership of itemPidl
    item.hash = HashPidl(item.pidl.get());
    return item;
}

DirItem MakeSidebarItem(PCIDLIST_ABSOLUTE pidl) {
    return MakeSidebarItem(WShell::GetDisplayName(pidl), pidl);
}

std::vector<DirItem> GetRecents(){
    std::vector<DirItem> items;
    const WShell::Pidl& quickAccess = SpecialFolders::pidlQuickAccess;
    
    IterateFolder(quickAccess, SHCONTF_FOLDERS | SHCONTF_NAVIGATION_ENUM, [&](IShellFolder* target, PITEMID_CHILD child) {
        (void) target;
        WShell::Pidl pinnedPidl = WShell::Pidl(ILCombine(quickAccess.get(), child));
        items.push_back(MakeSidebarItem(pinnedPidl.get()));
    });

    return items;
}

std::vector<DirItem> GetBookmarks(){
    std::vector<DirItem> items;
    return items;
}


std::vector<DirItem> GetStorage(){
    std::vector<DirItem> items;
    // This PC's real drives/containers (skips virtual entries like "Gallery")
    IterateFolder(SpecialFolders::pidlThisPC, SHCONTF_FOLDERS | SHCONTF_STORAGE | SHCONTF_NAVIGATION_ENUM, [&](IShellFolder* target, PITEMID_CHILD child) {
        SFGAOF attrs = SFGAO_FOLDER | SFGAO_STREAM;
        if (FAILED(target->GetAttributesOf(1, (LPCITEMIDLIST*)&child, &attrs))) return;
        
        bool isRealContainer = (attrs & SFGAO_FOLDER) && !(attrs & SFGAO_STREAM);
        if (!isRealContainer) return;
        
        WShell::Pidl drivePidl = WShell::Pidl(ILCombine(SpecialFolders::pidlThisPC.get(), child));
        items.push_back(MakeSidebarItem(drivePidl.get()));
    });
    
    const WShell::Pidl& recycleBin = SpecialFolders::pidlRecycleBin;
    items.push_back(MakeSidebarItem(recycleBin.get()));
    
    return items;
}

std::vector<DirItem> GetDocuments(){
    std::vector<DirItem> items;

    items.push_back(MakeSidebarItem(SpecialFolders::pidlHome.get()));

    std::vector<DirItem> accounts = GetOneDriveAccounts();

    for (auto& account : accounts){
        items.push_back(std::move(account));
    }
    
    items.push_back(MakeSidebarItem(SpecialFolders::pidlDocuments.get()));
    items.push_back(MakeSidebarItem(SpecialFolders::pidlDownloads.get()));
    
    return items;
    
}
