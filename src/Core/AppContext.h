#pragma once

#include "Types.h"
#include "Shell.h"
#include "Navigation.h"
#include <wrl/client.h>

#include <d3d11.h>
#include <imgui.h>
#include <ShlObj.h>
#include <string>
#include <vector>
#include "..\Icons\icons.h"


using Microsoft::WRL::ComPtr;


struct AppContext{
    // Windows & Graphics
    HWND hwnd = nullptr;
    ComPtr<ID3D11Device> d3dDevice;
    ComPtr<ID3D11DeviceContext> d3dContext;
    ComPtr<IDXGISwapChain> swapChain;
    ComPtr<ID3D11RenderTargetView> renderTargetView;

    bool swapChainOccluded = false;
    UINT resizeWidth = 0;
    UINT resizeHeight = 0;

    // UI State
    f32 dpiScale = 1.0f;
    ImFont* mainFont = nullptr;   // non-owning — ImGui's font atlas owns these
    ImFont* iconFont = nullptr;
    ImVec4 clearColor = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);

    // Navigation State
    Navigation::NavigationController navigation;
    Icons::IconManager icons;

    // special pidls
   WShell::Pidl pidlThisPC;
   WShell::Pidl pidlHome;
   WShell::Pidl pidlDesktop;

};