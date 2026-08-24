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



inline u64 HashPidl(PCIDLIST_ABSOLUTE pidl){
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

// Should be non const so it can be freed
inline PIDLIST_ABSOLUTE GetFullPidl(PCIDLIST_ABSOLUTE parent, PCITEMID_CHILD child){
    return ILCombine(parent, child);
}


inline u64 HashCombinedPidl(PCIDLIST_ABSOLUTE parent, PCITEMID_CHILD child) {
    u64 hash = 1469598103934665603ULL; // FNV offset basis
    const unsigned char* bytes;
    UINT size;

    // 1. Hash Parent bytes (EXCLUDING the 2-byte null terminator)
    if (parent && !ILIsEmpty(parent)) {
        bytes = reinterpret_cast<const unsigned char*>(parent);
        size = ILGetSize(parent);
        if (size >= 2) {
            for (UINT i = 0; i < size - 2; ++i) {
                hash ^= bytes[i];
                hash *= 1099511628211ULL;
            }
        }
    }

    // 2. Hash Child bytes (INCLUDING its 2-byte null terminator)
    if (child && !ILIsEmpty(child)) {
        bytes = reinterpret_cast<const unsigned char*>(child);
        size = ILGetSize(child);
        for (UINT i = 0; i < size; ++i) {
            hash ^= bytes[i];
            hash *= 1099511628211ULL;
        }
    } else {
        // Empty child just adds the 2-byte null terminator
        hash ^= 0; hash *= 1099511628211ULL;
        hash ^= 0; hash *= 1099511628211ULL;
    }

    return hash;
}

// FNV-1a hash, case-insensitive
inline u64 HashIdentityString(const std::wstring& str){
    if (str.empty()) return 0;
    u64 hash = 1469598103934665603ULL;
    for (wchar_t c : str){
        hash ^= (u64)towlower(c);
        hash *= 1099511628211ULL;
    }
    return hash;
}

// Resolve a PIDL to its canonical identity string.
// SIGDN_FILESYSPATH first (real files → "C:\Users\...\foo"),
// SIGDN_DESKTOPABSOLUTEPARSING fallback (virtual items → "::{CLSID}\...").
// Same object always resolves to the same string, regardless of whether
// the PIDL is "lean" (clipboard CIDA) or "rich" (EnumObjects).
inline std::wstring GetPidlIdentityString(PCIDLIST_ABSOLUTE fullPidl){
    PWSTR path = nullptr;
    if (SUCCEEDED(SHGetNameFromIDList(fullPidl, SIGDN_FILESYSPATH, &path))){
        std::wstring result = path;
        CoTaskMemFree(path);
        return result;
    }
    if (SUCCEEDED(SHGetNameFromIDList(fullPidl, SIGDN_DESKTOPABSOLUTEPARSING, &path))){
        std::wstring result = path;
        CoTaskMemFree(path);
        return result;
    }
    return L"";
}

// Overload 1: absolute PIDL (used for your listing items)
inline u64 HashItemIdentity(PCIDLIST_ABSOLUTE fullPidl){
    return HashIdentityString(GetPidlIdentityString(fullPidl));
}

// Overload 2: parent + child (used for clipboard CIDA items)
inline u64 HashItemIdentity(PCIDLIST_ABSOLUTE parentPidl, LPCITEMIDLIST childPidl){
    PIDLIST_ABSOLUTE fullPidl = ILCombine(parentPidl, childPidl);
    if (!fullPidl) return 0;
    u64 hash = HashItemIdentity(fullPidl);
    ILFree(fullPidl);
    return hash;
}