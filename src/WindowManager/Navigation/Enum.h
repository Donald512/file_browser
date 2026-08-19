#pragma once

#include <ShlObj.h>
#include "Pidl.h"
#include <vector>
#include "Str.h"
#include "Shell.h"
#include "Item.h"
#include "BasicTypes.h"
#include "Types/global.h"
#include <iostream>
#include <propkey.h>
#include <propvarutil.h>

#include <wrl/client.h>
using Microsoft::WRL::ComPtr;



/* todo
IterateFolder(safeFolder, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN,
    [&](IShellFolder* pTarget, PITEMID_CHILD child) {
        DirItem item;
        
        // 1. FAST NAME: Query local to the folder, avoiding absolute parsing
        item.name = WShell::GetDisplayName(pTarget, child, SHGDN_INFOLDER | SHGDN_FORPARSING);
        
        // 2. LAZY PIDL: Only do ILCombine if your architecture absolutely requires absolute PIDLs here
        item.pidl = WShell::Pidl(ILCombine(safeFolder, child)); 
        item.hash = HashPidl(item.pidl.get());

        // 3. SEED ATTRIBUTES: Pass a pre-populated mask to avoid pulling extra expensive columns
        item.attributes = SFGAO_FOLDER | SFGAO_CANRENAME | SFGAO_CANDELETE;
        pTarget->GetAttributesOf(1, (LPCITEMIDLIST*)&child, &item.attributes);

        WIN32_FIND_DATAW wfd{};
        if (SUCCEEDED(SHGetDataFromIDListW(pTarget, child, SHGDFIL_FINDDATA, &wfd, sizeof(wfd)))){
            item.size = (static_cast<u64>(wfd.nFileSizeHigh) << 32) | wfd.nFileSizeLow;
            item.lastWriteTime = wfd.ftLastWriteTime;

            // 4. BLAZING FAST METADATA (No disk IO / No Shell Extension hang)
            // By using SHGFI_USEFILEATTRIBUTES, SHGetFileInfo matches the extension 
            // against the registry in-memory, completely ignoring the disk/recycle bin state.
            SHFILEINFOW sfi = {};
            DWORD dwAttr = (item.attributes & SFGAO_FOLDER) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
            
            // Pass the string extension or wfd.cFileName instead of the PIDL!
            if (SHGetFileInfoW(wfd.cFileName, dwAttr, &sfi, sizeof(sfi), 
                               SHGFI_USEFILEATTRIBUTES | SHGFI_TYPENAME)) {
                item.typeName = Str::WideToString(sfi.szTypeName);
            }
        }
        items.push_back(std::move(item));
    });

*/



template <typename Func>
// ! DO NOT delete this, sidebarEnum needs this
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

template <typename Func>
void IterateFolder(IShellFolder* pFolder, DWORD shcontfFlags, Func&& callback) {
    if (!pFolder) return;

    ComPtr<IEnumIDList> enumerator;
    if (FAILED(pFolder->EnumObjects(nullptr, shcontfFlags, &enumerator))) return;

    PITEMID_CHILD childPidl = nullptr;
    ULONG fetched = 0;

    while (enumerator->Next(1, &childPidl, &fetched) == S_OK) {
        callback(pFolder, childPidl);
        
        if(childPidl){ // if the lambda stole it, pChild will be null
            CoTaskMemFree(childPidl);   
        }
    }
}



