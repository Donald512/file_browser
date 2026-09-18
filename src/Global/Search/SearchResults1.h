#pragma once

#include <mutex>

#include "BasicTypes.h"
#include "MemoryArena.h"

class Arena;
struct FileSysItem;
struct TaskSystem;

class SearchResults{
    private:

    std::mutex m_mutex;
    Arena g_resultArena;
    Arena g_charArena;
    size_t m_totalResults = 0;

    TaskSystem& tasks;

    public:
    SearchResults(TaskSystem& tasks) : tasks(tasks){}

    void Init();
    void appendBatch(const FileSysItem* batch, size_t count, const char* paths, size_t numBytes);
    size_t size();
    const FileSysItem* data() const { return (const FileSysItem*)g_resultArena.buffer;}

    void clear();

    void SearchFolder(const wchar_t* query, const wchar_t* folderPath, bool isRecursive );
};

