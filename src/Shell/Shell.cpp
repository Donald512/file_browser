#pragma once



#include "Str.h"
#include <Shlwapi.h>
#include <ShlObj.h>
#include "ComUtils.h"
#include "KnownSpecialFolders.h"
#include <propsys.h>
#include <propkey.h>
#include "Shell.h"
#include <propvarutil.h>
#include "App.h"


#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

namespace WShell{

    // Extracts a child's display name cleanly.
    std::string GetDisplayName(IShellFolder* folder, PCITEMID_CHILD child, SHGDNF flags) {
        STRRET strName;
        if (SUCCEEDED(folder->GetDisplayNameOf(child, flags, &strName))) {
            wchar_t nameBuffer[MAX_PATH] = {};
            StrRetToBufW(&strName, child, nameBuffer, MAX_PATH);
            return Str::WideToString(nameBuffer);
        }
        return "";
    }

    // this function might have a bug, because the destination needs to know the size of the name
    int GetDisplayName2(IShellFolder* folder, PCITEMID_CHILD child, SHGDNF flags, char* dest) {  // writes the char* to a buffer eg, in an arena, prevent mallocing, copying, and freeing, when you can just write straight to the dest
        STRRET strName;
        if (SUCCEEDED(folder->GetDisplayNameOf(child, flags, &strName))) {
            wchar_t nameBuffer[MAX_PATH] = {};
            StrRetToBufW(&strName, child, nameBuffer, MAX_PATH);
            return Str::WideToUtf8(nameBuffer, dest);
        }
        return -1;
    }
    
