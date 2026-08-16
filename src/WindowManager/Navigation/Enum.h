#pragma once

#include <ShlObj.h>
#include "Pidl.h"
#include <vector>
#include "Str.h"
#include "Shell.h"
#include "Item.h"

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
            item.pidl = WShell::Pidl(ILCombine(safeFolder, child)); // <-- use safeFolder, not folder
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