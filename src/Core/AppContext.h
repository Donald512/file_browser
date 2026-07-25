#pragma once

#include <d3d11.h>
#include <imgui.h>
#include <ShlObj.h>
#include <string>
#include <vector>
#include <wrl/client.h>

#include "Types.h"
#include "Shell.h"
#include "Navigation.h"
#include "icons.h"


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

    std::vector<WShell::NewMenuItem> newMenuItems = WShell::EnumerateNewMenu();
    const std::vector<WShell::Sidebar::Item>& items1 = WShell::Sidebar::GetItems(WShell::Sidebar::Category::C1);
    const std::vector<WShell::Sidebar::Item>& items2 = WShell::Sidebar::GetItems(WShell::Sidebar::Category::C2);
    const std::vector<WShell::Sidebar::Item>& items3 = WShell::Sidebar::GetItems(WShell::Sidebar::Category::C3);

    
};
