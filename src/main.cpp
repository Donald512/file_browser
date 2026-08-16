#include "KnownSpecialFolders.h"
#include <KnownFolders.h>
#include "Tab.h"
#include "App.h"
#include "deviceCreation.h"
#include "imgui_render_boilerplate.h"

#include "WndprocHandler.h"
#include "MainApp.h"

#pragma comment(lib, "comctl32.lib")

#pragma comment(lib, "Shell32.lib") 
#pragma comment(lib, "Shlwapi.lib") 
#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "propsys.lib")
#pragma comment(lib, "Ole32.lib")


int main (void){
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

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

    ::ShowWindow(app.gfx.hwnd, SW_SHOWMAXIMIZED);
    ::UpdateWindow(app.gfx.hwnd);

    InitializeImGui(app.gfx.hwnd, app.gfx.d3dDevice.Get() ,app.gfx.d3dContext.Get(), &app.ui.dpi);
    
    RECT rect;
    ::GetClientRect(app.gfx.hwnd, &rect);
    f32 initialWidth = (f32)(rect.right - rect.left);
    f32 initialHeight = (f32)(rect.bottom - rect.top);
    InitializeUI(initialWidth, initialHeight);
    
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
        app.gfx.swapChainOccluded = false;
        if (g_dpiChanged){
            doIfDpiChanges(g_dpiFromWndproc);
            app.ui.dpi = g_dpiFromWndproc;
            g_dpiChanged = false;
        }

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (app.gfx.resizeWidth != 0 && app.gfx.resizeHeight != 0){

            // Unbind the render target from the context !!!
            ID3D11RenderTargetView* nullRTV = nullptr;
            app.gfx.d3dContext->OMSetRenderTargets(1, &nullRTV, nullptr);

            CleanupRenderTarget(app.gfx.renderTargetView.GetAddressOf());

            app.gfx.swapChain->ResizeBuffers(0, app.gfx.resizeWidth, app.gfx.resizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            app.gfx.resizeWidth = app.gfx.resizeHeight = 0;

            CreateRenderTarget(app.gfx.swapChain.Get(), app.gfx.d3dDevice.Get(), app.gfx.renderTargetView.GetAddressOf());
        }

        app.textures.NextFrame(); 
        ImGui_Backend_NewFrame();
        ImGui::NewFrame();
        // ImGui::ShowMetricsWindow();

        // UI::Render(app);
        ImGuiIO& io = ImGui::GetIO();
        GameLoop(app.gfx.width, app.gfx.height, io.MousePos.x, io.MousePos.y, io.MouseDown[0], io.DeltaTime, app.ui.dpi, app, ::IsZoomed(app.gfx.hwnd));

        ImGui::Render();

        app.ProcessCommands();

        MyGraphicsAPI_PresentFrame(app.ui.clearColor, app.gfx.renderTargetView.Get(), app.gfx.d3dContext.Get(), app.gfx.swapChain.Get(), &app.gfx.swapChainOccluded); 
    }

    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    ShutdownImGui(app.gfx.hwnd, app.gfx.d3dDevice.GetAddressOf() ,app.gfx.d3dContext.GetAddressOf(), app.gfx.swapChain.GetAddressOf(), app.gfx.renderTargetView.GetAddressOf(), wc);
    CoUninitialize();
    printf("Exited succefully\n");
    return 0;

}