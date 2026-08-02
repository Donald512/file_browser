#include "Shell.h"
#include <Shlwapi.h>
#include <propkey.h>
#include "ShellInternal.h"


#pragma comment(lib, "propsys.lib")


using namespace WShell;

Pidl home = GetKnownFolderPidl(L"shell:::{f874310e-b6b7-47dc-bc84-b9e6b38f5903}");

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


bool WShell::PidlHasSubFolders(PCIDLIST_ABSOLUTE folder, bool accurate){
    if (!folder) return false;
    if (ILIsEqual(folder, home)) return false;
    // memoize, map<Pidl, bool>

    bool hasSubFolders = false;
    ComPtr<IShellItem> pParentItem;
    
    if (FAILED(SHCreateItemFromIDList(folder, IID_PPV_ARGS(&pParentItem)))) return hasSubFolders;
    
    // check if folder, only folders can have subfolders
    SFGAOF attr = 0;
    if (accurate){
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
    }
    else{
        if (SUCCEEDED(pParentItem->GetAttributes(SFGAO_HASSUBFOLDER, &attr))) {
            return (attr & SFGAO_HASSUBFOLDER) != 0;
        }
        return false;
        
    }
    
    return hasSubFolders;
}



std::string WShell::GetPidlTypeName(PCIDLIST_ABSOLUTE pidl) {
    if (!pidl) return "";

    SHFILEINFOW sfi = {};
    // Passing SHGFI_PIDL | SHGFI_TYPENAME asks Windows to return 
    // the localized display string (e.g., "File folder", "Application", "Text Document")
    if (SHGetFileInfoW(reinterpret_cast<LPCWSTR>(pidl), 0, &sfi, sizeof(sfi), SHGFI_PIDL | SHGFI_TYPENAME)){
        return Str::WideToString(sfi.szTypeName);
    }

    return "";
}


u64 WShell::GetPidlFileSize(PCIDLIST_ABSOLUTE pidl) {
    if (!pidl) return 0;

    // 1. FAST PATH: Physical disk file
    wchar_t* szPath = nullptr;
    if (SUCCEEDED(SHGetNameFromIDList(pidl, SIGDN_FILESYSPATH, &szPath))) {
        WIN32_FILE_ATTRIBUTE_DATA fileData = {};
        u64 size = 0;
        if (GetFileAttributesExW(szPath, GetFileExInfoStandard, &fileData)) {
            size = (static_cast<u64>(fileData.nFileSizeHigh) << 32) | fileData.nFileSizeLow;
        }
        CoTaskMemFree(szPath);
        return size;
    }
    return 0;
}



std::string WShell::FetchWindowsTooltip(PCIDLIST_ABSOLUTE pidl){
    std::string tooltipStr = "";
    
    // Create an IShellItem from the PIDL
    IShellItem* pItem = nullptr;
    if (SUCCEEDED(SHCreateItemFromIDList(pidl, IID_PPV_ARGS(&pItem)))) {
        
        // Ask the Shell for the UI object that handles InfoTips (Tooltips)
        IQueryInfo* pQueryInfo = nullptr;
        if (SUCCEEDED(pItem->BindToHandler(NULL, BHID_SFUIObject, IID_PPV_ARGS(&pQueryInfo)))) {
            
            wchar_t* pwszTip = nullptr;
            
            // QITIPF_DEFAULT gets standard properties. 
            // QITIPF_USENAME includes the filename at the very top (like your 3rd screenshot).
            if (SUCCEEDED(pQueryInfo->GetInfoTip(QITIPF_DEFAULT, &pwszTip)) && pwszTip) {

                std::wstring wstr(pwszTip);
                wstr.erase(std::remove_if(wstr.begin(), wstr.end(), [](wchar_t c) {
                    return c == L'\r' || c == 0x200E || c == 0x200F || c == 0x202A || c == 0x202B || c == 0x202C;
                }), wstr.end());

                tooltipStr = Str::SanitizeWString(wstr.c_str());
                
                CoTaskMemFree(pwszTip);
            }
            pQueryInfo->Release();
        }
        pItem->Release();
    }
    
    return tooltipStr;
}



