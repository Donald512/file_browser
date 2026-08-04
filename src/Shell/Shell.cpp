#include "Shell.h"
#include <Shlwapi.h>
#include "Str.h"
#include "Icons.h"  
#include <wrl/client.h>
#include <PortableDeviceApi.h>
#include <shobjidl.h>
#include <propsys.h>
#include <propkey.h>
#include <algorithm>

#pragma comment(lib, "PortableDeviceGuids.lib")
#pragma comment(lib, "propsys.lib")

using Microsoft::WRL::ComPtr;
using namespace WShell;

static std::wstring GetDefaultValue(HKEY root, const wchar_t* subkey);
Pidl home = GetKnownFolderPidl(L"shell:::{f874310e-b6b7-47dc-bc84-b9e6b38f5903}");

namespace { // Anonymous namespace means these are private to this .cpp file
    



}

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


std::vector<Item> WShell::EnumFolder(PCIDLIST_ABSOLUTE folder){
    std::vector<Item> items;
    IterateFolder(folder, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, [&](IShellFolder* pTarget, PITEMID_CHILD child) {
        Item item;
        item.name = GetDisplayName(pTarget, child, SHGDN_NORMAL);
        item.pidl = CombineChild(folder, child);
        item.hash = HashPidl(item.pidl.get());
        
        // item.attributes = 0xFFFFFFFF;
        item.attributes = SFGAO_FOLDER | SFGAO_CANRENAME | SFGAO_CANDELETE;
        pTarget->GetAttributesOf(1, (LPCITEMIDLIST*)&child, &item.attributes);

        bool isFolder = (item.attributes & SFGAO_FOLDER);

        WIN32_FIND_DATAW wfd{};
        if (SUCCEEDED(SHGetDataFromIDListW(pTarget, child, SHGDFIL_FINDDATA, &wfd, sizeof(wfd)))){
            if (!isFolder){
                item.size.value = (static_cast<u64>(wfd.nFileSizeHigh) << 32) | wfd.nFileSizeLow;
                item.size.resolved = true;
            } else{
                item.size.value = 0;
                item.size.resolved = true;
            }
            item.lastWriteTime = wfd.ftLastWriteTime;

            SHFILEINFOW sfi = {};
            DWORD dwAttr = (item.attributes & SFGAO_FOLDER) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
            if (SHGetFileInfoW(wfd.cFileName, dwAttr, &sfi, sizeof(sfi), SHGFI_TYPENAME | SHGFI_USEFILEATTRIBUTES)) {
                item.typeName.value = Str::WideToString(sfi.szTypeName);
                item.typeName.resolved = true;
            }
        }
        // VIRTUAL PATH: If SHGetDataFromIDListW failed (e.g. "This PC"), 
        // item.size and item.typeName remain unresolved (resolved = false) 
        // and will naturally execute their fallback lambdas if accessed.
        items.push_back(std::move(item));
    });

    return items;
}

bool WShell::Directory::Load(PCIDLIST_ABSOLUTE folder){

    if (!folder) return false;
    items = WShell::EnumFolder(folder);
    access = WShell::GetFolderAccess(folder);
    selectedIndex = -1;

    ResortItems();
    return true;
}
// =======================================

std::vector<ItemLite>WShell::GetLiteItems(PCIDLIST_ABSOLUTE folder){
    
    std::vector<ItemLite> items;
    // IterateFolder(folder, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, [&](IShellFolder* pTarget, PITEMID_CHILD child) {
    IterateFolder(folder, SHCONTF_FOLDERS, [&](IShellFolder* pTarget, PITEMID_CHILD child) {
        ItemLite item;
        item.name = GetDisplayName(pTarget, child, SHGDN_NORMAL);
        item.pidl = CombineChild(folder, child);
        item.hash = HashPidl(item.pidl.get());
        items.push_back(std::move(item));
    });

    return items;
}
// =======================================

namespace {
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
}

std::vector<Item> WShell::GetSidebarItems(int category){
    std::vector<Item> items;

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

// =======================================

// todo
// ! USE HASSUBFOLDER attr stuff 

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

// =======================================
// RegQueryValueExW reads a VALUE, this function is just boilerplate that will be used to read the registry
static std::wstring GetDefaultValue(HKEY root, const wchar_t* subkey){
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(root, subkey, 0, KEY_READ, &hKey) != ERROR_SUCCESS) return L"";

    wchar_t buffer[256] = {};
    DWORD size = sizeof(buffer);
    std::wstring result;

    if (RegQueryValueExW(hKey, nullptr, nullptr, nullptr, reinterpret_cast<BYTE*>(buffer), &size) == ERROR_SUCCESS) {
        result = buffer;
    }
    RegCloseKey(hKey);
    return result;
}