std::vector<DirItem> EnumFolder(const std::wstring dirPath){
    std::wstring searchPath = dirPath; 
    if (searchPath.back() != L'\\' && searchPath.back() != L'/') {  // check if it already contains a backslash e.g: C:/ or C:\ before adding /
        searchPath += L"\\";
    }
    
    searchPath += L"*";     // Build search path with wildcard (e.g., L"C:\\Windows\\*")
    WIN32_FIND_DATAW findData{};

    HANDLE hFind = FindFirstFileExW( searchPath.c_str(), FindExInfoBasic, &findData, FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH);

    if (hFind == INVALID_HANDLE_VALUE){
        std::cout << "Failed to open directory. Error: " << GetLastError() << std::endl; 
    }

    std::vector<DirItem> result;
    do {
        // skip dummy "." and ".." parent dir entries
        if (wcscmp(findData.cFileName, L".") == 0 || wcscmp(findData.cFileName, L"..") == 0){
            continue;
        }

        DirItem item;
        item.attributes = findData.dwFileAttributes;
        item.name = Str::WideToString(findData.cFileName);
        item.size = (static_cast<u64>(findData.nFileSizeHigh) << 32) | findData.nFileSizeLow;
        item.lastWriteTime = findData.ftLastWriteTime; // sus

        // no typename, i was doing it wrong all along
    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);
    return result;
}
std::vector<DirItem> EnumFolder(PCIDLIST_ABSOLUTE folder, DirItem* parentItem = nullptr){
    std::vector<DirItem> items;

    PCIDLIST_ABSOLUTE safeFolder = folder; // default: use the param as-is

    if (parentItem){
        ComPtr<IShellFolder> desktop, targetFolder;
        WShell::Pidl folderClone = WShell::Pidl(ILClone(folder));
        PCIDLIST_ABSOLUTE folderClonePidl = folderClone.get();

        if (SUCCEEDED(SHGetDesktopFolder(&desktop))) {
            if (ILIsEmpty(folderClonePidl)) {
                targetFolder = desktop;
            } else {
                desktop->BindToObject(folderClone, nullptr, IID_PPV_ARGS(&targetFolder));
            }
        }

        if (targetFolder){
            parentItem->pidl = std::move(folderClone); 
            safeFolder = parentItem->pidl.get(); 
            parentItem->hash = HashPidl(safeFolder);
            parentItem->name = WShell::GetDisplayName(safeFolder);
        }
    }

    IterateFolder(safeFolder, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN,
        [&](IShellFolder* pTarget, PITEMID_CHILD child) {
            DirItem item;
            item.name = WShell::GetDisplayName(pTarget, child, SHGDN_NORMAL);
            item.pidl = WShell::Pidl(ILCombine(safeFolder, child)); // <-- use safeFolder, not folder   // ! delete this, lazy now
            item.hash = HashPidl(item.pidl.get());

            item.attributes = SFGAO_FOLDER | SFGAO_CANRENAME | SFGAO_CANDELETE;
            pTarget->GetAttributesOf(1, (LPCITEMIDLIST*)&child, &item.attributes);

            WIN32_FIND_DATAW wfd{};
            if (SUCCEEDED(SHGetDataFromIDListW(pTarget, child, SHGDFIL_FINDDATA, &wfd, sizeof(wfd)))){
                item.size = (static_cast<u64>(wfd.nFileSizeHigh) << 32) | wfd.nFileSizeLow;
                item.lastWriteTime = wfd.ftLastWriteTime;

                // SHFILEINFOW sfi = {};
                // DWORD dwAttr = (item.attributes & SFGAO_FOLDER) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
                // if (SHGetFileInfoW((LPCWSTR)item.pidl.get(), dwAttr, &sfi, sizeof(sfi), SHGFI_PIDL | SHGFI_TYPENAME)) {
                //     item.typeName = Str::WideToString(sfi.szTypeName);
                // }    // caused 20 seconds for recycle bin
            }
            items.push_back(std::move(item));
        });

    return items;
}


/*
DirParent2 GetDirParent(std::wstring& folder){
    DirParent2 parent;

    // risky ... but my middle name is danger
    // parent.pidl = WShell::Pidl(reinterpret_cast<PCIDLIST_ABSOLUTE>(folder.c_str()));
    return parent;
}
*/

DirParent GetDirParent(PCIDLIST_ABSOLUTE folder){
    DirParent parent;
    parent.pidl = WShell::Pidl(ILClone(folder));
    parent.hash = HashPidl(parent.pidl.get());
    parent.name = WShell::GetDisplayName(parent.pidl.get());
    parent.lastWriteTime = WShell::GetModifiedTime(folder);

    return parent;
}

void UpdateParentShellFolder(DirParent& parent){
    if (ILIsEmpty(parent.pidl.get())){ // if is desktop Root
        SHGetDesktopFolder(&parent.shellFolder);
    }
    else {
        HRESULT hr = SHBindToObject(nullptr, parent.pidl.get(), nullptr, IID_PPV_ARGS(&parent.shellFolder));
    
        if (FAILED(hr)) {
            ComPtr<IShellFolder> desktop;
            if (SUCCEEDED(SHGetDesktopFolder(&desktop))) {
                desktop->BindToObject(parent.pidl.get(), nullptr, IID_PPV_ARGS(&parent.shellFolder));
            }
        }
    }
}

std::vector<DirChild> GetDirChildren(IShellFolder* parentShellFolder, PCIDLIST_ABSOLUTE parentPidl){
    std::vector<DirChild> children{};
    if (!parentShellFolder) return children;

    constexpr DWORD flags = SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN;

    IterateFolder(parentShellFolder, flags, [&](IShellFolder* pTarget, PITEMID_CHILD& pChild){
        DirChild child;

        child.pidl = WShell::Pidl(reinterpret_cast<PIDLIST_ABSOLUTE>(pChild));  // Steal ownership
        pChild = nullptr;  // This prevents IterateFolder from freeing it

        child.hash = HashCombinedPidl(parentPidl, child.pidl.get());
        child.name = WShell::GetDisplayName(pTarget, child.pidl.get(), SHGDN_NORMAL);

        child.attributes = SFGAO_FOLDER | SFGAO_CANRENAME | SFGAO_CANDELETE | SFGAO_HIDDEN;
        PCIDLIST_ABSOLUTE childPtr = child.pidl.get();
        pTarget->GetAttributesOf(1, (LPCITEMIDLIST*)&childPtr, &child.attributes);

        WIN32_FIND_DATAW wfd{};
        if (SUCCEEDED(SHGetDataFromIDListW(pTarget, child.pidl.get(), SHGDFIL_FINDDATA, &wfd, sizeof(wfd)))){
            child.size = (static_cast<u64>(wfd.nFileSizeHigh) << 32) | wfd.nFileSizeLow;
            child.lastWriteTime = wfd.ftLastWriteTime;
        }
        children.push_back(std::move(child)); 
    });

    return children;
}

