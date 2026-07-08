// imgui_boilerplate.h

#pragma once

#include <Windows.h>
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include <d3d11.h>
#include "core.h"
#include <windowsx.h>

bool CreateMyOSWindow(AppContext &ctx, WNDCLASSEXW &wc);
bool InitializeGraphicsAPI(AppContext& ctx, WNDCLASSEXW &wc);
void InitializeImGui(AppContext &ctx);
static void BuildFonts(AppContext& ctx, ImFontAtlas* atlas);
void ApplyWindows11DarkTheme();
void ImGui_Backend_NewFrame();
void MyGraphicsAPI_PresentFrame(AppContext& ctx);
void ShutdownImGui(AppContext& ctx, WNDCLASSEXW& wc);
void SetBackgroundColor(AppContext& ctx, float r, float g, float b, float a);
bool CreateDeviceD3D(AppContext& ctx);
void CleanupDeviceD3D(AppContext& ctx);
void CreateRenderTarget(AppContext& ctx);
void CleanupRenderTarget(AppContext& ctx);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);