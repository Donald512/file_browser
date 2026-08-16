#pragma once

#include "Item.h"
#include "KnownSpecialFolders.h"
#include "Enum.h"
#include "Pidl.h"
#include "ShlObj.h"
#include "Shell.h"
#include "iconRegular.h"

// own special file becuase will include bookmarks too, and later, special tags
enum class SbarCategory {Recents, Bookmarks, Storage, Documents};

// pointer to a functon that takes no arguments and returns a std::vector<DirItem>
using SidebarLoader = std::vector<DirItem>(*)();

struct SidebarCategory {
    SbarCategory cat;
    const char* name;
    const char* icon;
    SidebarLoader loader;
    std::vector<DirItem> contents;

    bool isOpen = true;

    // Explicitly handle move/copy due to non-copyable elements
    SidebarCategory(SbarCategory c, const char* n, const char* i, SidebarLoader l)
        : cat(c), name(n), icon(i), loader(l), isOpen(true) {}

    SidebarCategory(const SidebarCategory&) = delete;
    SidebarCategory& operator=(const SidebarCategory&) = delete;
    SidebarCategory(SidebarCategory&&) noexcept = default;
    SidebarCategory& operator=(SidebarCategory&&) noexcept = default;

    void Load() {
        contents = loader();
    }
};


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

struct SidebarManager {
    std::vector<SidebarCategory> categories;

    void Init() {
        categories.reserve(4);
        
        categories.push_back({ SbarCategory::Recents,   "Recents",     ICON_REG_CLOCK,      GetRecents });
        categories.push_back({ SbarCategory::Bookmarks, "Bookmarks",   ICON_REG_BOOKMARK,   GetBookmarks });
        categories.push_back({ SbarCategory::Storage,   "Storage",     ICON_REG_HARD_DRIVE, GetStorage });
        categories.push_back({ SbarCategory::Documents, "Documents",   ICON_REG_DOCUMENT,   GetDocuments });

        for (auto& category : categories)
            category.Load();
    }
};