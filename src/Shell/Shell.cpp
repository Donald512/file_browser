#include "Shell.h"
#include <Shlwapi.h>
#include "..\Str\Str.h"
// #include "Str.h"
#include "..\Icons\icons.h"
#include <wrl/client.h>


using Microsoft::WRL::ComPtr;
using namespace WShell;

bool WShell::Directory::Load(PCIDLIST_ABSOLUTE folder){
    if (!folder) return false;
    items.clear();

    ComPtr<IShellFolder> pDesktop;
    ComPtr<IShellFolder> pTargetFolder;

    //  Fetch Desktop Root
    if (FAILED(SHGetDesktopFolder(&pDesktop))) {
        return false;
    }

    //  Bind to the target folder or mirror the desktop
    if (ILIsEmpty(folder)) {
        pTargetFolder = pDesktop; // ComPtr automatically calls AddRef() under the hood
    }
    else {
        // IID_PPV_ARGS automatically passes the correct IID interface GUID and casts the pointer type safely
        if (FAILED(pDesktop->BindToObject(folder, nullptr, IID_PPV_ARGS(&pTargetFolder)))) {
            return false; 
        }
    } // pDesktop is no longer needed after this point;

    //  Enumerate Objects
    ComPtr<IEnumIDList> pEnum;
    if (FAILED(pTargetFolder->EnumObjects(nullptr, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &pEnum))) {
        return false; 
    }

    PITEMID_CHILD childPidl = nullptr; // Using standard modern Windows naming for child PIDLs
    ULONG fetched = 0;

    while(pEnum->Next(1, &childPidl, &fetched) == S_OK){
        Item item;

        STRRET strName;
        if (SUCCEEDED(pTargetFolder->GetDisplayNameOf(childPidl, SHGDN_NORMAL, &strName))) {
            wchar_t nameBuffer[MAX_PATH] = {};
            StrRetToBufW(&strName, childPidl, nameBuffer, MAX_PATH);
            item.name = Str::WideToString(nameBuffer);
        }
        
        // todo, check if Folder and if it is, get the extension and use FIlEATTRIBUTES
        item.attributes = SFGAO_FOLDER | SFGAO_CANRENAME | SFGAO_CANDELETE;
        pTargetFolder->GetAttributesOf(1, (LPCITEMIDLIST*)&childPidl, &item.attributes);


        item.pidl =WShell::Pidl(ILCombine(folder, childPidl));

        UINT iconFlags = SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_LARGEICON;
        item.iconCacheKey = Icons::GetIconIndex(item.pidl.get(), 0, 0, iconFlags);

        items.push_back(std::move(item));
        CoTaskMemFree(childPidl);   
    }

    access =WShell::GetFolderAccess(folder); 
    printf("capacity: %zu, size: %zu\n", items.capacity(), items.size());
    return true;
}

// =======================================

std::vector<ItemLite>WShell::GetLiteItems(PCIDLIST_ABSOLUTE folder){
    std::vector<ItemLite> items;

    if (!folder) return items;

    ComPtr<IShellFolder> pDesktop;
    ComPtr<IShellFolder> pTargetFolder;
    //  Fetch Desktop Root
    if (FAILED(SHGetDesktopFolder(&pDesktop))) {
        return items;
    }

    //  Bind to the target folder or mirror the desktop
    if (ILIsEmpty(folder)) {
        pTargetFolder = pDesktop; // ComPtr automatically calls AddRef() under the hood
    }
    else {
        // IID_PPV_ARGS automatically passes the correct IID interface GUID and casts the pointer type safely
        if (FAILED(pDesktop->BindToObject(folder, nullptr, IID_PPV_ARGS(&pTargetFolder)))) {
            return items; 
        }
    } // pDesktop is no longer needed after this point;

    //  Enumerate Objects
    ComPtr<IEnumIDList> pEnum;
    if (FAILED(pTargetFolder->EnumObjects(nullptr, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &pEnum))) {
        return items; 
    }

    PITEMID_CHILD childPidl = nullptr; // Using standard modern Windows naming for child PIDLs
    ULONG fetched = 0;

    while (pEnum->Next(1, &childPidl, &fetched) == S_OK){ 
        ItemLite item;

        STRRET strName;
        if (SUCCEEDED(pTargetFolder->GetDisplayNameOf(childPidl, SHGDN_NORMAL, &strName))) {
            wchar_t nameBuffer[MAX_PATH] = {};
            StrRetToBufW(&strName, childPidl, nameBuffer, MAX_PATH);
            item.name = Str::WideToString(nameBuffer);
            
        }
        item.pidl =WShell::Pidl(ILCombine(folder, childPidl));

        items.push_back(std::move(item));
        CoTaskMemFree(childPidl);
    }
    return items;
}

// =======================================

