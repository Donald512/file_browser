#pragma once

#include "UniqueFunction.h"
#include <mutex>
#include <thread>
#include <queue>
#include "Threading.h"


struct TaskSystem{

    // Threading
    Threading::ThreadPool threadPool{ std::thread::hardware_concurrency() };

    std::mutex mainThreadMutex;
    std::queue<UniqueFunction> mainThreadJobs;

    void DispatchToMain(UniqueFunction job) {
        std::lock_guard<std::mutex> lock(mainThreadMutex);  // just locks the list of jobs so only one thing is using it at once
        mainThreadJobs.push(std::move(job));    // push the new job to the list 
    }

    void RunMainThreadJobs() {
        std::queue<UniqueFunction > jobs; // temporary list to hold all the jobs
        {
            std::lock_guard<std::mutex> lock(mainThreadMutex);  // lock job
            std::swap(jobs, mainThreadJobs); // Fast swap,. moves all the jobs from main thread to temporary list
        }   //  unlocks Lock
        while (!jobs.empty()) { // do the jobs one by one
            jobs.front()(); // run the top job
            jobs.pop(); // discard when done
        }
    }

    /*
        1 - 'work' runs on a worker thread and returns an owned, moveable result, never a reference/pointer into anything the main thread might touch. this matters because when threads finish, their stack is destroyed and whatever they reference or pointer they return is dangling
        2 - The returned result is handed to 'onDone', which runs on the main thread. This is intentionally the only way to touch shared state (Item/ItemLite fields, ImGui/D3D11 state, AppContext members) from a worker threads' result - by never actually touching it. the worker only ever computes a plain value, and mutation of shared state happens on main thread inside onDone. 
        3 - Staleness/cancellation is the caller's job - check 'is this current?' by using eg (a generation counter, a still-open node, etc at the top of onDone) before using it
    */

    template <typename TWork, typename TOnDone>     // TWork is a callable that does background work, and TOnDone is  a callable that handles the result
    void RunAsync(TWork&& work, TOnDone&& onDone) {
        threadPool.Enqueue([this, work = std::forward<TWork>(work), onDone = std::forward<TOnDone>(onDone)]() mutable {
            if constexpr (std::is_void_v<std::invoke_result_t<TWork>>){ // checks at compile time, if calling work() returns void 
                work();
                DispatchToMain([onDone = std::move(onDone)]() mutable {onDone(); });
            }   else{   // if work returns something, eg an icon key
                auto result = work();
                DispatchToMain([onDone = std::move(onDone), result = std::move(result)]() mutable {
                    onDone(std::move(result));
                });
            }
        });
    }
};

/*
    Main() calls OleInitialize(nullptr) which is COINIT_APARTMENTTHREADED (STA), which is required because this is the thread pumping window messages
    Worker threads init COINIT_MULTITHREADED (MTA). Any IShellFolder*, IContextMenu*, IShellItem* obtained on the main thread cannot be handed to a worker-thread 
    Never pass an IShellFolder* accross the thread boundary. PIDLIST_ABSOLUTE are plain data, safe to copy and hand to a worker
    Rebind with SHBindToObject or SHGetDesktopFolder, on the worker thread from th PIDL

    Cannot append to DirChildren's Arena, has to build its own off site, and swap the shared_ptr

    IconManager, DirectoryManager and unordered_map's need locking

    TypenameStore::GetOrCreateId mutates a shared arena (m_arena, m_offsets, m_stringToId), needs a mutex, GetDirChildren2 calls it per item.

    Never touch ImGui state, D3D device/context or any HWND/HMENU from a worker thread. Rendering, window messages, menu construction must stay main-thread only. background work is for computing hata, and handed back to DispatchToMain to apply.

    Priority:
    DirectoryManager::GetOrRequest / GetDirChildren. Needs loading visual too
    IconManager
    ReadDirectoryChangesW
    Copy, move, delete

*/