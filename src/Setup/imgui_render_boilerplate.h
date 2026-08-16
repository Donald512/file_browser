#pragma once

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include <windows.h>
#include "deviceCreation.h"
#include "BasicTypes.h"
#include "imgui_fonts.h"
#include "theme.h"



struct GraphicsResources {
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    IDXGISwapChain* swapChain = nullptr;
    ID3D11RenderTargetView* rtv = nullptr;
    UINT width = 0;
    UINT height = 0;
    bool resizeRequested = false;
};

inline void InitializeImGui(HWND hwnd, ID3D11Device* pD3dDevice, ID3D11DeviceContext* pD3dContext, f32* dpi){
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    ApplyWindows11DarkTheme();
    // Setup scaling based on primary window position
    *dpi = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST));
    
    // Pass the correct font atlas pointer from ImGuiIO
    BuildFonts(*dpi);

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(pD3dDevice, pD3dContext);

}

inline void ImGui_Backend_NewFrame(){
    // B. Tell ImGui you are starting a new frame
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
}

inline void MyGraphicsAPI_PresentFrame(const ImVec4& clearColor, ID3D11RenderTargetView* pRenderTargetView, ID3D11DeviceContext* pD3dContext, IDXGISwapChain* pSwapChain, bool* swapChainOccluded){

    // 1. Calculate the raw triangle data (Step D placeholder 1)
    ImGui::Render();

    ImVec4 clear_color = clearColor;
    const float clear_color_with_alpha[4] = {clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };

    ID3D11RenderTargetView* rtv = pRenderTargetView;
    pD3dContext->OMSetRenderTargets(1, &rtv, nullptr);

    pD3dContext->ClearRenderTargetView(pRenderTargetView, clear_color_with_alpha);

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    HRESULT hr = pSwapChain->Present(1, 0); // 1 = Lock to your monitor's VSync refresh rate
    *swapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
}


inline void ShutdownImGui(HWND hwnd, ID3D11Device** ppD3dDevice, ID3D11DeviceContext** ppD3dContext, IDXGISwapChain** ppSwapChain, ID3D11RenderTargetView** ppRenderTargetView, WNDCLASSEXW& wc){
    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D(ppD3dDevice, ppD3dContext, ppSwapChain, ppRenderTargetView);
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
}

void doIfDpiChanges(f32 dpi){
    BuildFonts(dpi);
    ImGuiStyle baselineStyle;
    ImGui::GetStyle() = baselineStyle; 
    ApplyWindows11DarkTheme(); // Re-apply colors because the reset wiped them!
    
    // Tell DX11 to release the old texture allocation handles on the GPU
    ImGui_ImplDX11_InvalidateDeviceObjects();
    
    // Force DX11 to upload the brand-new scaled font sheet to the GPU
    ImGui_ImplDX11_CreateDeviceObjects();
}


// !!!!!!---------------------------------------------------------------------------------------

