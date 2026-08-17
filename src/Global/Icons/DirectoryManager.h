#include <vector>
#include "BasicTypes.h"
#include "WinFramework.h"
#include <ShObjIdl.h>
#include <memory>
#include <unordered_map>

struct CachedFolder {
    // String Arena (SoA)
    std::vector<char> nameArena;
    std::vector<u32>  nameOffsets;
    std::vector<u16>  nameLengths;
    
    // PIDL Arena (SoA) - Raw bytes packed sequentially
    std::vector<u8>   pidlArena;     // Stores raw ITEMIDLIST bytes back-to-back
    std::vector<u32>  pidlOffsets;   // Offset into pidlArena for each item
    std::vector<u16>  pidlLengths;   // Length of each child PIDL in bytes
    
    // Parallel Attributes
    std::vector<u64>      hashes;
    std::vector<SFGAOF>   attributes;
    std::vector<FILETIME> lastWriteTimes;
    std::vector<u64>      sizes;
    
    size_t ItemCount() const { return hashes.size(); }

    // Helper to view a raw child PIDL without allocating memory
    PCITEMID_CHILD GetChildPidl(size_t index) const {
        return reinterpret_cast<PCITEMID_CHILD>(&pidlArena[pidlOffsets[index]]);
    }
};

class DirectoryManager{
    public:
    std::shared_ptr<const CachedFolder> Get(u64 folderHash, PCIDLIST_ABSOLUTE folder){
        auto it = cache.find(folderHash);
        if (it != cache.end()){
            return it->second;
        }
        
    }
    private:
        std::unordered_map<u64, std::shared_ptr<const CachedFolder>> cache;
};

std::shared_ptr<const CachedFolder> GetOrRequest(PCIDLIST_ABSOLUTE folderPidl, u64 folderHash){
    auto cached = std::make_shared<CachedFolder>();
    // since this runs once per initial enumeration, just use no reserving

}