    bool GetWideDisplayName2(IShellFolder* folder, PCITEMID_CHILD child, SHGDNF flags, wchar_t* wBufOut){
        STRRET strName;
        if (SUCCEEDED(folder->GetDisplayNameOf(child, flags, &strName))) {
            if (FAILED(StrRetToBufW(&strName, child, wBufOut, MAX_PATH))){
                return false;
            }
            return true;
        }
        return false;
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
    
        
    // Helper: Safely fetches a Shell name and automatically handles COM memory & UTF-8 conversion
    std::string GetShellName(PCIDLIST_ABSOLUTE pidl, SIGDN sigdn) {
        PWSTR pRawPath = nullptr;
        if (SUCCEEDED(SHGetNameFromIDList(pidl, sigdn, &pRawPath)) && pRawPath) {
            UniqueCoTaskStr pathGuard(pRawPath); // RAII: Free memory on exit or exception
            return Str::WideToString(pathGuard.get());
        }
        return std::string{};
    }

    std::string GetFullPath(PCIDLIST_ABSOLUTE pidl){ 
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

    
    bool PidlHasSubFolders(PCIDLIST_ABSOLUTE folder, bool accurate){
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

    u32 GetIconIndex(PCIDLIST_ABSOLUTE pidl, const wchar_t* pszPath, DWORD dwFileAttributes, UINT uFlags){
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

    FILETIME GetModifiedTime(PCIDLIST_ABSOLUTE pidl){
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

    bool ExecuteFile(PCIDLIST_ABSOLUTE file){
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

    void ShowRenameError(HWND hwnd, HRESULT hr){
        LPWSTR msgBuf = nullptr;
        DWORD dwFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
        DWORD len = FormatMessageW(dwFlags, nullptr, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&msgBuf, 0, nullptr);

        wchar_t fullMsg[1024] = {};
        if (len && msgBuf){
            swprintf_s(fullMsg, L"Failed to rename file. \n\nError 0x%08X: %s", (unsigned) hr, msgBuf);
            LocalFree(msgBuf);
        }   else swprintf_s(fullMsg, L"Failed to rename file. \n\nError 0x%08X: (no description available)", (unsigned) hr);

        MessageBoxW(hwnd, fullMsg, L"Error", MB_OK | MB_ICONERROR);
    }

    void CommitRename(HWND hwnd, PCIDLIST_ABSOLUTE parentPidl, RenameChild child, const char* newName){
        if (strcmp(child.name, newName) == 0) return; // Names are identical, no need to rename
        std::wstring wNewName = Str::Utf8ToWide(newName, 0, nullptr);

        ComPtr<IFileOperation> fileOp;
        HRESULT hr = CoCreateInstance(CLSID_FileOperation, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&fileOp));
        if (FAILED(hr)) {ShowRenameError(hwnd, hr); return;}

        // No progress dialog, no confirmation UI, no "flying files" animation - keep it silent
        fileOp->SetOperationFlags(FOF_NO_UI | FOFX_SHOWELEVATIONPROMPT | FOFX_EARLYFAILURE | FOF_ALLOWUNDO);
        
        PIDLIST_ABSOLUTE fullPidl = ILCombine(parentPidl, child.pidl);
        if (!fullPidl) {ShowRenameError(hwnd, E_FAIL); return;}

        ComPtr<IShellItem> item;
        hr = SHCreateItemFromIDList(fullPidl, IID_PPV_ARGS(&item));
        ILFree(fullPidl);
        if (FAILED(hr)) {ShowRenameError(hwnd, hr); return;}

        hr = fileOp->RenameItem(item.Get(), wNewName.c_str(), nullptr);
        if (FAILED(hr)) {ShowRenameError(hwnd, hr); return;}

        RenameProgressSink sink;
        DWORD cookie = 0;
        fileOp->Advise(&sink, &cookie);

        hr = fileOp->PerformOperations();
        fileOp->Unadvise(cookie);

        BOOL aborted = FALSE;
        fileOp->GetAnyOperationsAborted(&aborted);

        if (FAILED(hr) || aborted || FAILED(sink.result)){
            HRESULT realErr = FAILED(sink.result) ? sink.result : hr;
            ShowRenameError(hwnd, realErr);
        }
    }

    // Resolves template path relative to ShellNew or Templates directory
    static std::wstring ResolveTemplatePath(const std::wstring& fn, const std::wstring& winShellNew) {
        if (fn.empty()) return L"";
        if (fn.size() > 2 && (fn[1] == L':' || (fn[0] == L'\\' && fn[1] == L'\\'))) {
            return fn; // Absolute path
        }

        // 1. Check %windir%\ShellNew
        std::wstring path = winShellNew + L"\\" + fn;
        if (GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES) return path;

        // 2. Check %APPDATA%\Microsoft\Windows\Templates
        wchar_t appData[MAX_PATH];
        if (GetEnvironmentVariableW(L"APPDATA", appData, MAX_PATH) > 0) {
            path = std::wstring(appData) + L"\\Microsoft\\Windows\\Templates\\" + fn;
            if (GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES) return path;
        }

        return winShellNew + L"\\" + fn;
    }

    // Retrieves friendly display name
    static std::wstring GetShellNewDisplayName(HKEY hNew, const std::wstring& ext) {
        wchar_t buf[512] = { 0 };
        DWORD bufSize = sizeof(buf);

        // 1. Try reading indirect resource string from ItemName or MenuText
        if (RegQueryValueExW(hNew, L"ItemName", nullptr, nullptr, (LPBYTE)buf, &bufSize) == ERROR_SUCCESS ||
            RegQueryValueExW(hNew, L"MenuText", nullptr, nullptr, (LPBYTE)buf, &bufSize) == ERROR_SUCCESS) {
            wchar_t resolved[256] = { 0 };
            if (SHLoadIndirectString(buf, resolved, ARRAYSIZE(resolved), nullptr) == S_OK && resolved[0] != L'\0') return resolved;
        }

        // 2. AssocQueryString for ASSOCSTR_FRIENDLYDOCNAME
        DWORD cch = ARRAYSIZE(buf);
        if (AssocQueryStringW(ASSOCF_NONE, ASSOCSTR_FRIENDLYDOCNAME, ext.c_str(), nullptr, buf, &cch) == S_OK && buf[0] != L'\0') return buf;

        // 3. Fallback
        return ext.empty() ? L"File" : ext;
    }
    
    // Extracts template/data from an opened ShellNew key
    bool ProcessShellNewKey(HKEY hNew, const std::wstring& ext, const std::wstring& shellNewDir, ShellNewEntry& entry) {
        bool hasFile = (RegQueryValueExW(hNew, L"FileName", nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS);
        bool hasData = (RegQueryValueExW(hNew, L"Data", nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS);
        bool hasNull = (RegQueryValueExW(hNew, L"NullFile", nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS);
        bool hasCmd  = (RegQueryValueExW(hNew, L"Command", nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS);

        // If it has no creation method, it's not a valid ShellNew item
        if (!hasFile && !hasData && !hasNull && !hasCmd) return false;

        // Skip Command entries (installer launchers), except .lnk shortcuts
        if (hasCmd && ext != L".lnk") return false;

        entry.ext = ext;
        entry.baseName = GetShellNewDisplayName(hNew, ext);

        if (hasFile) {
            DWORD sz = 0;
            if (RegQueryValueExW(hNew, L"FileName", nullptr, nullptr, nullptr, &sz) == ERROR_SUCCESS && sz > 0) {
                std::wstring fn(sz / sizeof(wchar_t), L'\0');
                RegQueryValueExW(hNew, L"FileName", nullptr, nullptr, (LPBYTE)fn.data(), &sz);
                while (!fn.empty() && fn.back() == L'\0') fn.pop_back();
                entry.templatePath = ResolveTemplatePath(fn, shellNewDir);
            }
        }

        if (hasData) {
            DWORD sz = 0;
            if (RegQueryValueExW(hNew, L"Data", nullptr, nullptr, nullptr, &sz) == ERROR_SUCCESS && sz > 0) {
                entry.data.resize(sz);
                RegQueryValueExW(hNew, L"Data", nullptr, nullptr, entry.data.data(), &sz);
            }
        }

        return true;
    }

    void CreateShellNewItem(App& app, PCIDLIST_ABSOLUTE parentPidl, const ShellNewEntry& e) {
        // Special case: Shortcut (.lnk) launches the native Windows wizard
        if (e.ext == L".lnk") {
            wchar_t folderPath[MAX_PATH];
            if (SHGetPathFromIDListW(parentPidl, folderPath)) {
                std::wstring cmd = L"rundll32.exe appwiz.cpl,NewLinkHere " + std::wstring(folderPath);
                STARTUPINFOW si{ sizeof(si) };
                PROCESS_INFORMATION pi{};
                if (CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
                    CloseHandle(pi.hThread);
                    CloseHandle(pi.hProcess);
                }
            }
            return;
        }

        ComPtr<IShellItem> psiParent;   
        if (FAILED(SHCreateItemFromIDList(parentPidl, IID_PPV_ARGS(&psiParent)))) return;
        
        ComPtr<IFileOperation> pfo;     
        if (FAILED(CoCreateInstance(CLSID_FileOperation, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&pfo)))) return;
        pfo->SetOperationFlags(FOF_SILENT | FOF_NOCONFIRMATION | FOFX_SHOWELEVATIONPROMPT | FOF_RENAMEONCOLLISION | FOF_ALLOWUNDO);

        RenameProgressSink sink; 
        DWORD cookie = 0; 
        pfo->Advise(&sink, &cookie);

        // Format the Explorer-style default name:
        // e.g. "New folder", "New Text Document.txt", "New Microsoft Word Document.docx"
        std::wstring fullName;
        if (e.isFolder) {
            fullName = L"New folder";
        } else if (e.baseName.rfind(L"New ", 0) == 0) {
            fullName = e.baseName + e.ext;
        } else {
            fullName = L"New " + e.baseName + e.ext;
        }

        HRESULT hr;
        if (e.isFolder) {
            hr = pfo->NewItem(psiParent.Get(), FILE_ATTRIBUTE_DIRECTORY, fullName.c_str(), nullptr, nullptr);
        } else {
            const wchar_t* templatePathPtr = e.templatePath.empty() ? nullptr : e.templatePath.c_str();
            hr = pfo->NewItem(psiParent.Get(), FILE_ATTRIBUTE_NORMAL, fullName.c_str(), templatePathPtr, nullptr);
        }
        
        if (SUCCEEDED(hr) && SUCCEEDED(pfo->PerformOperations()) && SUCCEEDED(sink.result)) {
            if (!e.data.empty()) { // Write binary blob if needed
                HANDLE h = CreateFileW(sink.createdFullPath.c_str(), GENERIC_WRITE, 0, nullptr, TRUNCATE_EXISTING, 0, nullptr);
                if (h != INVALID_HANDLE_VALUE) { 
                    DWORD w; 
                    WriteFile(h, e.data.data(), (DWORD)e.data.size(), &w, nullptr); 
                    CloseHandle(h); 
                }
            }

            // Trigger inline rename on the newly created item
            auto& tab = app.window.GetActiveTab();
            tab.newState.expectingNewItem = true;
            tab.newState.itemHash = HashIdentityString(sink.createdFullPath);
            tab.newState.itemName = Str::WideToString(sink.createdName); 
        }
        pfo->Unadvise(cookie);
    }

}

