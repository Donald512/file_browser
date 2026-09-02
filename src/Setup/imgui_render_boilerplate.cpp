#include "imgui_render_boilerplate.h"
#include "theme.h"
#include "imgui_fonts.h"
#include "MainApp.h"


void InitializeImGui(HWND hwnd, ID3D11Device* pD3dDevice, ID3D11DeviceContext* pD3dContext, f32* dpi){
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    
    ImGuiIO& io = ImGui::GetIO();
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;     // Enable Keyboard Controls
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    ApplyWindows11DarkTheme();
    // Setup scaling based on primary window position
    *dpi = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST));
    
    // Pass the correct font atlas pointer from ImGuiIO
    BuildFonts(*dpi);

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(pD3dDevice, pD3dContext);

}

void ImGui_Backend_NewFrame(){
    // B. Tell ImGui you are starting a new frame
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
}

void MyGraphicsAPI_PresentFrame(const ImVec4& clearColor, ID3D11RenderTargetView* pRenderTargetView, ID3D11DeviceContext* pD3dContext, IDXGISwapChain* pSwapChain, bool* swapChainOccluded){

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


void ShutdownImGui(HWND hwnd, ID3D11Device** ppD3dDevice, ID3D11DeviceContext** ppD3dContext, IDXGISwapChain** ppSwapChain, ID3D11RenderTargetView** ppRenderTargetView, WNDCLASSEXW& wc){
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

void RenderFrame(App& app){
    if (::IsIconic(app.gfx.hwnd)) return;

    if (app.gfx.swapChainOccluded){
        if (app.gfx.swapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) return;                    
        app.gfx.swapChainOccluded = false;  
    }

    // Do the swap-chain resize here too, not just in main()'s loop —
    // during a live drag, main()'s loop never runs, so this is the only
    // place a pending resize will actually get applied before Present.
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
    // ========
    // toggle with F1
    // static bool showMetrics = false;
    // static bool showIDStackTool = false;
    // if (ImGui::IsKeyPressed(ImGuiKey_F1)) showMetrics = !showMetrics;
    // if (ImGui::IsKeyPressed(ImGuiKey_F2)) showIDStackTool = !showIDStackTool;
    // if (showMetrics) ImGui::ShowMetricsWindow();
    // if (showIDStackTool) ImGui::ShowIDStackToolWindow();
    // ========

    // UI::Render(app);
    ImGuiIO& io = ImGui::GetIO();
    GameLoop(app.gfx.width, app.gfx.height, io.MousePos.x, io.MousePos.y, io.MouseDown[0], io.DeltaTime, app.ui.dpi, app, ::IsZoomed(app.gfx.hwnd));

    ImGui::Render();

    app.ProcessCommands();
    app.tasks.RunMainThreadJobs();

    MyGraphicsAPI_PresentFrame(app.ui.clearColor, app.gfx.renderTargetView.Get(), app.gfx.d3dContext.Get(), app.gfx.swapChain.Get(), &app.gfx.swapChainOccluded); 
}