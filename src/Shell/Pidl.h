#pragma once

#include <ShlObj.h>
#include "BasicTypes.h"

namespace WShell{
    class Pidl{
    public:
        Pidl() = default;
        Pidl(std::nullptr_t) : ptr(nullptr) {}
        explicit Pidl(PIDLIST_ABSOLUTE owned) : ptr(owned) {}
        explicit Pidl(PCIDLIST_ABSOLUTE unowned) : ptr(unowned ? ILClone(unowned) : nullptr) {}
        
        ~Pidl(){ if (ptr) ILFree(ptr); }
        
        // no copying — a pidl has one owner. Use Clone() for a duplicate.
        Pidl(const Pidl&) = delete;
        Pidl& operator=(const Pidl&) = delete;
        
        Pidl(Pidl&& other) noexcept : ptr(other.ptr) { other.ptr = nullptr; }
        Pidl& operator=(Pidl&& other) noexcept{
            if (this != &other){
                if (ptr) ILFree(ptr);
                ptr = other.ptr;
                other.ptr = nullptr;
            }
            return *this;
        }
        
        explicit operator bool() const { return ptr != nullptr; }
        
        PCIDLIST_ABSOLUTE get() const { return ptr; }
        operator PCIDLIST_ABSOLUTE() const { return ptr; } // lets it be passed anywhere a raw pidl is expected
        
        PIDLIST_ABSOLUTE* GetAddressOf(){
            if (ptr){
                ILFree((LPITEMIDLIST)ptr);
                ptr = nullptr;
            }
            return &ptr;
        }
        
        Pidl Clone() const { return Pidl(ptr ? ILClone(ptr) : nullptr); }
        
    private:
        PIDLIST_ABSOLUTE ptr = nullptr;
    };
    
}



u64 HashPidl(PCIDLIST_ABSOLUTE pidl){
    if (!pidl) return 0;

    // Pure in-memory FNV-1a over the raw ITEMIDLIST bytes. ILGetSize() just walks the
    // linked SHITEMID structure summing `cb` fields — no shell/COM call, no I/O — so this
    // is cheap enough to call every frame and safe to call from any thread.
    const unsigned char* bytes = reinterpret_cast<const unsigned char*>(pidl);
    UINT size = ILGetSize(pidl);

    u64 hash = 1469598103934665603ULL; // FNV offset basis
    for (UINT i = 0; i < size; ++i){
        hash ^= bytes[i];
        hash *= 1099511628211ULL; // FNV prime
    }
    return hash;
}