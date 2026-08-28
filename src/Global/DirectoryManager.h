#pragma once

#include <vector>
#include "BasicTypes.h"
#include "WinFramework.h"
#include <ShObjIdl.h>
#include <memory>
#include <atomic>
#include <optional>
#include <unordered_map>
#include "Item.h"
#include "Enum.h"
#include "TypenameManager.h"

struct CachedDirChildren{
    DirChildren children;
    FILETIME modifiedTime = {};
};


struct CachedDirHandle{
    u32 index = UINT32_MAX;
    u32 gen = 0;

    bool isValid() const {return index != UINT32_MAX;}

    bool operator==(const CachedDirHandle& other) const{ return index == other.index &&  gen == other.gen; };
    bool operator!=(const CachedDirHandle& other) const{ return !(*this == other); };
};


struct CachedDirSlot{
    CachedDirChildren cachedData = {};
    u64 folderHash = 0;
    u32 gen = 1;
    bool hasData = false;
    
    // Async stuff
    bool isPending = false;
};

class DirectoryManager{
    public:
    DirectoryManager(TaskSystem& tasks, TypenameStore& typeStore) : tasks(tasks), typeStore(typeStore) {}
    TypenameStore& GetTypeStore() { return typeStore; }

    const DirChildren* Get(CachedDirHandle handle) const; 
    CachedDirHandle GetOrRequest(PCIDLIST_ABSOLUTE parentPidl, u64 hash);   

    void InvalidateByHash(u64 hash);
    void InvalidateSlot(u32 slotIdx);

    private:
        std::vector<CachedDirSlot> slots;   // Cached dirchildren that we have
        std::vector<u32> freeSlots;         // pool of reusable array indices
        std::unordered_map<u64, u32> hashToSlotIndex;   // Hashmap of pidlHash to vector index that tells the index where the cachedDirInfo is

        TaskSystem& tasks;
        TypenameStore& typeStore;

        u32 AcquireSlot(u64 hash);  // helper to find an empty slot or grow the array
};

// Used by Tab Dir Member to get a pointer to where the actual data is in memory
inline const DirChildren* DirectoryManager::Get(CachedDirHandle handle) const{  
    if (handle.index >= slots.size()) return nullptr;

    const CachedDirSlot& slot = slots[handle.index]; // handle.index is the index into the slots vector
    if (slot.gen != handle.gen || !slot.hasData)   return nullptr;  // whoever owns this handle is holding stale data, or the slot isnt being used 

    return &slot.cachedData.children;
}
// This is used to kickstart querying the API. Gets the Folder children data and 
inline CachedDirHandle DirectoryManager::GetOrRequest(PCIDLIST_ABSOLUTE parentPidl, u64 hash){
    u32 slotIdx;

    auto it = hashToSlotIndex.find(hash);
    if (it != hashToSlotIndex.end())  slotIdx = it->second;    // we have a hit
    else slotIdx = AcquireSlot(hash);   // else, get an empty slot to store new data

    CachedDirSlot& slot = slots[slotIdx];

    if (slot.isPending) return {slotIdx, slot.gen}; // if the async is already sent, dont resend it 

    FILETIME currentModTime = WShell::GetModifiedTime(parentPidl);

    if (slot.hasData){  // if this is not a new slot
        if (CompareFileTime(&slot.cachedData.modifiedTime, &currentModTime) == 0)   return {slotIdx, slot.gen};
        else{
            InvalidateSlot(slotIdx);
            slotIdx = AcquireSlot(hash); 
            // No risk of slot being a stale reference here, because AcquireSlot will most highly likely, not cause the vector to realocate, since it reuses the empty space  created by InvalidateSlot
        }

    }

    slots[slotIdx].isPending = true;
    u32 generation = slots[slotIdx].gen;

    PIDLIST_ABSOLUTE pidlClone = ILClone(parentPidl);

    tasks.RunAsync(
        [pidlClone, currentModTime]() -> std::optional<DirectoryBuildResult>{
            IShellFolder* pTarget = nullptr;
            HRESULT hr = SHBindToObject(nullptr, pidlClone, nullptr, IID_IShellFolder, (void**)&pTarget);

            if (FAILED(hr) || !pTarget){
                ILFree(pidlClone);
                return std::nullopt;       // returns a valid handle, but the handle does  not contain data
            }

            DirectoryBuildResult result = BuildDirectoryOffsite(pTarget, pidlClone);
            result.modifiedTime = currentModTime;

            pTarget->Release();
            ILFree(pidlClone);
            return result;
        },
        [this, slotIdx, generation](std::optional<DirectoryBuildResult> resultOpt){
            CachedDirSlot& slot = slots[slotIdx];
            
            // Gen check: If someone invalidated while we were working, discard
            // if the slot that we were gonna put the result in (the generation), changes while we were computing the result, or getting the result failed, return
            // Prevents an old directory from rewriting new cache data
            if (slot.gen != generation || !resultOpt.has_value()) return;  // ! Dont get this
            
            slot.isPending = false;      // this has to happen after we are sure this result belongs to this thread. tres important. another thread might be reusing this slot and has set slot.isPending to false

            auto& result = resultOpt.value();
            
            // intern strings on main thread
            for (size_t i = 0; i < result.rawTypes.size(); i++){
                if (!result.rawTypes[i].empty()){
                    TypenameStore::TypeIndex typeID = typeStore.GetOrCreateId(result.rawTypes[i].c_str());
                    result.children.typenameIndex[i] = typeID;
                }
            }
            
            slot.cachedData.modifiedTime = result.modifiedTime;
            slot.cachedData.children = std::move(result.children);
            slot.hasData = true;
        }
    );
    return {slotIdx, slots[slotIdx].gen};   // just to be safe, we dont use slot.gen
}

// Gets an empty slot and fills in the folderHash, and places the slot index in the map
inline u32 DirectoryManager::AcquireSlot(u64 hash){
    u32 slotIdx;
    if (!freeSlots.empty()){    // we return the last free spots in freeSlots, 
        slotIdx = freeSlots.back();
        freeSlots.pop_back();
    }
    else{   
        // if its empty, we cant use freeSlots, so we increase the size of slots, so we can store the new element
        // we dont increase the size of slots else, because we reuse an index thats no longer in use
        slotIdx = (u32)slots.size();    
        slots.emplace_back();
    }

    slots[slotIdx].folderHash = hash;   // just fill in the 
    slots[slotIdx].hasData = false; 
    
    hashToSlotIndex[hash] = slotIdx;
    
    return slotIdx;
}

inline void DirectoryManager::InvalidateByHash(u64 hash){
    auto it = hashToSlotIndex.find(hash);
    if (it != hashToSlotIndex.end())  InvalidateSlot(it->second);
} 

inline void DirectoryManager::InvalidateSlot(u32 slotIdx) {
    if (slotIdx >= slots.size()) return;

    CachedDirSlot& slot = slots[slotIdx];

    slot.hasData = false;
    slot.gen++; // increment so anyone with that handle will fail Get()
    slot.isPending = false;

    slot.cachedData.modifiedTime = {};
    slot.cachedData.children = {};
    
    hashToSlotIndex.erase(slot.folderHash);
    slot.folderHash = 0;
    freeSlots.push_back(slotIdx);
}