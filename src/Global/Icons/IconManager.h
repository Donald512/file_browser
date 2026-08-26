#pragma once
#include <unordered_map>
#include <unordered_set>
#include "BasicTypes.h"
#include "ShlObj.h"
#include "icons.h"
#include "TaskSystem.h"


// Thread-safe cache of icon indices keyed by hashed PIDLs.
// Missing entries are queued for asynchronous loading.

// Checks its unordered_map if it has seen the HashedPidl before, if it has, it returns the key, if it hasnt, it tells a worker thread to get it, and stores it in the map for next time. prevents multiple calculations

// will implement eviction if size becomes an issue

// does not need a mutex, because the background thread only queries the api, doesnt touch the map, it DispatchToMain(onDone), puts onDone into mainThreadJobs
// Then the UI takes onDone out of queue, and puts the key into the cache

class IconManager{
    public:
    IconManager(TaskSystem& tasks) : tasks(tasks) {}


    u32 GetIconIndex(PCIDLIST_ABSOLUTE pidl, u64 hash, DWORD dwFileAttributes, UINT uFlags) {
        auto it = iconIndexesOfPidls.find(hash);
        if (it != iconIndexesOfPidls.end()){
            return it->second;
        }
        u32 iconIndex = QuerySystemIconIndex(pidl, nullptr, dwFileAttributes, uFlags);
        iconIndexesOfPidls.emplace(hash, iconIndex);
        // guess we can store it if its an invalid result so we dont keep recomputing with bad parameters
        return iconIndex;
    }

    u32 GetIconIndex(PCIDLIST_ABSOLUTE pidl, u64 hash) {
        auto it = iconIndexesOfPidls.find(hash);
        if (it != iconIndexesOfPidls.end()){
            return it->second;
        }
        u32 iconIndex = QuerySystemIconIndex(pidl, nullptr, 0,  SHGFI_PIDL | SHGFI_SYSICONINDEX);
        iconIndexesOfPidls.emplace(hash, iconIndex);
        // guess we can store it if its an invalid result so we dont keep recomputing with bad parameters
        return iconIndex;
    }

    u32 GetIconIndex(PCIDLIST_ABSOLUTE parentPidl, PCITEMID_CHILD childPidl, u64 hash) {

        auto it = iconIndexesOfPidls.find(hash);
        if (it != iconIndexesOfPidls.end()) return it->second;

        
        if (pendingHashes.find(hash) != pendingHashes.end()) return UINT32_MAX;
        
        PIDLIST_ABSOLUTE absPidl = ILCombine(parentPidl, childPidl);
        if (!absPidl) return UINT32_MAX;
        
        pendingHashes.insert(hash);
        
        tasks.RunAsync(
            [absPidl]() -> int{
                int result = QuerySystemIconIndex(absPidl, nullptr, 0, SHGFI_PIDL | SHGFI_SYSICONINDEX);
                ILFree(absPidl);
                return result;
            }, 
            [this, hash] (int iconIndex){
                pendingHashes.erase(hash);
                if (iconIndex >= 0) iconIndexesOfPidls[hash] = (u32) iconIndex;
            }
        );
        return UINT32_MAX;
    }

    u32 GetIconIndex(IShellFolder* pParent, PCITEMID_CHILD pChild, u64 hash) {
        auto it = iconIndexesOfPidls.find(hash);
        if (it != iconIndexesOfPidls.end()){
            return it->second;
        }
        
        int iconIndex = -1;
        // Asks the parent shell folder to map its child PIDL 
        // to the system image list index
        SHMapPIDLToSystemImageListIndex(pParent, pChild, &iconIndex);
        iconIndexesOfPidls.emplace(hash, iconIndex);
        return iconIndex;
    }
    private:
        TaskSystem& tasks;
        std::unordered_map<u64, u32> iconIndexesOfPidls;
        std::unordered_set<u64> pendingHashes;
};