// =======================================

std::vector<NewMenuItem> WShell::EnumerateNewMenu(){
    std::vector<NewMenuItem> menuItems;

    HKEY hKeyRoot = nullptr;
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, nullptr, 0, KEY_READ, &hKeyRoot) != ERROR_SUCCESS){
        return menuItems;
    }

    wchar_t subKeyName[256];
    for (DWORD index = 0; ; index++){
        DWORD nameLen = 256;   // reset every iteration, not just once before the loop
        if (RegEnumKeyExW(hKeyRoot, index, subKeyName, &nameLen, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS){
            break;   // ERROR_NO_MORE_ITEMS, or a real error — either way, done
        }

         if (subKeyName[0] != L'.') continue;

        wchar_t shellNewSubkey[280];   // separate buffer — never mutate subKeyName itself
        swprintf_s(shellNewSubkey, L"%s\\ShellNew", subKeyName);

        HKEY hKeyShellNew = nullptr;
        if (RegOpenKeyExW(HKEY_CLASSES_ROOT, shellNewSubkey, 0, KEY_READ, &hKeyShellNew) != ERROR_SUCCESS){
            continue;   // no ShellNew key = doesn't belong in the New menu
        }

        NewMenuItem item;
        item.extension = Str::WideToString(subKeyName);

        wchar_t templateFile[MAX_PATH] = {};
        DWORD templateSize = sizeof(templateFile);
        if (RegQueryValueExW(hKeyShellNew, L"FileName", nullptr, nullptr, (LPBYTE)templateFile, &templateSize) == ERROR_SUCCESS){
            item.action = NewItemAction::FromTemplate;
            item.templatePath = TypeablePathToPidl(templateFile);
        }
        RegCloseKey(hKeyShellNew);

        std::wstring progID = GetDefaultValue(HKEY_CLASSES_ROOT, subKeyName);
        if (!progID.empty()){
            std::wstring friendlyName = GetDefaultValue(HKEY_CLASSES_ROOT, progID.c_str());
            if (!friendlyName.empty()){
                item.displayName = Str::WideToString(friendlyName.c_str());
            }
        }
        if (item.displayName.empty()){
            item.displayName = item.extension;   // fallback so the entry isn't blank
        }

        menuItems.push_back(std::move(item));
    }
    RegCloseKey(hKeyRoot);
    return menuItems;
    
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


void WShell::Directory::ResortItems(){
    std::sort(items.begin(), items.end(), [this](const Item& a, const Item& b){

        // Always keep Folders at the top, regardless of sort mode/direction
        bool aIsFolder = (a.attributes & SFGAO_FOLDER) != 0;
        bool bIsFolder = (b.attributes & SFGAO_FOLDER) != 0;

        if (aIsFolder != bIsFolder){
            return aIsFolder;
        }

        // compare based on selected sortmode
        int cmp = 0;
        switch (sortMode) {
            case SortMode::Name:{
                // _stricmp does a case-insensitive ASCII comparison.
                cmp = _stricmp(a.name.c_str(), b.name.c_str());
            }
                break;

            case SortMode::DateModified:{
                // CompareFileTime returns -1, 0, or 1
                cmp = ::CompareFileTime(&a.lastWriteTime, &b.lastWriteTime);
            }
                break;

            case SortMode::Type:{
                // cmp = _stricmp(a.TypeName().c_str(), b.TypeName().c_str());
                // Delibrately does not call .TypeName() here, those lazily resolve on first call, a sychronous shell call from inside a sort comparator. Unresolved items like virtual items that EnumFolder couldn't resolve up front just compare as "" until something  E.g: (a visible row's) WShell::Async::RequestMeta call) resolves them for real; sorting again afterward will then reflect the real type name.
                std::string aType = a.typeName.resolved ? a.typeName.value : std::string();
                std::string bType = b.typeName.resolved ? b.typeName.value : std::string();
                cmp = _stricmp(aType.c_str(), bType.c_str());
            }
                break;

            case SortMode::Size:{
                // if (a.Size() < b.Size()) cmp = -1;
                // else if (a.Size() > b.Size()) cmp = 1;
                // read the resolved value rather than call the lazy size() 
                u64 aSize = a.size.resolved ? a.size.value : 0ULL;
                u64 bSize = b.size.resolved ? b.size.value : 0ULL;
                if (aSize < bSize) cmp = -1;
                else if (aSize > bSize) cmp = 1;
            }
            break;
    
        }

        //  Strict Weak Ordering: If the primary condition is a tie, fallback to Name
        if (cmp == 0) {
            cmp = _stricmp(a.name.c_str(), b.name.c_str());
        }

        if (sortDirection == SortDirection::Descending){
            return cmp > 0;
        }
        return cmp < 0;
    });
}