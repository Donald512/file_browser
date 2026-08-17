#include "BasicTypes.h"
#include "WinFramework.h"
#include "iostream"

#define KB(x) ((x) * 1024ULL)
#define MB(x) ((x) * 1024ULL * 1024ULL)
#define GB(x) ((x) * 1024ULL * 1024ULL * 1024ULL)

class Arena{
    public:
        bool Init(u64 _cap);
        void* Alloc(u64 numElem, u64 elemSize, u64 alignSize);
        void Reset();

    private:
        char* buffer = nullptr;
        u64 offset = 0;     // current offset in arena
        u64 size = 0;       // current commited bytes
        u64 capacity = 0;   // total reserved bytes
        u64 pageSize = 0;

        void* Commit();
    
};

bool Arena::Init(u64 _cap){
    void* ptr = VirtualAlloc(NULL, _cap, MEM_RESERVE, PAGE_NOACCESS);
    if (ptr == nullptr){
        std::cout << "Reservation failed: " << GetLastError() << std::endl;
        return false;
    }
    capacity = _cap;
    buffer = (char*)ptr;

    SYSTEM_INFO si;
    GetSystemInfo(&si);
    pageSize = si.dwPageSize;

    return true;
}

void* Arena::Alloc(u64 numElem, u64 elemSize, u64 alignSize){
    if (alignSize == 0 || (alignSize & (alignSize - 1)) != 0){  // not a power of 2
        std::cout << "Not a power of 2" << std::endl;
        return nullptr;
    }
    
    uintptr_t allocationSize = numElem * elemSize; 
    if (numElem != 0 && allocationSize / numElem < elemSize){ // check if theres overflow
        std::cout << "Allocation overflow" << std::endl;
        return nullptr;
    }
    
    uintptr_t currentAddr = (uintptr_t)buffer + (uintptr_t)offset;
    // calculate padding
    uintptr_t padding = (~currentAddr + 1) & (alignSize - 1);

    uintptr_t alignedAddr = currentAddr + padding;
    uintptr_t neededEndAddr = alignedAddr + allocationSize;


    if (neededEndAddr > (uintptr_t)buffer + capacity){
        std::cout << "Arena capacity exhausted" << std::endl;
        return nullptr;
    }

    // if it is greater than capacity. prolly create a linked buffer
    while(neededEndAddr > size + (uintptr_t)buffer){
        uintptr_t bytesToCommit = neededEndAddr - ((uintptr_t)buffer + size);

        // round up to nearest page size
        bytesToCommit = (bytesToCommit + pageSize - 1) & ~(pageSize - 1);

        void* commitedPtr = VirtualAlloc(buffer + size, bytesToCommit, MEM_COMMIT, PAGE_READWRITE);
        if (commitedPtr == nullptr){
            std::cout << "Commit returned nullptr: " << GetLastError() << std::endl;
            return nullptr;
        }

        size += bytesToCommit;
    }
    offset = neededEndAddr - (uintptr_t)buffer;
    memset((void*)alignedAddr, 0, allocationSize);
    return (void*) alignedAddr;
}

void Arena::Reset(){
    offset = 0;
}