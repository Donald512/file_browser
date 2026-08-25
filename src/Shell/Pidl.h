#pragma once

#include <ShlObj.h>
#include "BasicTypes.h"
#include <string>


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



u64 HashPidl(PCIDLIST_ABSOLUTE pidl);

// Should be non const so it can be freed
PIDLIST_ABSOLUTE GetFullPidl(PCIDLIST_ABSOLUTE parent, PCITEMID_CHILD child);


u64 HashCombinedPidl(PCIDLIST_ABSOLUTE parent, PCITEMID_CHILD child);

// FNV-1a hash, case-insensitive
u64 HashIdentityString(const std::wstring& str);

// Resolve a PIDL to its canonical identity string.
// SIGDN_FILESYSPATH first (real files → "C:\Users\...\foo"),
// SIGDN_DESKTOPABSOLUTEPARSING fallback (virtual items → "::{CLSID}\...").
// Same object always resolves to the same string, regardless of whether
// the PIDL is "lean" (clipboard CIDA) or "rich" (EnumObjects).
std::wstring GetPidlIdentityString(PCIDLIST_ABSOLUTE fullPidl);

// Overload 1: absolute PIDL (used for listing items)
u64 HashItemIdentity(PCIDLIST_ABSOLUTE fullPidl);

// Overload 2: parent + child (used for clipboard CIDA items)
u64 HashItemIdentity(PCIDLIST_ABSOLUTE parentPidl, LPCITEMIDLIST childPidl);