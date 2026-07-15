#include "core.h"

namespace Threading{
    void Init(AppContext &ctx, u32 numThreads){
        ThreadPool &pool = ctx.threadPool;
        pool.terminate = false;

        for (u32 i = 0; i < numThreads; i++){
            // push_back expects a fully formed object, but we are constructing while appending
            pool.workers.emplace_back([&pool]() {   // lambda function that captures the pool reference, has access to queue, alarmclock, etc  
                Utils::InitCOM();   // this is the beginning of the function in each lambda 

                while (true){
                    Job currentJob;
                    {
                        std::unique_lock<std::mutex> lock(pool.queueMutex); // Workers takes the lock, and is the only one accessing the queue atp
                        // - Critical Section
                        pool.alarmClock.wait(lock, [&pool](){  // Go to sleep Until there is work OR they are terminated
                            return pool.terminate || !pool.queue.empty();   // sleep, until program is shutting down, or queue is not empty
                        });
                        
                        if (pool.terminate && pool.queue.empty()){   // if the app is closing and queue is empty, break out of infinite loop
                            break;
                        }

                        // Grab the job from the front of the queue
                        currentJob = std::move(pool.queue.front()); // std::move doesnt copy functions, it transfers ownership
                        pool.queue.pop();   // remove the first item from queue
                        // - Critical Section -  End
                    }// RAII doesnt trust me, so i dont trust RAII

                    if (currentJob.task){   // this just checks if the task is not empty
                        currentJob.task();  // this happen outside the lock or else only worker can be in the kitchen at any time, which is stupid, we only protecting the todo list
                    }
                }

                // clean up COM when thread dies
                CoUninitialize();
            });   
        }   
    }   

    void Enqueue(AppContext& ctx, u64 generation, std::function<void()> task) {
        ThreadPool& pool = ctx.threadPool;
        {
            // Grab the lock to safely push to the queue
            std::unique_lock<std::mutex> lock(pool.queueMutex);
            if (pool.terminate) return;
    
            pool.queue.push(Job{ generation, task });
        }
        
        // Ring the alarm clock to wake up exactly ONE sleeping worker
        pool.alarmClock.notify_one(); 
    }
    
    void Destroy(AppContext& ctx) {
        ThreadPool& pool = ctx.threadPool;
        {
            std::unique_lock<std::mutex> lock(pool.queueMutex);
            pool.terminate = true;
        }
        
        // Ring the alarm clock for ALL workers so they wake up and see they are fired
        pool.alarmClock.notify_all();
    
        // Wait for all workers to finish their current job and exit safely
        for (std::thread& worker : pool.workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }
}


