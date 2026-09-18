
#include <mutex>
#include <cstring>
#include "WinFramework.h"
#include <Windows.h>
#include <Shlwapi.h>
#include <cassert>

#include "SearchResults.h"
#include "Item.h"
#include "BasicTypes.h"
#include "MemoryArena.h"
#include "global.h"
#include "TaskSystem.h"
#include "match.h"
#include "Str.h"
#include "TypenameManager.h"

struct WorkerBatch{
    static constexpr size_t BATCH_CAP = 512;
    static constexpr size_t STRING_CAP = 128 * BATCH_CAP; // 64 KB

    FileSysItem results[BATCH_CAP];
    size_t resultCount = 0;

    char charBuffer[STRING_CAP];
    size_t charBytesUsed = 0;

    void add(SearchResults& globalPool, u64 size, FILETIME lastMod, const char* path, u32 pathLen);

    char* reserveChars(SearchResults& pool, u32 maxLen);

    void commit(u64 size, FILETIME lastMod, u32 actualLen, SFGAOF attrs, u64 hash, u16 typenameIdx);

    // Flush batch to the global arena in ONE atomic operation
    void flush(SearchResults& globalPool);
};


void WorkerBatch::add(SearchResults& globalPool, u64 size, FILETIME lastMod, const char* path, u32 pathLen){
    // if scratchPad is full, flush batch to the global pool
    CHECK_VOID(pathLen <= STRING_CAP);
    if (resultCount >= BATCH_CAP || (charBytesUsed + pathLen) > STRING_CAP) flush(globalPool);

    // Copy path into batch char buffer
    char* dest = &charBuffer[charBytesUsed];
    std::memcpy(dest, path, pathLen);
    charBytesUsed += pathLen;

    FileSysItem result;
    result.path = StringSlice{dest, pathLen};
    result.size = size;
    result.lastMod = lastMod;

    results[resultCount++] = result;
}

void WorkerBatch::flush(SearchResults& globalPool){
    if (!resultCount) return;
    globalPool.appendBatch(results, resultCount, charBuffer, charBytesUsed);

    resultCount = 0;
    charBytesUsed = 0;
}

char* WorkerBatch::reserveChars(SearchResults& pool, u32 maxLen) {
    if (resultCount >= BATCH_CAP || (charBytesUsed + maxLen) > STRING_CAP) flush(pool);
    return &charBuffer[charBytesUsed];
}

void WorkerBatch::commit(u64 size, FILETIME lastMod, u32 actualLen, SFGAOF attrs, u64 hash, u16 typenameIdx) {
    FileSysItem result;
    result.path = StringSlice{&charBuffer[charBytesUsed], actualLen};
    charBytesUsed += actualLen;
    result.size = size;
    result.lastMod = lastMod;
    result.attrs = attrs;
    result.hash = hash;
    result.typenameIndex = typenameIdx;
    results[resultCount++] = result;
}

// App or Tab
// Dirty duct tape for now
void SearchResults::Init(){
    children.hashes.reserve(1000000);
    children.attributes.reserve(1000000);
    children.sizes.reserve(1000000);
    children.lastWriteTimes.reserve(1000000);
    children.nameOffsets.reserve(1000000);
    children.nameLengths.reserve(1000000);
    children.typenameIndex.reserve(1000000);
    
    // 64 MB for raw string characters
    children.nameArena.reserve(64 * 1024 * 1024); 
}

void SearchResults::appendBatch(const FileSysItem* batch, size_t count, const char* paths, size_t numBytes){
    if (!count) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    
    for (size_t i = 0; i < count; i++){
        const auto& child = batch[i];
        u32 nameLen = child.path.len;
        u32 nameOffset = (u32)children.nameArena.size();

        children.nameArena.resize(nameOffset + nameLen);
        std::memcpy(&children.nameArena[nameOffset], child.path.data, nameLen);
        children.nameOffsets.push_back(nameOffset);
        children.nameLengths.push_back(nameLen);

        children.typenameIndex.push_back(child.typenameIndex);
        children.hashes.push_back(child.hash);
        children.attributes.push_back(child.attrs);
        children.sizes.push_back(child.size);
        children.lastWriteTimes.push_back(child.lastMod);

    }
}