std::string WShell::FetchTileViewLines(PCIDLIST_ABSOLUTE pidl) {
    std::string result = "";
    
    // Declarations for COM pointers and allocations so cleanup is straightforward
    IShellItem2* pItem2 = nullptr;
    wchar_t* pwszPropList = nullptr;
    IPropertyDescriptionList* pDescList = nullptr;
    IPropertyStore* pStore = nullptr;

    // 1. Create ShellItem2
    if (FAILED(SHCreateItemFromIDList(pidl, IID_PPV_ARGS(&pItem2)))) {
        return result;
    }

    // 2. Fetch the TileInfo property list string from registry
    if (FAILED(pItem2->GetString(PKEY_PropList_TileInfo, &pwszPropList)) || !pwszPropList) {
        // FALLBACK: If the registry doesn't specify TileInfo for this file type,
        // you could query default properties (e.g., PKEY_ItemTypeText / PKEY_Size) here.
        goto Cleanup;
    }

    // 3. Parse into Description List
    if (FAILED(PSGetPropertyDescriptionListFromString(pwszPropList, IID_PPV_ARGS(&pDescList)))) {
        goto Cleanup;
    }

    // 4. Bind to Property Store
    if (FAILED(pItem2->GetPropertyStore(GPS_DEFAULT, IID_PPV_ARGS(&pStore)))) {
        goto Cleanup;
    }

    UINT count = 0;
    if (FAILED(pDescList->GetCount(&count))) {
        goto Cleanup;
    }

    // 5. Loop through properties and append to the final string
    for (UINT i = 0; i < count; i++) {
        IPropertyDescription* pDesc = nullptr;
        if (FAILED(pDescList->GetAt(i, IID_PPV_ARGS(&pDesc)))) {
            continue;
        }

        PROPERTYKEY pkey;
        if (SUCCEEDED(pDesc->GetPropertyKey(&pkey))) {
            PROPVARIANT propvar;
            PropVariantInit(&propvar);

            if (SUCCEEDED(pStore->GetValue(pkey, &propvar))) {
                wchar_t* pwszDisplay = nullptr;

                if (SUCCEEDED(pDesc->FormatForDisplay(propvar, PDFF_DEFAULT, &pwszDisplay)) && pwszDisplay) {
                    
                    std::string cleanLine = Str::SanitizeWString(pwszDisplay);

                    if (!cleanLine.empty()){
                        if (!result.empty()) {
                            result += '\n';
                        }
                        result += cleanLine;
                    }
                    CoTaskMemFree(pwszDisplay);
                }
            }
            PropVariantClear(&propvar);
        }
        pDesc->Release();
    }

// Single cleanup site keeps resource leaks from creeping in
Cleanup:
    if (pStore)        pStore->Release();
    if (pDescList)     pDescList->Release();
    if (pwszPropList)  CoTaskMemFree(pwszPropList);
    if (pItem2)        pItem2->Release();

    return result;
}


void WShell::FileTime(const FILETIME& ft, char* outBuf, int outBufSize) {
    // Zeroed FILETIME means "no timestamp" (e.g. drive roots) — don't even try to convert it.
    if (ft.dwLowDateTime == 0 && ft.dwHighDateTime == 0) {
        if (outBufSize > 0) outBuf[0] = '\0';
        return;
    }

    FILETIME localFt;
    SYSTEMTIME st;
    if (!::FileTimeToLocalFileTime(&ft, &localFt) || !::FileTimeToSystemTime(&localFt, &st)) {
        if (outBufSize > 0) outBuf[0] = '\0';
        return;
    }

    char dateBuf[32] = {};
    char timeBuf[32] = {};
    ::GetDateFormatA(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, nullptr, dateBuf, sizeof(dateBuf));
    ::GetTimeFormatA(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, nullptr, timeBuf, sizeof(timeBuf));

    ::sprintf_s(outBuf, outBufSize, "%s %s", dateBuf, timeBuf);
}

// Converts uint64_t size directly into human readable UTF-8 buffer (e.g., "14.2 MB")
void WShell::Size(u64 sizeInBytes, char* outBuf, int outBufSize) {
    if (!outBuf || outBufSize <= 0) return;
    outBuf[0] = '\0';
    wchar_t wbuf[32] = {};
    ::StrFormatByteSizeW(static_cast<LONGLONG>(sizeInBytes), wbuf, 32);
    ::WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, outBuf, outBufSize, nullptr, nullptr);
}


// Shared by every Sidebar::Item construction site below (drives, Recycle Bin,
// Control Panel, Quick Access) — previously each one rebuilt name/pidl/icon by
// hand with small, easy-to-miss differences.
ItemLite MakeSidebarItem(std::string name, PCIDLIST_ABSOLUTE itemPidl){
    ItemLite item;
    item.name = std::move(name);
    item.pidl = WShell::Pidl(itemPidl);   // clones — caller keeps ownership of itemPidl
    item.hash = HashPidl(item.pidl.get());
    return item;
}

