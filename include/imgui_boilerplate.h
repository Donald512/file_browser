#pragma once

#include <Windows.h>
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include <d3d11.h>
#include "file_browser.h"



// Data
extern ID3D11Device*            g_pd3dDevice;
extern ID3D11DeviceContext*     g_pd3dDeviceContext;
extern IDXGISwapChain*          g_pSwapChain;
extern bool                     g_SwapChainOccluded;
extern UINT                     g_ResizeWidth, g_ResizeHeight;
extern ID3D11RenderTargetView*  g_mainRenderTargetView;


HWND CreateMyOSWindow();
bool InitializeGraphicsAPI(HWND &window);
void InitializeImGui(HWND &window);
void ImGui_Backend_NewFrame();
void MyGraphicsAPI_PresentFrame();
void ShutdownImGui(HWND &window);
void SetBackgroundColor(float r, float g, float b, float a = 1.0f);
// Forward declarations of helper functions by imgui
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
