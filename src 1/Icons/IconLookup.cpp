#include "IconLookup.h"

u32 Icons::GetIconIndex(PCIDLIST_ABSOLUTE pidl, const wchar_t* pszPath, DWORD dwFileAttributes, UINT uFlags){
    // if (!pidl) return 0;

    // memoize, map<Pidl | wchar_t*, bool>
    SHFILEINFOW sfi = {};

    if (pidl){
        SHGetFileInfoW((LPCWSTR) pidl, dwFileAttributes, &sfi, sizeof(sfi), uFlags);
    }
    else if (pszPath){
        SHGetFileInfoW(pszPath, dwFileAttributes, &sfi, sizeof(sfi), uFlags);
    }
    // sfi.iIcon now contains the unique icon index
    return sfi.iIcon; // even if it fails, it returns 0
}
