#pragma once

#include <ShlObj.h>
#include <ShlObj_core.h>
#include <string>
#include "Pidl.h"

namespace WShell{

    // Extracts a child's display name.
    std::string GetDisplayName(IShellFolder* folder, PCITEMID_CHILD child, SHGDNF flags);

    // this function might have a bug, because the destination needs to know the size of the name
    int GetDisplayName2(IShellFolder* folder, PCITEMID_CHILD child, SHGDNF flags, char* dest);

    bool GetWideDisplayName2(IShellFolder* folder, PCITEMID_CHILD child, SHGDNF flags, wchar_t* wBufOut);
    
    std::string GetDisplayName(PCIDLIST_ABSOLUTE pidl );
        
    // Helper: Safely fetches a Shell name and automatically handles COM memory & UTF-8 conversion
    std::string GetShellName(PCIDLIST_ABSOLUTE pidl, SIGDN sigdn);

    std::string GetFullPath(PCIDLIST_ABSOLUTE pidl);

    bool GetPidlTypeName(IShellFolder* pParent, PCITEMID_CHILD childPidl, wchar_t* outBuffer, UINT maxChars);

    bool GetItemTypeName(PCIDLIST_ABSOLUTE parentPidl, IShellFolder* pParent, PCITEMID_CHILD child, wchar_t* out);
    
    Pidl GetFullPath(const wchar_t* widePath);

    
    bool PidlHasSubFolders(PCIDLIST_ABSOLUTE folder, bool accurate = false);

    u32 GetIconIndex(PCIDLIST_ABSOLUTE pidl, const wchar_t* pszPath, DWORD dwFileAttributes, UINT uFlags);

    FILETIME GetModifiedTime(PCIDLIST_ABSOLUTE pidl);


    bool ExecuteFile(PCIDLIST_ABSOLUTE file);

    
    struct RenameChild{
        PCITEMID_CHILD pidl;
        const char* name;
    };

    void CommitRename(HWND hwnd, PCIDLIST_ABSOLUTE parentPidl, RenameChild child, const char* newName);

}

class RenameProgressSink : public IFileOperationProgressSink {
public:
    HRESULT result = S_OK;
    std::wstring failedName;
    std::wstring createdName; // <-- Stores the final uniquely collided name
    std::wstring createdFullPath;

    IFACEMETHODIMP PostRenameItem(DWORD, IShellItem*, LPCWSTR pszNewName, HRESULT hrRename, IShellItem*) override {
        if (FAILED(hrRename)){
            result = hrRename;
            if (pszNewName) failedName = pszNewName;
        }
        return S_OK;
    }

    IFACEMETHODIMP PostNewItem(DWORD, IShellItem*, LPCWSTR, LPCWSTR, DWORD, HRESULT hrNew, IShellItem* psiNewItem) override {
        if (SUCCEEDED(hrNew) && psiNewItem) {
            PWSTR pszFullPath = nullptr;
            // Get the display name of the newly created item
            if (SUCCEEDED(psiNewItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFullPath))) {
                createdFullPath = pszFullPath; // Captures "New folder (2)", etc.
                CoTaskMemFree(pszFullPath);
            }

            PWSTR pszLeafName = nullptr;
            if (SUCCEEDED(psiNewItem->GetDisplayName(SIGDN_NORMALDISPLAY, &pszLeafName))){
                createdName = pszLeafName;
                CoTaskMemFree(pszLeafName);
            }
        } 

        else if (FAILED(hrNew)) result = hrNew;
        return S_OK;
    }


    // Boilerplate no-ops for the rest of the interface
    IFACEMETHODIMP StartOperations() override { return S_OK; }
    IFACEMETHODIMP FinishOperations(HRESULT) override { return S_OK; }
    IFACEMETHODIMP PreRenameItem(DWORD, IShellItem*, LPCWSTR) override { return S_OK; }
    IFACEMETHODIMP PreMoveItem(DWORD, IShellItem*, IShellItem*, LPCWSTR) override { return S_OK; }
    IFACEMETHODIMP PostMoveItem(DWORD, IShellItem*, IShellItem*, LPCWSTR, HRESULT, IShellItem*) override { return S_OK; }
    IFACEMETHODIMP PreCopyItem(DWORD, IShellItem*, IShellItem*, LPCWSTR) override { return S_OK; }
    IFACEMETHODIMP PostCopyItem(DWORD, IShellItem*, IShellItem*, LPCWSTR, HRESULT, IShellItem*) override { return S_OK; }
    IFACEMETHODIMP PreDeleteItem(DWORD, IShellItem*) override { return S_OK; }
    IFACEMETHODIMP PostDeleteItem(DWORD, IShellItem*, HRESULT, IShellItem*) override { return S_OK; }
    IFACEMETHODIMP PreNewItem(DWORD, IShellItem*, LPCWSTR) override { return S_OK; }
    IFACEMETHODIMP UpdateProgress(UINT, UINT) override { return S_OK; }
    IFACEMETHODIMP ResetTimer() override { return S_OK; }
    IFACEMETHODIMP PauseTimer() override { return S_OK; }
    IFACEMETHODIMP ResumeTimer() override { return S_OK; }

    // IUnknown - trivial since this lives on the stack for one call
    IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
        if (riid == IID_IUnknown || riid == IID_IFileOperationProgressSink){ *ppv = this; return S_OK; }
        *ppv = nullptr; return E_NOINTERFACE;
    }
    IFACEMETHODIMP_(ULONG) AddRef() override { return 1; }
    IFACEMETHODIMP_(ULONG) Release() override { return 1; }
};