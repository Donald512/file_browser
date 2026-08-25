#pragma once

#include "Item.h"
#include "ShlObj.h"
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


DirItem MakeSidebarItem(std::string name, PCIDLIST_ABSOLUTE itemPidl);

DirItem MakeSidebarItem(PCIDLIST_ABSOLUTE pidl);

std::vector<DirItem> GetRecents();

std::vector<DirItem> GetBookmarks();


std::vector<DirItem> GetStorage();

std::vector<DirItem> GetDocuments();

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