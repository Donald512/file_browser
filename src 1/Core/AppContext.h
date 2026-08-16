#pragma once

#include <d3d11.h>
#include "imgui.h"
#include <ShlObj.h>
#include <string>
#include <vector>
#include <unordered_set>
#include <wrl/client.h>


#include "Types.h"
#include "Shell.h"
#include "Navigation.h"
#include "icons.h"


#include "TaskSystem.h"
#include "Threading.h"
#include <mutex>
#include <queue>




using Microsoft::WRL::ComPtr;

struct GraphicsContext{
    HWND hwnd = nullptr;
    ComPtr<ID3D11Device> d3dDevice;
    ComPtr<ID3D11DeviceContext> d3dContext;
    ComPtr<IDXGISwapChain> swapChain;
    ComPtr<ID3D11RenderTargetView> renderTargetView;

    bool swapChainOccluded = false;
    UINT resizeWidth = 0;
    UINT resizeHeight = 0;
};

struct UiState{
    f32 dpiScale = 1.0f;
    ImFont* mainFont = nullptr;   // non-owning — ImGui's font atlas owns these
    ImFont* smallFont = nullptr;   // non-owning — ImGui's font atlas owns these
    ImFont* iconFont = nullptr;
    ImVec4 clearColor = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
};


struct AppContext{
    GraphicsContext gfx;
    UiState ui;

    // Navigation & icon caching
    Navigation::NavigationController navigation;
    Icons::IconManager icons;

    // Special, frequently-referenced pidls
    WShell::Pidl pidlThisPC;
    WShell::Pidl pidlHome;
    WShell::Pidl pidlDesktop;
    WShell::Pidl pidlQuickAccess;
    WShell::Pidl pidlNetwork;


    // these are populated Asynchronously shorty after startup, called once from main.cpp right after AppContext exists, and tasks, (at the bottom) is constructed and ready. Sidebar and command bar are written expecting that rendering an empty vector is no-op, 
    std::vector<WShell::NewMenuItem> newMenuItems;
    std::vector<WShell::Item> items1;    
    std::vector<WShell::Item> items2;    
    std::vector<WShell::Item> items3; 

    TaskSystem tasks;

    ComPtr<IContextMenu> activeContextMenu;
    std::vector<WShell::ContextMenuItem> activeContextMenuData;

    void UpdateContextMenu(PCIDLIST_ABSOLUTE pidl){
        activeContextMenuData = WShell::GetContextMenu(activeContextMenu, pidl, gfx.d3dDevice.Get());
    }

    std::unordered_set<u64> clipBoardCutItems{};
    UINT cfDropEffect;
    UINT cfShellIDList;

    bool isFileCutOnClipBoard(u64 hashedPidl){
        return clipBoardCutItems.find(hashedPidl) != clipBoardCutItems.end();
    }
};

