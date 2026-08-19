#include <vector>
#include "BasicTypes.h"
#include "WinFramework.h"
#include <ShObjIdl.h>
#include <memory>
#include <unordered_map>
#include "Item.h"
#include "Enum.h"

struct CachedDirChildren{
    DirChildren children;
    FILETIME modifiedTime = {};
};

class DirectoryManager{
    public:
    std::shared_ptr<const DirChildren> GetOrRequest(IShellFolder* parentFolder, PCIDLIST_ABSOLUTE parentPidl, u64 hash){

        FILETIME currentModTime = WShell::GetModifiedTime(parentPidl);

        auto it = cache.find(hash);
        if (it != cache.end()){
            if (CompareFileTime(&it->second->modifiedTime, &currentModTime) == 0){
                // ALIASING CONSTRUCTOR: Keeps the CachedDirChildren ref-count alive, 
                // but returns a pointer directly to the inner DirChildren struct!
                return std::shared_ptr<const DirChildren>(it->second, &it->second->children);
            }
            else{
                Invalidate(hash);
            }
        }
              
        // Mutable shared pointer internally
        auto newCacheEntry = std::make_shared<CachedDirChildren>();
        newCacheEntry->modifiedTime = currentModTime;
        
        // Put it in the cache immediately
        cache[hash] = newCacheEntry;  

        // Populate it
        newCacheEntry->children = GetDirChildren2(parentFolder, parentPidl);
        
        // Return the Aliased read-only pointer to the UI
        return std::shared_ptr<const DirChildren>(newCacheEntry, &newCacheEntry->children);

    }
    void Invalidate(u64 hash){
        cache.erase(hash);
    }
    private:
        std::unordered_map<u64, std::shared_ptr<const CachedDirChildren>> cache;
};
