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

class DirectoryManager{
    public:
    DirectoryManager(TaskSystem& tasks, TypenameStore& typeStore) : tasks(tasks), typeStore(typeStore) {}
    TypenameStore& GetTypeStore() { return typeStore; }
    
    std::shared_ptr<const DirChildren> GetOrRequest(PCIDLIST_ABSOLUTE parentPidl, u64 hash){
        if (pendingRequests.find(hash) != pendingRequests.end()) return nullptr;   // not done yet
        else{
            FILETIME currentModTime = WShell::GetModifiedTime(parentPidl); 
            auto it = cache.find(hash);
            if (it != cache.end()){
                if (CompareFileTime(&it->second->modifiedTime, &currentModTime) == 0) return std::shared_ptr<const DirChildren>(it->second, &it->second->children);
                else Invalidate(hash);  // stale cache
            }
            
            // Launch Async Task
            auto cancelToken = std::make_shared<std::atomic<bool>>(false);
            pendingRequests[hash] = cancelToken;
            
            PIDLIST_ABSOLUTE pidlClone = ILClone(parentPidl);
            tasks.RunAsync(
                [pidlClone, currentModTime, hash, cancelToken]() -> std::optional<DirectoryBuildResult> {
                    // printf("[Worker] Started task for hash %llu\n", hash);
                    if (cancelToken->load()) {
                        ILFree(pidlClone);
                        return std::nullopt;
                    }

                    // Bind to a fresh IShellFolder for the MTA worker thread
                    IShellFolder* pDesktop = nullptr;
                    SHGetDesktopFolder(&pDesktop);
                    IShellFolder* pTarget = nullptr;
                    // HRESULT hr = pDesktop->BindToObject(pidlClone, nullptr, IID_IShellFolder, (void**)&pTarget);// Corrected line:
                    HRESULT hr = SHBindToObject(nullptr, pidlClone, nullptr, IID_IShellFolder, (void**)&pTarget);

                    // printf("[Worker] Started task for hash %llu\n", hash);
                    
                    if (FAILED(hr) || !pTarget){
                        if (pDesktop) pDesktop->Release();
                        ILFree(pidlClone);
                        return std::nullopt;
                    }

                    DirectoryBuildResult result;
                    
                    // printf("[Worker] Starting BuildDirectoryOffsite...\n");
                    result = BuildDirectoryOffsite(pTarget, pidlClone, *cancelToken);
                    // printf("[Worker] BuildDirectoryOffsite finished!\n");
                    
                    result.hash = hash;
                    result.modifiedTime = currentModTime;

                    pTarget->Release();
                    pDesktop->Release();
                    ILFree(pidlClone);
                    
                    if (cancelToken->load()) return std::nullopt;
                    return result;
                },
                // Main thread callback
                [this, hash](std::optional<DirectoryBuildResult> resultOpt){
                    pendingRequests.erase(hash);
                    if (!resultOpt.has_value()) return; // cancelled or failed

                    auto& result = resultOpt.value();

                    // Intern the strings on the main thread safely
                    for (size_t i = 0; i < result.rawTypes.size(); ++i) {
                        if (!result.rawTypes[i].empty()) {
                            u16 idx = typeStore.GetOrCreateId(result.rawTypes[i].c_str());
                            result.children.typenameIndex[i] = idx;
                        }
                    }
                    
                    // update cache
                    auto newCacheEntry = std::make_shared<CachedDirChildren>();
                    newCacheEntry->modifiedTime = result.modifiedTime;
                    newCacheEntry->children = std::move(result.children);
                    cache[hash] = newCacheEntry;
                    
                    // tell UI that results are ready
                }
            );
            return nullptr; // UI shows Loading until callback fires
        }
    }

    void Invalidate(u64 hash){
        cache.erase(hash);
        // if a request is in flight for this hash
        auto it = pendingRequests.find(hash);
        if (it != pendingRequests.end()){
            it->second->store(true);
            pendingRequests.erase(it);
        }
    }
    private:
        TaskSystem& tasks;
        TypenameStore& typeStore;
        std::unordered_map<u64, std::shared_ptr<CachedDirChildren>> cache;
        std::unordered_map<u64, std::shared_ptr<std::atomic<bool>>> pendingRequests;
};
