// threading.cpp
/* 
Load 4 (or set amount of threads at runtime)
Send them to sleep
Put work in queue
When work arrives, wake workers up
When each worker finishes, it goes to the next available work in queue

*/

// prolly go into core.h, its own namespace
// namespace Threading::Patience in ms
// constexpr Icon = 100
// constexpr Thumbnail = 500
// ...  Metadata = 300

// Struct Job Queue, or Array of Jobs, with Capacity, Jobs, 
// prolly add functionality where array destroys from top to bottom, or not

// Struct Job
// Function pointers, with arguments, and patience argument
// enum state, done, pending or untouched
