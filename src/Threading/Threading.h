#include <thread>              // std::thread
#include <vector>              // std::vector
#include <queue>               // std::queue
#include <mutex>               // std::mutex, std::unique_lock
#include <condition_variable>  // std::condition_variable
#include <functional>          // std::function
#include <atomic>              // std::atomic
#include <cstdint>             // uint64_t (if you use standard integer types)
#include <utility>             // std::move
#include "Types.h"

class ThreadPool{
public:
    using JobFunction = std::function<void()>;

    ThreadPool(size_t numThreads = std::thread::hardware_concurrency());
    ~ThreadPool();

    void Enqueue(u64 generation, JobFunction task);

    void Shutdown();

private:
    struct Job
    {
        u64 generation;
        JobFunction task;
    };

    void WorkerLoop();

private:
    std::vector<std::thread> workers;

    std::queue<Job> jobQueue;

    std::mutex queueMutex;
    std::condition_variable cv;

    bool terminate = false;
};