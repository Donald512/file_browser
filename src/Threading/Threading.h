#pragma once

#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <cstdint>
#include <utility>

#include "Types.h"


namespace Threading{

    class ThreadPool {
        
    public:
        using JobFunction = UniqueFunction;
    
        explicit ThreadPool(u32 numThreads = std::thread::hardware_concurrency());
        ~ThreadPool();
    
        // Delete copy constructor & assignment operator to prevent accidental copies
        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;
    
        void Enqueue(JobFunction task);
        void Shutdown();
    
    private:
        // td plan to add timeout functionality to threads, so ill prolly make a 
        /*
        struct Job{
            JobFunction task;
            std::chrono::steady_clock::time_point timeout;
            // random stuff like ID, priority etc
        }
        */
    
        std::vector<std::thread> workers;
        std::queue<JobFunction> jobQueue;
    
        std::mutex queueMutex;
        std::condition_variable cv;
    
        bool terminate = false;
    };

}

// The threadpool has no idea what a "generation" is. 
// cancellation/staleness is the caller's concern, whoever enques a job does its "is this still relevant" check