std::vector<ItemLite> WShell::GetSidebarItems(int category){
    std::vector<ItemLite> items;

    if (category == 3){
        Pidl thisPc = GetKnownFolderPidl(FOLDERID_ComputerFolder);
        // This PC's real drives/containers (skips virtual entries like "Gallery")
        IterateFolder(thisPc, SHCONTF_FOLDERS | SHCONTF_STORAGE | SHCONTF_NAVIGATION_ENUM, [&](IShellFolder* target, PITEMID_CHILD child) {
            SFGAOF attrs = SFGAO_FOLDER | SFGAO_STREAM;
            if (FAILED(target->GetAttributesOf(1, (LPCITEMIDLIST*)&child, &attrs))) return;

            bool isRealContainer = (attrs & SFGAO_FOLDER) && !(attrs & SFGAO_STREAM);
            if (!isRealContainer) return;

            Pidl drivePidl = CombineChild(thisPc.get(), child);
            items.push_back(MakeSidebarItem(GetDisplayName(target, child, SHGDN_NORMAL), drivePidl.get()));
        });

        Pidl recycleBin = GetKnownFolderPidl(FOLDERID_RecycleBinFolder);
        items.push_back(MakeSidebarItem(PidlToTypeablePath(recycleBin.get()), recycleBin.get()));
    }
    else if (category == 2){
        Pidl quickAccess = GetKnownFolderPidl(L"shell:::{679F85CB-0220-4080-B29B-5540CC05AAB6}");
        
        IterateFolder(quickAccess, SHCONTF_FOLDERS | SHCONTF_NAVIGATION_ENUM, [&](IShellFolder* target, PITEMID_CHILD child) {
            Pidl pinnedPidl = CombineChild(quickAccess.get(), child);
            items.push_back(MakeSidebarItem(GetDisplayName(target, child, SHGDN_NORMAL), pinnedPidl.get()));
        });
    }
    else if (category == 1){    
        items.push_back(MakeSidebarItem(PidlToTypeablePath(home.get()), home.get()));
        std::vector<ItemLite> accounts = GetOneDriveAccounts();
        for (auto& account : accounts){
            items.push_back(std::move(account));
        }
        Pidl documents = GetKnownFolderPidl(FOLDERID_Documents);
        Pidl downloads = GetKnownFolderPidl(FOLDERID_Downloads);
        items.push_back(MakeSidebarItem(GetDisplayName(documents.get()), documents.get()));
        items.push_back(MakeSidebarItem(GetDisplayName(downloads.get()), downloads.get()));
    }

    return items;
}


std::vector<ItemLite> WShell::GetOneDriveAccounts(){
    std::vector<ItemLite> accounts;

    HKEY hKeyRoot = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\OneDrive\\Accounts", 0, KEY_READ, &hKeyRoot) != ERROR_SUCCESS){
        return accounts;   // OneDrive not installed/configured — not an error, just nothing to show
    }
    wchar_t subKeyName[256];
    for (DWORD index = 0; ; index++){
        DWORD nameLen = 256;   // reset every iteration — same bug you caught in EnumerateNewMenu
        if (RegEnumKeyExW(hKeyRoot, index, subKeyName, &nameLen, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS){
            break;
        }
        HKEY hKeyAccount = nullptr;
        if (RegOpenKeyExW(hKeyRoot, subKeyName, 0, KEY_READ, &hKeyAccount) != ERROR_SUCCESS){
            continue;
        }
        wchar_t userFolder[MAX_PATH] = {};
        DWORD folderSize = sizeof(userFolder);
        LONG folderResult = RegQueryValueExW(hKeyAccount, L"UserFolder", nullptr, nullptr, (LPBYTE)userFolder, &folderSize);
        if (folderResult == ERROR_SUCCESS && userFolder[0] != L'\0'){
            ItemLite account;

            account.pidl = TypeablePathToPidl(userFolder);   
            if (account.pidl){
                account.name = GetDisplayName(account.pidl.get());
                accounts.push_back(std::move(account));
            }
        }
        RegCloseKey(hKeyAccount);
    }

    RegCloseKey(hKeyRoot);
    return accounts;
}


// Extracts a child's display name cleanly.
std::string GetDisplayName(IShellFolder* folder, PITEMID_CHILD child, SHGDNF flags) {
    STRRET strName;
    if (SUCCEEDED(folder->GetDisplayNameOf(child, flags, &strName))) {
        wchar_t nameBuffer[MAX_PATH] = {};
        StrRetToBufW(&strName, child, nameBuffer, MAX_PATH);
        return Str::WideToString(nameBuffer);
    }
    return "";
}
std::string GetDisplayName(PCIDLIST_ABSOLUTE pidl ) {
    wchar_t* niceName = nullptr;
    if (SUCCEEDED(SHGetNameFromIDList(pidl, SIGDN_NORMALDISPLAY, &niceName))){
        std::string name = Str::WideToString(niceName);
        CoTaskMemFree(niceName);
        return name;
    }
    return "";
}