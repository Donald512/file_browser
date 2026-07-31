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

    class MoveOnlyTask {
        struct Concept {
            virtual ~Concept() = default;
            virtual void Call() = 0;
        };

        template<typename F>
        struct Model final : Concept {
            F func;
            Model(F&& f) : func(std::move(f)) {}
            void Call() override { func();}
        };

        std::unique_ptr<Concept> impl;

    public :
        MoveOnlyTask() = default;

        template<typename F, typename = std::enable_if_t<!std::is_same_v<std::decay_t<F>, MoveOnlyTask>>>
        MoveOnlyTask(F&& f) : impl(std::make_unique<Model<std::decay_t<F>>>(std::forward<F>(f))) {}

        MoveOnlyTask(MoveOnlyTask&&) = default;
        MoveOnlyTask& operator=(MoveOnlyTask&&) = default;

        void operator()() {if (impl) impl->Call(); }
        explicit operator bool() const {return impl != nullptr; }
    };

    class ThreadPool {
        
    public:
        using JobFunction = std::function<void()>;
    
        explicit ThreadPool(u32 numThreads = std::thread::hardware_concurrency());
        ~ThreadPool();
    
        // Delete copy constructor & assignment operator to prevent accidental copies
        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;
    
        void Enqueue(JobFunction task);
        void Shutdown();
    
    private:
    
        std::vector<std::thread> workers;
        std::queue<JobFunction> jobQueue;
    
        std::mutex queueMutex;
        std::condition_variable cv;
    
        bool terminate = false;
    };

}