DirChildren GetDirChildren2(IShellFolder* parentShellFolder, PCIDLIST_ABSOLUTE parentPidl){
    DirChildren children{};
    if (!parentShellFolder) return children;

    // Baseline reservations to minimize early reallocations
    constexpr size_t baseline = 64;
    children.hashes.reserve(baseline);
    children.attributes.reserve(baseline);
    children.lastWriteTimes.reserve(baseline);
    children.sizes.reserve(baseline);
    children.pidlOffsets.reserve(baseline);
    children.pidlLengths.reserve(baseline);
    children.nameOffsets.reserve(baseline);
    children.nameLengths.reserve(baseline);
    children.pidlArena.reserve(baseline * 64); // Guess ~64 bytes per PIDL
    children.nameArena.reserve(baseline * 48); // Guess ~48 bytes per name

    constexpr DWORD flags = SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN;

    size_t index = 0;
    IterateFolder(parentShellFolder, flags, [&](IShellFolder* pTarget, PITEMID_CHILD& pChild){
        // PIDL arena
        u16 pidlLen = (u16)ILGetSize(pChild);
        u32 pidlOffset = (u32)children.pidlArena.size(); // offset is the current size

        children.pidlArena.resize(pidlOffset + pidlLen);    // just increases size, if capacity allows, reallocates if it doesnt
        memcpy(&children.pidlArena[pidlOffset], pChild, pidlLen);   // copies the contents of pChild to start of pidl

        children.pidlOffsets.push_back(pidlOffset);
        children.pidlLengths.push_back(pidlLen);
        PCITEMID_CHILD pStoredPidl = children.GetChildPidl(index); // this should be correct
        // does not free pChild;
        

        wchar_t wideBuffer[MAX_PATH] = {0};
        if (!WShell::GetWideDisplayName2(pTarget, pStoredPidl, SHGDN_NORMAL, wideBuffer)){
            PRINTERR;
            wideBuffer[0] = L'\0';
        }
        int sizeNeeded = Str::GetRequiredWideToUtf8Size(wideBuffer); // includes null terminator
        if (sizeNeeded < 0) PRINTERR;   // will prolly see the warnng before it crashes

        u32 nameOffset = (u32)children.nameArena.size();
        children.nameArena.resize(nameOffset + sizeNeeded);
        Str::WriteUtf8CharToBufferFromWide(wideBuffer, &children.nameArena[nameOffset], sizeNeeded);

        children.nameOffsets.push_back(nameOffset);
        children.nameLengths.push_back((u16)(sizeNeeded - 1));


        children.hashes.push_back(HashCombinedPidl(parentPidl, pStoredPidl));


        SFGAOF attrs = SFGAO_FOLDER | SFGAO_CANRENAME | SFGAO_CANDELETE |SFGAO_HIDDEN;
        pTarget->GetAttributesOf(1, (LPCITEMIDLIST*)&pStoredPidl, &attrs);
        children.attributes.push_back(attrs);
        
        u64 size = 0;
        FILETIME lastWriteTime = {};
        WIN32_FIND_DATAW wfd{};
        if (SUCCEEDED(SHGetDataFromIDListW(pTarget, pStoredPidl, SHGDFIL_FINDDATA, &wfd, sizeof(wfd)))){
            size = (static_cast<u64>(wfd.nFileSizeHigh) << 32) | wfd.nFileSizeLow;
            lastWriteTime = wfd.ftLastWriteTime;
        }
        children.sizes.push_back(size);
        children.lastWriteTimes.push_back(lastWriteTime);
        CoTaskMemFree(pChild);
        pChild = nullptr;   // prevent double freeing
        index++;

    });
    shrinkVec(children.hashes);
    shrinkVec(children.attributes);
    shrinkVec(children.lastWriteTimes);
    shrinkVec(children.sizes);
    shrinkVec(children.pidlOffsets);
    shrinkVec(children.pidlLengths);
    shrinkVec(children.nameOffsets);
    shrinkVec(children.nameLengths);
    shrinkVec(children.pidlArena);
    shrinkVec(children.nameArena);

    return children;
}


std::vector<DirItem> GetOneDriveAccounts(){
    std::vector<DirItem> accounts;

    HKEY hKeyRoot = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\OneDrive\\Accounts", 0, KEY_READ, &hKeyRoot) != ERROR_SUCCESS){
        return accounts;   // OneDrive not installed/configured nothing to show
    }
    wchar_t subKeyName[256];
    for (DWORD index = 0; ; index++){
        DWORD nameLen = 256;   // reset every iteration
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
            DirItem account;

            account.pidl = WShell::GetFullPath(userFolder);
            if (account.pidl){
                account.name = WShell::GetDisplayName(account.pidl.get());
                accounts.push_back(std::move(account));
            }
        }
        RegCloseKey(hKeyAccount);
    }

    RegCloseKey(hKeyRoot);
    return accounts;
}