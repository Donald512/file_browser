#pragma once

#include <mutex>

#include "BasicTypes.h"
#include <vector>
#include "Item.h"
#include "TaskSystem.h"

class Arena;
struct FileSysItem;
struct TaskSystem;


class SearchResults{
    private:
    mutable std::mutex m_mutex; 

    size_t m_totalResults = 0;
    
    TaskSystem& tasks;
    TypenameStore& typeStore;
    
    public:
    SearchResults(TaskSystem& tasks, TypenameStore& typeStore) : tasks(tasks), typeStore(typeStore) {}
    
    void Init();
    void appendBatch(const FileSysItem* batch, size_t count, const char* paths, size_t numBytes);
    
    void clear();
    DirChildren children;   // safe to read, even though it grows, because every frame, we dont keep the pointer, we get the new location evvery frame
    mutable std::vector<u32> refs;
    const std::vector<u32>& VisibleIndices() const;
     
    void SearchFolder(const wchar_t* query, const wchar_t* folderPath, bool isRecursive );
};

