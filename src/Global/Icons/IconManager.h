#pragma once
#include <unordered_map>
#include "BasicTypes.h"
#include "ShlObj.h"
#include "icons.h"

// Thread-safe cache of icon indices keyed by hashed PIDLs.
// Missing entries are queued for asynchronous loading.

// Checks its unordered_map if it has seen the HashedPidl before, if it has, it returns the key, if it hasnt, it tells a worker thread to get it, and stores it in the map for next time. prevents multiple calculations

// will implement eviction if size becomes an issue
class IconManager{

    public:
    u32 GetIconIndex(PCIDLIST_ABSOLUTE pidl, u64 hash, DWORD dwFileAttributes, UINT uFlags) const {
        auto it = IconIndexesOfPidls.find(hash);
        if (it != IconIndexesOfPidls.end()){
            return it->second;
        }
        u32 iconIndex = QuerySystemIconIndex(pidl, nullptr, dwFileAttributes, uFlags);
        IconIndexesOfPidls.emplace(hash, iconIndex);
        // guess we can store it if its an invalid result so we dont keep recomputing with bad parameters
        return iconIndex;
    }

    u32 GetIconIndex(PCIDLIST_ABSOLUTE pidl, u64 hash) const {
        auto it = IconIndexesOfPidls.find(hash);
        if (it != IconIndexesOfPidls.end()){
            return it->second;
        }
        u32 iconIndex = QuerySystemIconIndex(pidl, nullptr, 0,  SHGFI_PIDL | SHGFI_SYSICONINDEX);
        IconIndexesOfPidls.emplace(hash, iconIndex);
        // guess we can store it if its an invalid result so we dont keep recomputing with bad parameters
        return iconIndex;
    }
    private:
        mutable std::unordered_map<u64, u32> IconIndexesOfPidls;
};