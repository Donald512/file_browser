#pragma once
#include "BasicTypes.h"
#include "ShlObj.h"



inline u32 QuerySystemIconIndex(PCIDLIST_ABSOLUTE pidl, const wchar_t* pszPath, DWORD dwFileAttributes, UINT uFlags) {
    SHFILEINFOW sfi = {};

    // Ensure SHGFI_SYSICONINDEX is always set
    UINT flags = uFlags | SHGFI_SYSICONINDEX;

    if (pidl) {
        SHGetFileInfoW(reinterpret_cast<LPCWSTR>(pidl), dwFileAttributes, &sfi, sizeof(sfi), flags);
    } else if (pszPath) {
        SHGetFileInfoW(pszPath, dwFileAttributes, &sfi, sizeof(sfi), flags);
    }

    return sfi.iIcon;
}