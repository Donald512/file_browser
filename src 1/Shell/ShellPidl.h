#pragma once

#include <ShlObj.h>

namespace WShell{
    class Pidl{
    public:
        Pidl() = default;
        Pidl(std::nullptr_t) : ptr(nullptr) {}
        explicit Pidl(PIDLIST_ABSOLUTE owned) : ptr(owned) {}
        explicit Pidl(PCIDLIST_ABSOLUTE unowned) : ptr(unowned ? ILClone(unowned) : nullptr) {}
        
        ~Pidl(){ if (ptr) ILFree(ptr); }
        
        // no copying — a pidl has one owner. Use Clone() if you need a duplicate.
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