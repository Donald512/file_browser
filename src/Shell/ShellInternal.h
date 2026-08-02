#pragma once
#include "Shell.h"


std::string GetDisplayName(IShellFolder* folder, PITEMID_CHILD child, SHGDNF flags);
std::string GetDisplayName(PCIDLIST_ABSOLUTE pidl );

WShell::Pidl CombineChild(PCIDLIST_ABSOLUTE parent, PITEMID_CHILD child);


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