bool WShell::ExecuteFile(PCIDLIST_ABSOLUTE file){
    if (!file) return false;

    SHELLEXECUTEINFOW sei = {};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_IDLIST | SEE_MASK_ASYNCOK;
    sei.lpIDList = const_cast<PIDLIST_ABSOLUTE>(file);
    sei.nShow = SW_SHOWNORMAL;

    if (!::ShellExecuteExW(&sei)){
        // todo handle error, eg Access Denied, or No app associated
        DWORD err = GetLastError();
        printf("Failed to launch item. Error: %lu\n", err);
        return false;
    }
    return true;
}

// =======================================

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

// =======================================

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

// =======================================

// todo
// ! USE HASSUBFOLDER attr stuff 
bool WShell::PidlHasSubFolders(PCIDLIST_ABSOLUTE folder){
    if (!folder) return false;

    bool hasSubFolders = false;
    ComPtr<IShellItem> pParentItem;
    
    if (FAILED(SHCreateItemFromIDList(folder, IID_PPV_ARGS(&pParentItem)))) return hasSubFolders;
    
    // check if folder, only folders can have subfolders
    SFGAOF attr = 0;
    if (FAILED(pParentItem->GetAttributes(SFGAO_FOLDER, &attr)) || !(attr & SFGAO_FOLDER)) return hasSubFolders;
    
    ComPtr<IEnumShellItems> pEnum;
    if (FAILED(pParentItem->BindToHandler(nullptr, BHID_EnumItems, IID_PPV_ARGS(&pEnum)))) return hasSubFolders;
    
    ComPtr<IShellItem> pChild;
    ULONG fetched = 0; 

    // exit once a folder is found
    while (pEnum->Next(1, &pChild, &fetched) == S_OK && fetched == 1){
        SFGAOF childAttr = 0;
        if (SUCCEEDED(pChild->GetAttributes(SFGAO_FOLDER, &childAttr))){
            if (childAttr & SFGAO_FOLDER){
                hasSubFolders = true;
                break;
            }
        }
    }
    return hasSubFolders;
}

// =======================================

FolderAccess WShell::GetFolderAccess(PCIDLIST_ABSOLUTE folder){
    if (!folder) return FolderAccess::NoCreate;
    
    // 1. Physical path check (Handles mapped folders, redirects, & templates)
    // This catches read-only drives, system folders
    wchar_t* pszPath = nullptr;
    if (SUCCEEDED(SHGetNameFromIDList(folder, SIGDN_FILESYSPATH, &pszPath))){
        DWORD dwAttrib = GetFileAttributesW(pszPath);
        if (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY)){
            // Check if the directory is marked read-only on the file system level
            if (dwAttrib & FILE_ATTRIBUTE_READONLY) {
                CoTaskMemFree(pszPath);
                return FolderAccess::Restricted; // Allow browsing/basic viewing, but restrict creation
            }

            // todo check if Quick write-permission check using CreateFileW is neccesary
            CoTaskMemFree(pszPath);
            return FolderAccess::FullAccess;
        }
        CoTaskMemFree(pszPath);
        return FolderAccess::NoCreate;
    }

    // 2. Fallback for Virtual/Shell Folders (OneDrive, Libraries, Control Panel, etc.)
    // These don't have standard physical paths, so we check Shell attributes instead.
    ComPtr<IShellFolder> pParentFolder;
    PCUITEMID_CHILD pidlChild = nullptr;

    HRESULT hr = SHBindToFolderIDListParent(nullptr, folder, IID_PPV_ARGS(&pParentFolder), &pidlChild);
    if (FAILED(hr)) return FolderAccess::NoCreate;
    
    ULONG attributes = SFGAO_FILESYSTEM | SFGAO_FILESYSANCESTOR | SFGAO_STORAGE | SFGAO_STREAM;
    hr = pParentFolder->GetAttributesOf(1, &pidlChild, &attributes);

    // If it has physical or storage shell attributes, it a real place on disk
    if (SUCCEEDED(hr) && ((attributes & SFGAO_FILESYSTEM) || (attributes & SFGAO_FILESYSANCESTOR))) {
        // "This PC" and "Network" are FILESYSANCESTOR, but they do NOT have SFGAO_STORAGE or SFGAO_STREAM.
        // ZIP folders and OneDrive folders WILL have SFGAO_STORAGE/SFGAO_STREAM along with file system flags.
        bool isFileSystem = (attributes & SFGAO_FILESYSTEM);
        bool isStorageContainer = (attributes & (SFGAO_STORAGE | SFGAO_STREAM));
        if (isFileSystem && isStorageContainer) return FolderAccess::FullAccess;
        
        // If it's a file system ancestor (like the Desktop root itself), we can allow it
        // ONLY if it's not a pure virtual root folder like "This PC".
        if ((attributes & SFGAO_FILESYSANCESTOR) && isStorageContainer) return FolderAccess::FullAccess; 
    }
    // otherwise its a purely virtual namespace (like "This PC" root or "Network")
    return FolderAccess::NoCreate;
}