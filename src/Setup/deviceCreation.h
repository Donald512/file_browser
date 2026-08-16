// imgui_boilerplate.h

#pragma once

#include <d3d11.h>
#include "imgui_impl_win32.h"
#include <windowsx.h>
#include <cstdio>


inline HWND CreateMyOSWindow(WNDCLASSEXW &wc, App* appInstance){
    ImGui_ImplWin32_EnableDpiAwareness();
    HWND hwnd{};

    if (::RegisterClassExW(&wc) == 0){
        printf("Register Class failed");
        return hwnd;
    }
    // pass Address of ctx as final param (lpParam), this is to allow us to pass AppContext into WndProc
    hwnd = ::CreateWindowW(wc.lpszClassName, L"File Browser", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,CW_USEDEFAULT ,CW_USEDEFAULT,   nullptr, nullptr, wc.hInstance, appInstance); 
    if (!hwnd){
        printf("Create Window failed");
    }

    return hwnd;
}

void CleanupRenderTarget(ID3D11RenderTargetView** ppRenderTargetView){
    if (ppRenderTargetView && *ppRenderTargetView) {
        (*ppRenderTargetView)->Release();
        *ppRenderTargetView = nullptr;
    }
}

HRESULT CreateRenderTarget(IDXGISwapChain* pSwapChain, ID3D11Device* pD3dDevice, ID3D11RenderTargetView** ppRenderTargetView){
    ID3D11Texture2D* pBackBuffer;
    HRESULT res = pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (FAILED(res)) return res;
    res = pD3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, ppRenderTargetView);
    pBackBuffer->Release(); 
    return res;
}

HRESULT CreateDeviceD3D(HWND hwnd, ID3D11Device** ppD3dDevice, ID3D11DeviceContext** ppD3dContext, IDXGISwapChain** ppSwapChain, ID3D11RenderTargetView** ppRenderTargetView){
    if (!hwnd || !ppD3dDevice || !ppD3dContext || !ppSwapChain || !ppRenderTargetView){
        printf("CreateDeviceD3D recieved a null or invalid pointer");
        return E_POINTER;
    }
    // Setup swap chain
    // This is a basic setup. Optimally could use e.g. DXGI_SWAP_EFFECT_FLIP_DISCARD and handle fullscreen mode differently. See #8979 for suggestions.
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;

    D3D_FEATURE_LEVEL featureLevel;

    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, ppSwapChain, ppD3dDevice, &featureLevel, ppD3dContext);

    // Try high-performance WARP software driver if hardware is not available.
    if (res == DXGI_ERROR_UNSUPPORTED){
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, ppSwapChain, ppD3dDevice, &featureLevel, ppD3dContext);
    } 
    if (FAILED(res)) return res;

    return CreateRenderTarget(*ppSwapChain, *ppD3dDevice, ppRenderTargetView);
}


void CleanupDeviceD3D(ID3D11Device** ppD3dDevice, ID3D11DeviceContext** ppD3dContext, IDXGISwapChain** ppSwapChain, ID3D11RenderTargetView** ppRenderTargetView){
    
    CleanupRenderTarget(ppRenderTargetView);
    if (ppSwapChain && *ppSwapChain) { (*ppSwapChain)->Release(); *ppSwapChain = nullptr; }
    if (ppD3dContext && *ppD3dContext) { (*ppD3dContext)->Release(); *ppD3dContext = nullptr; }
    if (ppD3dDevice && *ppD3dDevice) { (*ppD3dDevice)->Release(); *ppD3dDevice = nullptr; }
}


inline bool InitializeGraphicsAPI(HWND hwnd, WNDCLASSEXW& wc, ID3D11Device** ppD3dDevice, ID3D11DeviceContext** ppD3dContext, IDXGISwapChain** ppSwapChain, ID3D11RenderTargetView** ppRenderTargetView){
    // Initialize Direct3D
    if (FAILED(CreateDeviceD3D(hwnd, ppD3dDevice, ppD3dContext, ppSwapChain, ppRenderTargetView))){
        CleanupDeviceD3D(ppD3dDevice, ppD3dContext, ppSwapChain, ppRenderTargetView);
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return false;
    }
    return true;
}

