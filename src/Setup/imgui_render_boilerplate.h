#pragma once

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include <WinFramework.h>
#include "deviceCreation.h"
#include "BasicTypes.h"
#include "App.h"



struct GraphicsResources {
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    IDXGISwapChain* swapChain = nullptr;
    ID3D11RenderTargetView* rtv = nullptr;
    UINT width = 0;
    UINT height = 0;
    bool resizeRequested = false;
};

void InitializeImGui(HWND hwnd, ID3D11Device* pD3dDevice, ID3D11DeviceContext* pD3dContext, f32* dpi);

void ImGui_Backend_NewFrame();

void MyGraphicsAPI_PresentFrame(const ImVec4& clearColor, ID3D11RenderTargetView* pRenderTargetView, ID3D11DeviceContext* pD3dContext, IDXGISwapChain* pSwapChain, bool* swapChainOccluded);


void ShutdownImGui(HWND hwnd, ID3D11Device** ppD3dDevice, ID3D11DeviceContext** ppD3dContext, IDXGISwapChain** ppSwapChain, ID3D11RenderTargetView** ppRenderTargetView, WNDCLASSEXW& wc);

void doIfDpiChanges(f32 dpi);

void RenderFrame(App& app);