// imgui_boilerplate.h

#pragma once

#include "AppContext.h"

bool CreateMyOSWindow(AppContext &ctx, WNDCLASSEXW &wc);
bool InitializeGraphicsAPI(AppContext& ctx, WNDCLASSEXW &wc);
void InitializeImGui(AppContext &ctx);
void ImGui_Backend_NewFrame();
void MyGraphicsAPI_PresentFrame(AppContext& ctx);
void ShutdownImGui(AppContext& ctx, WNDCLASSEXW& wc);
bool CreateDeviceD3D(AppContext& ctx);
void CleanupDeviceD3D(AppContext& ctx);
void CreateRenderTarget(AppContext& ctx);
void CleanupRenderTarget(AppContext& ctx);


LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);