

#include "MemoryArena.h"
#include "BasicTypes.h"
#include "WinFramework.h"
#include "iostream"
#include <Windows.h>
#include <cassert>
#include "global.h"
#include "macros.h"


bool ArenaInit(Arena* arena, u64 _cap){
    CHECK_RET((arena->buffer == nullptr) && (arena->capacity == 0), false);
    void* ptr = VirtualAlloc(NULL, _cap, MEM_RESERVE, PAGE_NOACCESS);
    if (ptr == nullptr){
        std::cout << "Reservation failed: " << GetLastError() << std::endl;
        return false;
    }
    arena->capacity = _cap;
    arena->buffer = (char*)ptr;
    arena->committed = 0;
    arena->offset = 0;

    SYSTEM_INFO si;
    GetSystemInfo(&si);
    arena->pageSize = si.dwPageSize;

    return true;
}



void* ArenaPush(Arena* arena, u64 numElem, u64 elemSize, u64 alignSize){
    CHECK_RET(alignSize > 0 && (alignSize & (alignSize - 1)) == 0, nullptr);   // alignment must be a power of 2 and > 0
    CHECK_RET(numElem != 0 && elemSize != 0, nullptr);

    CHECK_RET(numElem <= UINTPTR_MAX / elemSize, nullptr);
    uintptr_t allocationSize = numElem * elemSize; 
    
    uintptr_t currentAddr = (uintptr_t)arena->buffer + (uintptr_t)arena->offset;
    uintptr_t padding = (alignSize - (currentAddr & (alignSize - 1))) & (alignSize - 1);

    CHECK_RET((padding <= UINTPTR_MAX - arena->offset), nullptr);
    uintptr_t offsetWithPadding = arena->offset + padding;

    CHECK_RET((allocationSize <= UINTPTR_MAX - offsetWithPadding), nullptr);
    uintptr_t newOffset = offsetWithPadding + allocationSize;

    CHECK_RET(newOffset <= arena->capacity, nullptr);

    uintptr_t alignedAddr = currentAddr + padding;
    

    if (newOffset > arena->committed) {
        uintptr_t bytesToCommit = newOffset - arena->committed;

        uintptr_t ps = (uintptr_t)arena->pageSize;
        bytesToCommit = (bytesToCommit + ps - 1) & ~(ps - 1);

        void* committedPtr = VirtualAlloc(arena->buffer + arena->committed, bytesToCommit, MEM_COMMIT, PAGE_READWRITE);
        if (committedPtr == nullptr) {
            std::cout << "Commit returned nullptr: " << GetLastError() << std::endl;
            return nullptr;
        }

        arena->committed += bytesToCommit;
    }

    arena->offset = newOffset;
    // memset((void*)alignedAddr, 0, allocationSize);  // todo delete this in release
    return (void*) alignedAddr;
}

