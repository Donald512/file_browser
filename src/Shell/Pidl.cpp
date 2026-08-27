#include "Pidl.h"


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

// Should be non const so it can be freed
PIDLIST_ABSOLUTE GetFullPidl(PCIDLIST_ABSOLUTE parent, PCITEMID_CHILD child){
    return ILCombine(parent, child);
}


u64 HashCombinedPidl(PCIDLIST_ABSOLUTE parent, PCITEMID_CHILD child) {
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
u64 HashIdentityString(const std::wstring& str){
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
std::wstring GetPidlIdentityString(PCIDLIST_ABSOLUTE fullPidl){
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
u64 HashItemIdentity(PCIDLIST_ABSOLUTE fullPidl){
    return HashIdentityString(GetPidlIdentityString(fullPidl));
}

// Overload 2: parent + child (used for clipboard CIDA items)
u64 HashItemIdentity(PCIDLIST_ABSOLUTE parentPidl, LPCITEMIDLIST childPidl){
    PIDLIST_ABSOLUTE fullPidl = ILCombine(parentPidl, childPidl);
    if (!fullPidl) return 0;
    u64 hash = HashItemIdentity(fullPidl);
    ILFree(fullPidl);
    return hash;
}