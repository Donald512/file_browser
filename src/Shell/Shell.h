#pragma once
#include "Str.h"
#include <Shlwapi.h>
#include <ShlObj.h>
#include <optional>
#include "ComUtils.h"
#include <wrl/client.h>
#include "KnownSpecialFolders.h"
#include <propsys.h>
#include <propkey.h>
#include <propvarutil.h>


namespace WShell{

    using Microsoft::WRL::ComPtr;
    // Extracts a child's display name cleanly.
    inline std::string GetDisplayName(IShellFolder* folder, PCITEMID_CHILD child, SHGDNF flags) {
        STRRET strName;
        if (SUCCEEDED(folder->GetDisplayNameOf(child, flags, &strName))) {
            wchar_t nameBuffer[MAX_PATH] = {};
            StrRetToBufW(&strName, child, nameBuffer, MAX_PATH);
            return Str::WideToString(nameBuffer);
        }
        return "";
    }

    // this function might have a bug, because the destination needs to know the size of the name
    inline int GetDisplayName2(IShellFolder* folder, PCITEMID_CHILD child, SHGDNF flags, char* dest) {  // writes the char* to a buffer eg, in an arena, prevent mallocing, copying, and freeing, when you can just write straight to the dest
        STRRET strName;
        if (SUCCEEDED(folder->GetDisplayNameOf(child, flags, &strName))) {
            wchar_t nameBuffer[MAX_PATH] = {};
            StrRetToBufW(&strName, child, nameBuffer, MAX_PATH);
            return Str::WideToUtf8(nameBuffer, dest);
        }
        return -1;
    }
    
    inline bool GetWideDisplayName2(IShellFolder* folder, PCITEMID_CHILD child, SHGDNF flags, wchar_t* wBufOut){
        STRRET strName;
        if (SUCCEEDED(folder->GetDisplayNameOf(child, flags, &strName))) {
            if (FAILED(StrRetToBufW(&strName, child, wBufOut, MAX_PATH))){
                return false;
            }
            return true;
        }
        return false;
    }
    
    inline std::string GetDisplayName(PCIDLIST_ABSOLUTE pidl ) {
        wchar_t* niceName = nullptr;
        if (SUCCEEDED(SHGetNameFromIDList(pidl, SIGDN_NORMALDISPLAY, &niceName))){
            std::string name = Str::WideToString(niceName);
            CoTaskMemFree(niceName);
            return name;
        }
        return "";
    }
    
        
    // Helper: Safely fetches a Shell name and automatically handles COM memory & UTF-8 conversion
    inline std::string GetShellName(PCIDLIST_ABSOLUTE pidl, SIGDN sigdn) {
        PWSTR pRawPath = nullptr;
        if (SUCCEEDED(SHGetNameFromIDList(pidl, sigdn, &pRawPath)) && pRawPath) {
            UniqueCoTaskStr pathGuard(pRawPath); // RAII: Free memory on exit or exception
            return Str::WideToString(pathGuard.get());
        }
        return std::string{};
    }

    inline std::string GetFullPath(PCIDLIST_ABSOLUTE pidl){ 
        if (!pidl || pidl->mkid.cb == 0) {
            // If it's the root Desktop, just return "Desktop"
            return "Desktop";
        }

        const SIGDN fallbacksInOrder[] = {SIGDN_FILESYSPATH, SIGDN_PARENTRELATIVEFORADDRESSBAR, SIGDN_NORMALDISPLAY, SIGDN_DESKTOPABSOLUTEPARSING};

        
        for (auto sigdn : fallbacksInOrder){
            const std::string path = GetShellName(pidl, sigdn);
            if (!path.empty() && !((path.size() >= 2) && path[0] == ':' && path[1] == ':')){
                return path;
            }
        }
        return "";
    }

    bool GetPidlTypeName(IShellFolder* pParent, PCITEMID_CHILD childPidl, wchar_t* outBuffer, UINT maxChars) {
        if (!pParent || !childPidl || !outBuffer || maxChars == 0) return false;

        ComPtr<IShellFolder2> pFolder2;
        if (FAILED(pParent->QueryInterface(IID_PPV_ARGS(&pFolder2)))) return false;

        VARIANT var;
        VariantInit(&var);
        
        // PKEY_ItemTypeText gets the localized "Type" string (e.g. "Text Document", "File folder")
        HRESULT hr = pFolder2->GetDetailsEx(childPidl, &PKEY_ItemTypeText, &var);
        
        if (SUCCEEDED(hr) && var.vt == VT_BSTR && var.bstrVal != nullptr) {
            wcsncpy_s(outBuffer, maxChars, var.bstrVal, _TRUNCATE);
            VariantClear(&var);
            return true;
        }

        VariantClear(&var);
        outBuffer[0] = L'\0';
        return false;
    }
        

    bool GetItemTypeName(PCIDLIST_ABSOLUTE parentPidl, IShellFolder* pParent, PCITEMID_CHILD child, wchar_t* out) {
        ComPtr<IShellItem2> item;
        if (FAILED(SHCreateItemWithParent(parentPidl, pParent, child, IID_PPV_ARGS(&item)))) 
            return false;

        PWSTR text = nullptr;
        if (FAILED(item->GetString(PKEY_ItemTypeText, &text))) 
            return false;

        out = text;
        CoTaskMemFree(text);
        return true;
    }

    
    Pidl GetFullPath(const wchar_t* widePath){

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

    
    inline bool PidlHasSubFolders(PCIDLIST_ABSOLUTE folder, bool accurate = false){
        if (!folder) return false;
        if (ILIsEqual(folder, SpecialFolders::pidlHome)) return false;

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
            return hasSubFolders;
        }
        if (SUCCEEDED(pParentItem->GetAttributes(SFGAO_HASSUBFOLDER, &attr))) {
            return (attr & SFGAO_HASSUBFOLDER) != 0;
        }
        return false;
    }

    inline u32 GetIconIndex(PCIDLIST_ABSOLUTE pidl, const wchar_t* pszPath, DWORD dwFileAttributes, UINT uFlags){
        // if (!pidl) return 0;

        // memoize, map<Pidl | wchar_t*, bool>
        SHFILEINFOW sfi = {};

        if (pidl){
            SHGetFileInfoW((LPCWSTR) pidl, dwFileAttributes, &sfi, sizeof(sfi), uFlags);
        }
        else if (pszPath){
            SHGetFileInfoW(pszPath, dwFileAttributes, &sfi, sizeof(sfi), uFlags);
        }
        // sfi.iIcon now contains the unique icon index
        return sfi.iIcon; // even if it fails, it returns 0
    }

    inline FILETIME GetModifiedTime(PCIDLIST_ABSOLUTE pidl){
        IPropertyStore* pStore = nullptr;

        FILETIME lastWriteTime{};
        if (SUCCEEDED(SHGetPropertyStoreFromIDList(pidl, GPS_FASTPROPERTIESONLY, IID_PPV_ARGS(&pStore)))){
            PROPVARIANT propvar;
            PropVariantInit(&propvar);

            if (SUCCEEDED(pStore->GetValue(PKEY_DateModified, &propvar))){
                if (propvar.vt == VT_FILETIME){
                    lastWriteTime = propvar.filetime;
                }
                PropVariantClear(&propvar);
            }
            pStore->Release();
        }
        return lastWriteTime;
    }


    inline bool ExecuteFile(PCIDLIST_ABSOLUTE file){
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


}