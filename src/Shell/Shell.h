#pragma once

#include <ShlObj.h>
#include <ShlObj_core.h>
#include "Pidl.h"
#include <string>

namespace WShell{

    // Extracts a child's display name.
    std::string GetDisplayName(IShellFolder* folder, PCITEMID_CHILD child, SHGDNF flags);

    // this function might have a bug, because the destination needs to know the size of the name
    int GetDisplayName2(IShellFolder* folder, PCITEMID_CHILD child, SHGDNF flags, char* dest);

    bool GetWideDisplayName2(IShellFolder* folder, PCITEMID_CHILD child, SHGDNF flags, wchar_t* wBufOut);
    
    std::string GetDisplayName(PCIDLIST_ABSOLUTE pidl );
        
    // Helper: Safely fetches a Shell name and automatically handles COM memory & UTF-8 conversion
    std::string GetShellName(PCIDLIST_ABSOLUTE pidl, SIGDN sigdn);

    std::string GetFullPath(PCIDLIST_ABSOLUTE pidl);

    bool GetPidlTypeName(IShellFolder* pParent, PCITEMID_CHILD childPidl, wchar_t* outBuffer, UINT maxChars);

    bool GetItemTypeName(PCIDLIST_ABSOLUTE parentPidl, IShellFolder* pParent, PCITEMID_CHILD child, wchar_t* out);
    
    Pidl GetFullPath(const wchar_t* widePath);

    
    bool PidlHasSubFolders(PCIDLIST_ABSOLUTE folder, bool accurate = false);

    u32 GetIconIndex(PCIDLIST_ABSOLUTE pidl, const wchar_t* pszPath, DWORD dwFileAttributes, UINT uFlags);

    FILETIME GetModifiedTime(PCIDLIST_ABSOLUTE pidl);


    bool ExecuteFile(PCIDLIST_ABSOLUTE file);

}