void SearchResults::clear(){
    children.hashes.clear();
    children.attributes.clear();
    children.lastWriteTimes.clear();
    children.sizes.clear();
    children.pidlOffsets.clear();
    children.pidlLengths.clear();
    children.nameOffsets.clear();
    children.nameLengths.clear();
    children.typenameIndex.clear();
    children.pidlArena.clear();
    children.nameArena.clear();
}


void SearchRecursive(const std::wstring& _query, const std::wstring& _currentPath, WorkerBatch& batch, SearchResults& resultsPool, TypenameStore& typeStore){
    CHECK_VOID(_query.c_str());
    CHECK_VOID(_currentPath.c_str());
    
    // dreading turning this function to casey approved 
    std::wstring query(_query);
    std::wstring folderPath(_currentPath);
    
    CHECK_VOID(folderPath.size());
    CHECK_VOID(query.size());
    
    if (folderPath.back() != L'\\') folderPath += L'\\';
    
    folderPath += L"*";
    
    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileExW(folderPath.c_str(), FindExInfoBasic, &findData, FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH);
    
    if (hFind == INVALID_HANDLE_VALUE) return;
    std::vector<std::wstring> subfoldersToSearch;
    do {
        if (wcscmp(findData.cFileName, L".") == 0 || wcscmp(findData.cFileName, L"..") == 0) continue;
        bool isDirectory = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        
        bool isMatch = false;
        if (!query.empty()){
            if (PathMatchSpecW(findData.cFileName, query.c_str())) isMatch = true;
            else if (MatchFuzzySubsequence(query, findData.cFileName)) isMatch = true;
        }
        else isMatch = true; // though it would never get here
        
        if (isMatch){
            
            FILETIME lastMod = findData.ftLastWriteTime;
            u64 itemSize = ((u64)findData.nFileSizeHigh << 32) | findData.nFileSizeLow;
            
            char* dest = batch.reserveChars(resultsPool, MAX_PATH * 3);
            int utf8PathLen = Str::WideToUtf8(findData.cFileName, dest);
            CHECK_VOID(dest != nullptr);
            CHECK_VOID(utf8PathLen != -1);
            
            // commit(u64 size, FILETIME lastMod, u32 actualLen, SFGAOF attrs, u64 hash, u16 typenameIdx)
            std::wstring fullPath = folderPath.substr(0, folderPath.size() - 2) + findData.cFileName;   // for (/* ) or just keep the query and the path seperate, later
            u64 hash = HashIdentityString(fullPath);
            
            batch.commit(itemSize, lastMod, utf8PathLen, findData.dwFileAttributes, hash, TypenameStore::InvalidIndex);
        }
        if (isDirectory){
            std::wstring subFolder = folderPath + findData.cFileName;
            subfoldersToSearch.push_back(subFolder);
        }
        
    } while(FindNextFileW(hFind, &findData));
    
    FindClose(hFind);
    for (const auto& subFolder : subfoldersToSearch) {
        SearchRecursive(query.c_str(), subFolder.c_str(), batch, resultsPool, typeStore);
    }
}

void SearchResults::SearchFolder(const wchar_t* _query, const wchar_t* _folderPath, bool isRecursive ){
    
    std::wstring query(_query);
    std::wstring folderPath(_folderPath);

    tasks.RunAsync(
        [this, query = std::move(query), folderPath = std::move(folderPath)](){
            WorkerBatch workerBatch;
            SearchRecursive(query, folderPath, workerBatch, *this, typeStore);
            workerBatch.flush(*this);
        },
        [this](){}
        // this callback's job is just "a folder finished", e.g.:
        //   - decrement an outstanding-task counter
        //   - signal the UI thread that new results are available / repaint
        //   - if it was the *last* outstanding task, signal "search complete"
    );
}

const std::vector<u32>& SearchResults::VisibleIndices() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    refs.resize(children.ItemCount());
    // For now
    for (int i = 0; i < children.ItemCount(); i++){
        refs[i] = i;
    }
    return refs;
}