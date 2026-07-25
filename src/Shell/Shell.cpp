#include "Shell.h"
#include <Shlwapi.h>
#include "..\Str\Str.h"
// #include "Str.h"
#include "Icons/IconLookup.h"   // Icons::GetIconIndex only — no renderer dependency
#include <wrl/client.h>
#include <PortableDeviceApi.h>
#pragma comment(lib, "PortableDeviceGuids.lib")


using Microsoft::WRL::ComPtr;
using namespace WShell;

static std::wstring GetDefaultValue(HKEY root, const wchar_t* subkey);
Pidl home = GetKnownFolderPidl(L"shell:::{f874310e-b6b7-47dc-bc84-b9e6b38f5903}");

namespace { // Anonymous namespace means these are private to this .cpp file
    
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

    // The universal COM enumeration loop — every "list a folder's children" call
    // in this file goes through here instead of hand-rolling BindToObject/EnumObjects.
    template <typename Func>
    void IterateFolder(PCIDLIST_ABSOLUTE folder, DWORD shcontfFlags, Func&& callback) {
        if (!folder) return;

        ComPtr<IShellFolder> desktop, targetFolder;
        if (FAILED(SHGetDesktopFolder(&desktop))) return;

        if (ILIsEmpty(folder)) {
            targetFolder = desktop;
        } else {
            if (FAILED(desktop->BindToObject(folder, nullptr, IID_PPV_ARGS(&targetFolder)))) return;
        } 

        ComPtr<IEnumIDList> enumerator;
        if (FAILED(targetFolder->EnumObjects(nullptr, shcontfFlags, &enumerator))) return;

        PITEMID_CHILD childPidl = nullptr;
        ULONG fetched = 0;

        while (enumerator->Next(1, &childPidl, &fetched) == S_OK) {
            callback(targetFolder.Get(), childPidl);
            CoTaskMemFree(childPidl);   
        }
    }

    // Combines a parent + child into a freshly-owned Pidl. Was written out by hand
    // (ILCombine(...) wrapped in WShell::Pidl(...)) at every single call site below.
    Pidl CombineChild(PCIDLIST_ABSOLUTE parent, PITEMID_CHILD child) {
        return Pidl(ILCombine(parent, child));
    }

    // The "SHGFI_PIDL | SHGFI_SYSICONINDEX | <size>" combination was repeated
    u64 GetSystemIconKey(PCIDLIST_ABSOLUTE pidl, UINT sizeFlag) {
        return Icons::GetIconIndex(pidl, nullptr, 0, SHGFI_PIDL | SHGFI_SYSICONINDEX | sizeFlag);
    }
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
        
        // item.attributes = 0xFFFFFFFF;
        item.attributes = SFGAO_FOLDER | SFGAO_CANRENAME | SFGAO_CANDELETE;
        pTarget->GetAttributesOf(1, (LPCITEMIDLIST*)&child, &item.attributes);

        item.iconKey = GetSystemIconKey(item.pidl.get(), SHGFI_LARGEICON);
        items.push_back(std::move(item));
    });

    return items;
}

bool WShell::Directory::Load(PCIDLIST_ABSOLUTE folder){
    if (!folder) return false;
    items = WShell::EnumFolder(folder);
    access = WShell::GetFolderAccess(folder);
    selectedIndex = -1;
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
        items.push_back(std::move(item));
    });

    return items;
}
// =======================================

namespace {
    // Shared by every Sidebar::Item construction site below (drives, Recycle Bin,
    // Control Panel, Quick Access) — previously each one rebuilt name/pidl/icon by
    // hand with small, easy-to-miss differences.
    WShell::Sidebar::Item MakeSidebarItem(std::string name, PCIDLIST_ABSOLUTE itemPidl, WShell::Sidebar::Category category){
        WShell::Sidebar::Item item;
        item.name = std::move(name);
        item.pidl = WShell::Pidl(itemPidl);   // clones — caller keeps ownership of itemPidl
        item.hasSubFolder = PidlHasSubFolders(itemPidl);
        item.iconKey = GetSystemIconKey(itemPidl, SHGFI_SMALLICON);
        item.category = category;
        return item;
    }
}

