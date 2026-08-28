
#include "Enum.h"

#include "Pidl.h"
#include "Str.h"

#include "Shell.h"
#include "BasicTypes.h"
#include "Types/global.h"
#include <iostream>
#include <propkey.h>
#include <propvarutil.h>

std::vector<DirItem> EnumFolder(PCIDLIST_ABSOLUTE folder, DirItem* parentItem){
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

DirChildren GetDirChildren2(IShellFolder* parentShellFolder, PCIDLIST_ABSOLUTE parentPidl, TypenameStore& typeStore){
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
    children.typenameIndex.reserve(baseline);
    children.pidlArena.reserve(baseline * 64); // Guess ~64 bytes per PIDL
    children.nameArena.reserve(baseline * 48); // Guess ~48 bytes per name

    constexpr DWORD flags = SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN;

    size_t index = 0;
    IterateFolder(parentShellFolder, flags, [&](IShellFolder* pTarget, PITEMID_CHILD& pChild){  // todo add a bool continue, or continue condition like while tab.isopen
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

        wchar_t typeBuffer[MAX_PATH] = { 0 }; 
        WShell::GetPidlTypeName(pTarget, pStoredPidl, typeBuffer, _countof(typeBuffer));

        int typeSizeNeeded = Str::GetRequiredWideToUtf8Size(typeBuffer);
        u16 assignedTypeIdx = TypenameStore::InvalidIndex;

        if (typeSizeNeeded > 0){
            std::vector<char> tempTypeBuffer(typeSizeNeeded);
            Str::WriteUtf8CharToBufferFromWide(typeBuffer, tempTypeBuffer.data(), typeSizeNeeded);

            // Pass the transient string to pool intern storage
            assignedTypeIdx = typeStore.GetOrCreateId(tempTypeBuffer.data());
        }
        children.typenameIndex.push_back(assignedTypeIdx);

        children.hashes.push_back(HashItemIdentity(parentPidl, pStoredPidl));


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
    shrinkVec(children.typenameIndex); 
    shrinkVec(children.pidlArena);
    shrinkVec(children.nameArena);

    return children;
}


DirectoryBuildResult BuildDirectoryOffsite(IShellFolder* pTarget, PCIDLIST_ABSOLUTE parentPidl) {
    DirectoryBuildResult result;
    DirChildren& children = result.children;
    
    if (!pTarget) return result;

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
    children.typenameIndex.reserve(baseline);
    children.pidlArena.reserve(baseline * 64); // Guess ~64 bytes per PIDL
    children.nameArena.reserve(baseline * 48); // Guess ~48 bytes per name


    constexpr DWORD flags = SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN;
    size_t index = 0;

    IterateFolder(pTarget, flags, [&](IShellFolder* pFolder, PITEMID_CHILD& pChild) {
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


        // TYPE LOGIC: Collect raw string instead of interning
        wchar_t typeBuffer[MAX_PATH] = { 0 }; 
        WShell::GetPidlTypeName(pFolder, pStoredPidl, typeBuffer, _countof(typeBuffer));
        result.rawTypes.push_back(Str::WideToUtf8(typeBuffer)); // convert wchar to std::string
        children.typenameIndex.push_back(TypenameStore::InvalidIndex); // Placeholder

        children.hashes.push_back(HashItemIdentity(parentPidl, pStoredPidl));


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
    shrinkVec(children.typenameIndex); 
    shrinkVec(children.pidlArena);
    shrinkVec(children.nameArena);

    return result;
};


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