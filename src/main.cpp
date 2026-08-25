#include "KnownSpecialFolders.h"
#include <KnownFolders.h>
#include "Tab.h"
#include "App.h"
#include "deviceCreation.h"
#include "imgui_render_boilerplate.h"

#include "MainApp.h"

#pragma comment(lib, "comctl32.lib")

#pragma comment(lib, "Shell32.lib") 
#pragma comment(lib, "Shlwapi.lib") 
#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "propsys.lib")
#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "OleAut32.lib")

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool g_dpiChanged = false;
f32 g_dpiFromWndproc = 1.0f;

bool g_isResizing = false;
bool g_isMinimized = false;
bool g_appReady = false;

int main (void){
    OleInitialize(nullptr);  // Calls CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    GetSpecialFolders();

    App app{};

    app.window.NewTab(SpecialFolders::defaultStartupFolder);
    app.window.NewTab(SpecialFolders::pidlHome);
    app.window.NewTab(SpecialFolders::pidlQuickAccess);
    app.window.NewTab(SpecialFolders::pidlDownloads);
    app.window.NewTab(SpecialFolders::pidlDesktop);
    app.window.NewTab(SpecialFolders::pidlRecycleBin);
    app.window.NewTab();

    app.sidebar.Init();


    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"File Browser Window", nullptr };


    app.gfx.hwnd = CreateMyOSWindow(wc, &app);
    if (!InitializeGraphicsAPI(app.gfx.hwnd, wc, app.gfx.d3dDevice.GetAddressOf(),app.gfx.d3dContext.GetAddressOf(),app.gfx.swapChain.GetAddressOf(), app.gfx.renderTargetView.GetAddressOf())) return 1;

    app.textures.Init(app.gfx.d3dDevice.Get(), app.gfx.d3dContext.Get());
    
    InitializeImGui(app.gfx.hwnd, app.gfx.d3dDevice.Get() ,app.gfx.d3dContext.Get(), &app.ui.dpi);

    RECT rect;
    ::GetClientRect(app.gfx.hwnd, &rect);

    g_appReady = true;

    ::ShowWindow(app.gfx.hwnd, SW_SHOWMAXIMIZED);
    ::UpdateWindow(app.gfx.hwnd);

    AddClipboardFormatListener(app.gfx.hwnd);


    
    bool running = true;
    while (running) {
        
        // A. Handle Windows events (clicks, closes, moves)
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) running = false;
        }

        if (!running) break;

        // When minimized, skip rendering entirely. Calling Present() on a minimized window with VSync enabled causes the GPU driver to block indefinitely
        if (g_isMinimized) {
            ::Sleep(10); 
            continue;
        }
        if (g_dpiChanged){
            doIfDpiChanges(g_dpiFromWndproc);
            app.ui.dpi = g_dpiFromWndproc;
            g_dpiChanged = false;
        }

        RenderFrame(app);
    }

    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    ShutdownImGui(app.gfx.hwnd, app.gfx.d3dDevice.GetAddressOf() ,app.gfx.d3dContext.GetAddressOf(), app.gfx.swapChain.GetAddressOf(), app.gfx.renderTargetView.GetAddressOf(), wc);
    OleUninitialize();
    printf("Exited succefully\n");
    return 0;

}