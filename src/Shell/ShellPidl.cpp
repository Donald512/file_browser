#include "Shell.h"
#include <Shlwapi.h>


using namespace WShell;

Pidl WShell::GetKnownFolderPidl(REFKNOWNFOLDERID folderID){
    PIDLIST_ABSOLUTE pidl = nullptr;
        SHGetKnownFolderIDList(folderID, 0, NULL, &pidl);
    return Pidl(pidl);
    }
Pidl WShell::GetKnownFolderPidl(const wchar_t* shellParsingGuid){
    PIDLIST_ABSOLUTE pidl = nullptr;
    SHParseDisplayName(shellParsingGuid, NULL, &pidl, 0, NULL);
    return Pidl(pidl);
}

WShell::Pidl WShell::TypeablePathToPidl(const wchar_t* widePath){

   WShell::Pidl pidl(nullptr);
    DWORD attrs = 0;

    // Try as a standard path or GUID Parsing Name (e.g. "C:\Windows" or "::{GUID}")
    if (SUCCEEDED(::SHParseDisplayName(widePath, nullptr, pidl.GetAddressOf(), 0, &attrs))){
        return pidl;
    }

    // if it failed, try searching the Desktop. (Catches "This PC", "Recycle Bin", "Linux", custom virtual folders)
    // TRIAL 2:
    ComPtr<IShellFolder> pDesktop;
    if (SUCCEEDED(::SHGetDesktopFolder(&pDesktop))) {
        ComPtr<IEnumIDList> pEnum;
        if (SUCCEEDED(pDesktop->EnumObjects(nullptr, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &pEnum))) {
            LPITEMIDLIST childPidl = nullptr;
            ULONG fetched = 0;
            
            while (pEnum->Next(1, &childPidl, &fetched) == S_OK) {
                STRRET strName;
                if (SUCCEEDED(pDesktop->GetDisplayNameOf(childPidl, SHGDN_NORMAL, &strName))) {
                    wchar_t nameBuf[MAX_PATH];
                    StrRetToBufW(&strName, childPidl, nameBuf, MAX_PATH);

                    if (_wcsicmp(nameBuf, widePath) == 0) {
                        pidl =WShell::Pidl(ILClone(childPidl)); 
                        CoTaskMemFree(childPidl); 
                        break;
                    }
                }
                CoTaskMemFree(childPidl);
            }
        }
        if (pidl) return pidl; 
    }

    // TRIAL 3:
    ComPtr<IKnownFolderManager> pManager;
    if (SUCCEEDED(CoCreateInstance(CLSID_KnownFolderManager, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pManager)))) {
        UINT count = 0;
        KNOWNFOLDERID* pIds = nullptr;
        
        if (SUCCEEDED(pManager->GetFolderIds(&pIds, &count))) {
            for (UINT i = 0; i < count; i++) {  
                ComPtr<IKnownFolder> pFolder;
                if (SUCCEEDED(pManager->GetFolder(pIds[i], &pFolder))) {
                    ComPtr<IShellItem> pItem;
                    if (SUCCEEDED(pFolder->GetShellItem(0, IID_PPV_ARGS(&pItem)))) {
                        LPWSTR pName = nullptr;
                        if (SUCCEEDED(pItem->GetDisplayName(SIGDN_NORMALDISPLAY, &pName))) {
                            if (_wcsicmp(pName, widePath) == 0) {
                                SHGetKnownFolderIDList(pIds[i], 0, NULL, pidl.GetAddressOf());
                            }
                            CoTaskMemFree(pName);
                        }
                    }
                }
                if (pidl) break;
            }
            CoTaskMemFree(pIds);
        }
        if (pidl) return pidl; 
    }
    return pidl;

}


std::string WShell::PidlToTypeablePath(PCIDLIST_ABSOLUTE pidl){ 
    if (!pidl || pidl->mkid.cb == 0) {
        // If it's the root Desktop, just return "Desktop"
        return "Desktop";
    }

    auto getutf8AndFreeWideStr = [](wchar_t* &pAllocatedPath){
        // does not copy the pointer
        std::string path = Str::WideToString(pAllocatedPath);
        CoTaskMemFree(pAllocatedPath);
        return path;
    };

    wchar_t* pAllocatedPath = nullptr;
    // Try to get a real file system path (e.g., "C:\Users\Documents")
    if (SUCCEEDED(SHGetNameFromIDList(pidl, SIGDN_FILESYSPATH, &pAllocatedPath))) {
        return getutf8AndFreeWideStr(pAllocatedPath);
    }

    // If it's a known folder or virtual shortcut, get the friendly parsing name (e.g., "Documents")
    if (SUCCEEDED(SHGetNameFromIDList(pidl, SIGDN_DESKTOPABSOLUTEPARSING, &pAllocatedPath))) {
        // Check if Windows handed us a nasty GUID string (starts with "::")
        if (pAllocatedPath[0] == L':' && pAllocatedPath[1] == L':') {
            CoTaskMemFree(pAllocatedPath);
            pAllocatedPath = nullptr;
            
            // It's a pure virtual root like "This PC". Get its display name instead.
            if (SUCCEEDED(SHGetNameFromIDList(pidl, SIGDN_NORMALDISPLAY, &pAllocatedPath))) {
                return getutf8AndFreeWideStr(pAllocatedPath);
            }
        } 
        else {
            return getutf8AndFreeWideStr(pAllocatedPath);
        }
    }
    return std::string{};
}


u64 WShell::HashPidl(PCIDLIST_ABSOLUTE pidl){
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

// Combines a parent + child into a freshly-owned Pidl. Was written out by hand
// (ILCombine(...) wrapped in WShell::Pidl(...)) at every single call site below.
Pidl CombineChild(PCIDLIST_ABSOLUTE parent, PITEMID_CHILD child){
    return Pidl(ILCombine(parent, child));
}

// The "SHGFI_PIDL | SHGFI_SYSICONINDEX | <size>" combination was repeated
u32 GetSystemIconKey(PCIDLIST_ABSOLUTE pidl, UINT sizeFlag) {
    return Icons::GetIconIndex(pidl, nullptr, 0, SHGFI_PIDL | SHGFI_SYSICONINDEX | sizeFlag);
}