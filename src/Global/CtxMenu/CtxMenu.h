#pragma once
#include "wrl/client.h"
#include <string>
#include <vector>
#include "imgui.h"
#include "BasicTypes.h"
#include <ShlObj.h>
#include <d3d11.h>


struct App;

using Microsoft::WRL::ComPtr;

struct ContextMenuItem {
    std::string text;
    std::string shortcut;
    std::string verb;
    UINT id;            // The offset ID to invoke the command later
    bool isSeparator = false;
    bool enabled = true;
    bool checked = false;

    ComPtr<ID3D11ShaderResourceView> srv; 
    ImTextureID hIconTex{};      

    std::vector<ContextMenuItem> subItems;  // for nested menus

    std::string label;      // per-item cached padded label
    f32 labelDpi = -1.0f;
};

#include <shlwapi.h> // IUnknown_SetSite
#pragma comment(lib, "shlwapi.lib")

class MinimalShellBrowser : public IShellBrowser {
    LONG m_refCount = 1;
    HWND m_hwnd;
public:
    explicit MinimalShellBrowser(HWND hwnd) : m_hwnd(hwnd) {}

    IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
        if (riid == IID_IUnknown || riid == IID_IOleWindow || riid == IID_IShellBrowser) {
            *ppv = static_cast<IShellBrowser*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    IFACEMETHODIMP_(ULONG) AddRef() override { return InterlockedIncrement(&m_refCount); }
    IFACEMETHODIMP_(ULONG) Release() override {
        ULONG c = InterlockedDecrement(&m_refCount);
        if (c == 0) delete this;
        return c;
    }

    // IOleWindow
    IFACEMETHODIMP GetWindow(HWND* phwnd) override { *phwnd = m_hwnd; return S_OK; }
    IFACEMETHODIMP ContextSensitiveHelp(BOOL) override { return E_NOTIMPL; }

    // IShellBrowser — stub everything except GetWindow
    IFACEMETHODIMP InsertMenusSB(HMENU, LPOLEMENUGROUPWIDTHS) override { return E_NOTIMPL; }
    IFACEMETHODIMP SetMenuSB(HMENU, HOLEMENU, HWND) override { return E_NOTIMPL; }
    IFACEMETHODIMP RemoveMenusSB(HMENU) override { return E_NOTIMPL; }
    IFACEMETHODIMP SetStatusTextSB(LPCOLESTR) override { return E_NOTIMPL; }
    IFACEMETHODIMP EnableModelessSB(BOOL) override { return E_NOTIMPL; }
    IFACEMETHODIMP TranslateAcceleratorSB(MSG*, WORD) override { return E_NOTIMPL; }
    IFACEMETHODIMP BrowseObject(LPCITEMIDLIST, UINT) override { return E_NOTIMPL; }
    IFACEMETHODIMP GetViewStateStream(DWORD, IStream**) override { return E_NOTIMPL; }
    IFACEMETHODIMP GetControlWindow(UINT, HWND* phwnd) override { *phwnd = nullptr; return S_FALSE; }
    IFACEMETHODIMP SendControlMsg(UINT, UINT, WPARAM, LPARAM, LRESULT*) override { return E_NOTIMPL; }
    IFACEMETHODIMP QueryActiveShellView(IShellView** ppshv) override { *ppshv = nullptr; return E_FAIL; }
    IFACEMETHODIMP OnViewWindowActive(IShellView*) override { return E_NOTIMPL; }
    IFACEMETHODIMP SetToolbarItems(LPTBBUTTONSB, UINT, UINT) override { return E_NOTIMPL; }
};


void ExecuteContextMenuCommand(App& app, ComPtr<IContextMenu> menu, PCIDLIST_ABSOLUTE parentPidl, std::vector<PCITEMID_CHILD>& childPidls, UINT idOffset, HWND ownerHwnd);



std::vector<ContextMenuItem> GetBackgroundContextMenu(ComPtr<IContextMenu>& outMenu, PCIDLIST_ABSOLUTE folderPidl, ID3D11Device* dev);

std::vector<ContextMenuItem> GetContextMenu(ComPtr<IContextMenu>& outActiveMenu, PCIDLIST_ABSOLUTE parentPidl, std::vector<PCITEMID_CHILD>& childPidls, HWND hwnd, ID3D11Device* pDevice);