#pragma once
#include "WinFramework.h"
#include <thread>
#include "TaskSystem.h"
#include "DirectoryManager.h"
#include <string>
#include <vector>
#include <memory>
#include "AppCommands.h"

struct WatchState{
    u64 hash = 0;
    std::wstring path;
    HANDLE hDir = INVALID_HANDLE_VALUE;
    std::thread thread;
    std::atomic<bool> stopFlag{false};
    std::chrono::steady_clock::time_point lastInvalidate;
};

class DirectoryWatcher{
    public:
    DirectoryWatcher(TaskSystem& tasks, DirectoryManager& dirManager, std::function<void(AppCommand)> queueCommand) : tasks(tasks), dirManager(dirManager), queueCommand(std::move(queueCommand)){}

    ~DirectoryWatcher(){
        StopAll();
    }

    // Start watching a directory. Returns a handle that allows it to be stopped.
    void Watch(PCIDLIST_ABSOLUTE pidl, u64 hash);
    void Stop(u64 hash);
    void StopAll();

    private:
    void WatcherThreadLoop(WatchState* state);

    TaskSystem& tasks;
    DirectoryManager& dirManager;
    std::function<void(AppCommand)> queueCommand;

    std::unordered_map<u64, std::unique_ptr<WatchState>> watches;
};

inline void DirectoryWatcher::Stop(u64 hash){
    auto it = watches.find(hash);
    if (it == watches.end()) return;

    WatchState& s = *it->second;
    s.stopFlag = true;

    if (s.hDir != INVALID_HANDLE_VALUE) CancelIoEx(s.hDir, nullptr); // unblocks worker
    if (s.thread.joinable()) s.thread.join();                        // worker fully done
    if (s.hDir != INVALID_HANDLE_VALUE) CloseHandle(s.hDir);         // now safe
    watches.erase(it);
}

inline void DirectoryWatcher::StopAll(){
    std::vector<u64> hashes;
    for (auto& [hash, _] : watches){
        hashes.push_back(hash);
    }
    for (u64 hash : hashes){
        Stop(hash);
    }
}

inline void DirectoryWatcher::Watch(PCIDLIST_ABSOLUTE pidl, u64 hash){
    // Already watching this hash?
    if (watches.find(hash) != watches.end()) return;

    wchar_t path[MAX_PATH];
    if (!SHGetPathFromIDListW(pidl, path)) return;  // not a file system path

    
    // FILE_FLAG_BACKUP_SEMANTICS is mandatory for directories
    HANDLE hDir = CreateFileW(path, FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if ((hDir) == INVALID_HANDLE_VALUE) return;
    
    auto watch = std::make_unique<WatchState>();
    watch->hash = hash;
    watch->path = path;
    watch->hDir = hDir;

    WatchState* state = watch.get();
    watches[hash] = std::move(watch);   

    state->thread = std::thread([this, state](){
        WatcherThreadLoop(state);
    });
}

inline void DirectoryWatcher::WatcherThreadLoop(WatchState* state){
    // Buffer for change notifications (8KB)
    constexpr size_t bufferSize = 64 * 1024;
    std::vector<BYTE> buffer(bufferSize);;

    // Watch for: file/folder creation, deletion, rename, modification
    DWORD dwNotifyFilter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE| FILE_NOTIFY_CHANGE_CREATION;

    while (!state->stopFlag){
        DWORD bytesReturned = 0;
        
        // no recursion
        BOOL success = ReadDirectoryChangesW(state->hDir, buffer.data(), bufferSize, FALSE, dwNotifyFilter, &bytesReturned, nullptr, nullptr); 
        
        DWORD err = success ? 0 : GetLastError();
        if (!success && err != ERROR_NOTIFY_ENUM_DIR)   break;  // Cancelled or handle died

        bool somethingChanged = success ? (bytesReturned > 0)   : true;  // overflow

        auto now = std::chrono::steady_clock::now();
        if (somethingChanged && now - state->lastInvalidate >= std::chrono::milliseconds(300)){
            state->lastInvalidate = now;
            u64 hash = state->hash;
            tasks.DispatchToMain([this, hash](){
                dirManager.InvalidateByHash(hash); 
                queueCommand(Cmd_RefreshByHash{hash});
            });  // destroy and rebuild
            
        }
    }
}


/*
    When the user a directory currently being watched, ReadDirectoryChangesW will fail and the thread will exit. Therefore the tab currently showing will not be updated, cos theres nothing currently watching it. Will prolly close the tab 
    This does not watch virtual folders  
*/