std::vector<WShell::Sidebar::Item> WShell::Sidebar::GetItems(Category cat){
    std::vector<WShell::Sidebar::Item> items;

    if (cat == Category::C3){
        Pidl thisPc = GetKnownFolderPidl(FOLDERID_ComputerFolder);
        // This PC's real drives/containers (skips virtual entries like "Gallery")
        IterateFolder(thisPc, SHCONTF_FOLDERS | SHCONTF_STORAGE | SHCONTF_NAVIGATION_ENUM, [&](IShellFolder* target, PITEMID_CHILD child) {
            SFGAOF attrs = SFGAO_FOLDER | SFGAO_STREAM;
            if (FAILED(target->GetAttributesOf(1, (LPCITEMIDLIST*)&child, &attrs))) return;

            bool isRealContainer = (attrs & SFGAO_FOLDER) && !(attrs & SFGAO_STREAM);
            if (!isRealContainer) return;

            Pidl drivePidl = CombineChild(thisPc.get(), child);
            items.push_back(MakeSidebarItem(GetDisplayName(target, child, SHGDN_NORMAL), drivePidl.get(), Category::C3));
        });

        Pidl recycleBin = GetKnownFolderPidl(FOLDERID_RecycleBinFolder);
        items.push_back(MakeSidebarItem(PidlToTypeablePath(recycleBin.get()), recycleBin.get(), Category::C3));
    }
    else if (cat == Category::C2){
        Pidl quickAccess = GetKnownFolderPidl(L"shell:::{679F85CB-0220-4080-B29B-5540CC05AAB6}");
        
        IterateFolder(quickAccess, SHCONTF_FOLDERS | SHCONTF_NAVIGATION_ENUM, [&](IShellFolder* target, PITEMID_CHILD child) {
            Pidl pinnedPidl = CombineChild(quickAccess.get(), child);
            items.push_back(MakeSidebarItem(GetDisplayName(target, child, SHGDN_NORMAL), pinnedPidl.get(), Category::C2));
        });
    }
    else if (cat == Category::C1){    
        items.push_back(MakeSidebarItem(PidlToTypeablePath(home.get()), home.get(), Category::C1));
        std::vector<Sidebar::Item> accounts = GetOneDriveAccounts();
        for (auto& account : accounts){
            items.push_back(std::move(account));
        }
        Pidl documents = GetKnownFolderPidl(FOLDERID_Documents);
        Pidl downloads = GetKnownFolderPidl(FOLDERID_Downloads);
        items.push_back(MakeSidebarItem(GetDisplayName(documents.get()), documents.get(), Category::C1));
        items.push_back(MakeSidebarItem(GetDisplayName(downloads.get()), downloads.get(), Category::C1));
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

        item.iconKey = Icons::GetIconIndex(nullptr, subKeyName, FILE_ATTRIBUTE_NORMAL, SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
        menuItems.push_back(std::move(item));
    }
    RegCloseKey(hKeyRoot);
    return menuItems;
    
}

std::vector<Sidebar::Item> Sidebar::GetOneDriveAccounts(){
    std::vector<Sidebar::Item> accounts;

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
            Sidebar::Item account;

            account.pidl = TypeablePathToPidl(userFolder);   
            if (account.pidl){
                account.name = GetDisplayName(account.pidl.get());
                account.hasSubFolder = PidlHasSubFolders(account.pidl.get());
                account.iconKey = Icons::GetIconIndex(account.pidl.get(), nullptr, 0, SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
                accounts.push_back(std::move(account));
            }
        }
        RegCloseKey(hKeyAccount);
    }

    RegCloseKey(hKeyRoot);
    return accounts;
}

/*
This PC                             :    10110000000000000000000101110100
Network                             :    10110000000001000000000001100100
Donald's S20 FE                     :    11110000100000000000000001001101
Linux                               :    10100000100000000000000001001101
31, 29, 6, 2

Home                                :    10100000000000000000000000000100
Gallery                             :    00110000100000000000000100000100
Donald - Personal                   :    11110000100000000000000001001101
Somtochukwu - University of Windsor :    11110000100000000000000001001101

This PC                             :    10110000000000000000000101110100
Network                             :    10110000000001000000000001100100
Recycle Bin                         :    00100000000000000000000101010100
Control Panel                       :    10100000000000000000000000100100
Donald Udeh                         :    11110000100000000000000100101101
Libraries                           :    10110000100000000000000100001101
Music                               :    11110000100000000000000001001101
Downloads                           :    11110000100000000000000001001101
Pictures                            :    11110000100000000000000001001101
Control Panel                       :    00000000000000000000000000100100
Videos                              :    11110000100000000000000001001101
Documents                           :    11110000100000000000000001001101
Linux                               :    10100000100000000000000001001101
Desktop                             :    11110000100000000000000001001101
Gallery                             :    00110000100000000000000100000100
Home                                :    10100000000000000000000000000100
Donald - Personal                   :    11110000100000000000000001001101
Somtochukwu - University of Windsor :    11110000100000000000000001001101
Learn about this picture            :    00000000000000000000000000000000
Donald's S20 FE                     :    11110000100000000000000001001101
Arduino IDE                         :    01000000010000010000000101110111
Command Prompt                      :    01000000010000010000000101110111
Desktop                             :    01110000100000000000000101111111
Discord                             :    01000000010000010000000101110111
Dynamic Theme                       :    01000000010000010000000101110111
Firefox.exe                         :    01000000010000000000000101110111
GitHub Desktop                      :    01000000010000010000000101110111
IOLab                               :    01000000010000010000000101110111
JDownloader 2                       :    01000000010000010000000101110111
Old Firefox Data                    :    11110000100000000000000101111111
SignalRgb                           :    01000000010000010000000101110111
Stacher7                            :    01000000010000010000000101110111
udeh - Chrome                       :    01000000010000010000000101110111
Visual Studio Code-Donalds-PC       :    01000000010000010000000101110111
Visual Studio Code                  :    01000000010000010000000101110111
VsCode.code-workspace               :    01000000010000000000000101110111
WECDSB Student AI Hub               :    01000000010000010000000101110111
Accessibility Insights For Windows  :    01000000010000010000000101110111
Adobe Acrobat                       :    01000000010000010000000101110111
AirDroid                            :    01000000010000010000000101110111
Google Chrome                       :    01000000010000010000000101110111
Hasleo Backup Suite                 :    01000000010000010000000101110111
KiCad 10.0                          :    01000000010000010000000101110111
Microsoft Edge                      :    01000000010000010000000101110111
VLC media player                    :    01000000010000010000000101110111
VMware Workstation Pro              :    01000000010000010000000101110111
*/