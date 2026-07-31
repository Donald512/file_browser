#include "Threading.h"
#include <combaseapi.h>

#pragma comment(lib, "Ole32.lib")

using namespace Threading;

ThreadPool::ThreadPool(u32 numThreads) {
    // Fall back to at least 1 worker if hardware_concurrency returns 0
    if (numThreads == 0) {
        numThreads = 1;     // todo check implication of changing to 4
    }

    workers.reserve(numThreads);

    for (u32 i = 0; i < numThreads; ++i) {
        workers.emplace_back([this]() {
            // Initialize COM library on this thread
            HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
            const bool comInitialized = SUCCEEDED(hr);

            while (true) {
                JobFunction currentJob;
                {
                    std::unique_lock<std::mutex> lock(queueMutex);

                    cv.wait(lock, [this]() {
                        return terminate || !jobQueue.empty();
                    });

                    if (terminate && jobQueue.empty()) {
                        break;
                    }

                    currentJob = std::move(jobQueue.front());
                    jobQueue.pop();
                }

                if (currentJob) {
                    currentJob();
                }
            }

            // Only uninitialize if COM was successfully initialized on this thread
            if (comInitialized) {
                CoUninitialize();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    Shutdown();
}

void ThreadPool::Enqueue(JobFunction task) {
    if (!task) return;

    {
        std::unique_lock<std::mutex> lock(queueMutex);
        if (terminate) {
            return; // Ignore enqueues after shutdown
        }
        jobQueue.push(std::move(task));
    }

    cv.notify_one();
}

void ThreadPool::Shutdown() {
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        if (terminate) {
            return; // Prevent multiple shutdowns
        }
        terminate = true;
    }

    cv.notify_all();

    for (std::thread& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    workers.clear();
}