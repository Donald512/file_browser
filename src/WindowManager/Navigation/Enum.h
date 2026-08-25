#pragma once

#include "WinFramework.h"
#include <ShlObj.h>

#include <vector>

#include "Item.h"

#include "TypenameManager.h"

#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

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

std::vector<DirItem> EnumFolder(PCIDLIST_ABSOLUTE folder, DirItem* parentItem = nullptr);

DirParent GetDirParent(PCIDLIST_ABSOLUTE folder);

void UpdateParentShellFolder(DirParent& parent);

std::vector<DirChild> GetDirChildren(IShellFolder* parentShellFolder, PCIDLIST_ABSOLUTE parentPidl);

DirChildren GetDirChildren2(IShellFolder* parentShellFolder, PCIDLIST_ABSOLUTE parentPidl, TypenameStore& typeStore);


std::vector<DirItem> GetOneDriveAccounts();