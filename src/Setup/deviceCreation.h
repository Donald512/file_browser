// imgui_boilerplate.h

#pragma once

#include <d3d11.h>
#include "imgui_impl_win32.h"
#include <windowsx.h>
#include <cstdio>

struct App;

HWND CreateMyOSWindow(WNDCLASSEXW &wc, App* appInstance);

void CleanupRenderTarget(ID3D11RenderTargetView** ppRenderTargetView);

HRESULT CreateRenderTarget(IDXGISwapChain* pSwapChain, ID3D11Device* pD3dDevice, ID3D11RenderTargetView** ppRenderTargetView);

HRESULT CreateDeviceD3D(HWND hwnd, ID3D11Device** ppD3dDevice, ID3D11DeviceContext** ppD3dContext, IDXGISwapChain** ppSwapChain, ID3D11RenderTargetView** ppRenderTargetView);


void CleanupDeviceD3D(ID3D11Device** ppD3dDevice, ID3D11DeviceContext** ppD3dContext, IDXGISwapChain** ppSwapChain, ID3D11RenderTargetView** ppRenderTargetView);


bool InitializeGraphicsAPI(HWND hwnd, WNDCLASSEXW& wc, ID3D11Device** ppD3dDevice, ID3D11DeviceContext** ppD3dContext, IDXGISwapChain** ppSwapChain, ID3D11RenderTargetView** ppRenderTargetView);