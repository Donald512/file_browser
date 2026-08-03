// imgui_boilerplate.cpp

#include <d3d11.h>
#include "WinFramework.h"
#include "imgui_boilerplate.h"
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
#include "WinFramework.h"  
#include "ImGuiTheme.h" // For ApplyWindows11DarkTheme
#include "Core.h"
#include <windowsx.h>


bool CreateMyOSWindow(AppContext &ctx, WNDCLASSEXW &wc){
    ImGui_ImplWin32_EnableDpiAwareness();

    if (::RegisterClassExW(&wc) == 0){
        printf("Register Class failed");
        return false;
    }
    // pass Address of ctx as final param (lpParam), this is to allow us to pass AppContext into WndProc
    ctx.gfx.hwnd = ::CreateWindowW(wc.lpszClassName, L"File Browser", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,CW_USEDEFAULT ,CW_USEDEFAULT,   nullptr, nullptr, wc.hInstance, &ctx); 
    if (!ctx.gfx.hwnd){
        printf("Create Window failed");
    }

    return ctx.gfx.hwnd != nullptr;
}

bool InitializeGraphicsAPI(AppContext& ctx, WNDCLASSEXW &wc){
    // Initialize Direct3D
    if (!CreateDeviceD3D(ctx)){
        CleanupDeviceD3D(ctx);
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return false;
    }
    return true;
}

void InitializeImGui(AppContext &ctx){
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    ApplyWindows11DarkTheme();
    // Setup scaling based on primary window position
    ctx.ui.dpiScale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTONEAREST));
    
    // Pass the correct font atlas pointer from ImGuiIO
    BuildFonts(ctx, io.Fonts);

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(ctx.gfx.hwnd);
    ImGui_ImplDX11_Init(ctx.gfx.d3dDevice.Get(), ctx.gfx.d3dContext.Get());

}

void ImGui_Backend_NewFrame(){
    // B. Tell ImGui you are starting a new frame
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
}

void MyGraphicsAPI_PresentFrame(AppContext& ctx){
    // Rendering
    // 1. Calculate the raw triangle data (Step D placeholder 1)
    ImGui::Render();

    // 2. Prep your clear color (handles alpha blending math)
    ImVec4 clear_color = ctx.ui.clearColor;
    const float clear_color_with_alpha[4] = {clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };

    // 3. Tell your GPU to target your main window view
    ID3D11RenderTargetView* rtv = ctx.gfx.renderTargetView.Get();
    ctx.gfx.d3dContext->OMSetRenderTargets(1, &rtv, nullptr);

    // 4. Wipe the previous frame's pixels off the screen using your clear color
    ctx.gfx.d3dContext->ClearRenderTargetView(ctx.gfx.renderTargetView.Get(), clear_color_with_alpha);

    // 5. Hand the calculated ImGui triangles over to DirectX 11 to draw them
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    HRESULT hr = ctx.gfx.swapChain->Present(1, 0); // 1 = Lock to your monitor's VSync refresh rate
    ctx.gfx.swapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
}


void ShutdownImGui(AppContext& ctx, WNDCLASSEXW& wc){
    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D(ctx);
    ::DestroyWindow(ctx.gfx.hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
}


void SetBackgroundColor(AppContext& ctx, float r, float g, float b, float a) {      // think this function is useless
    ctx.ui.clearColor = ImVec4(r, g, b, a);    
}


// !!!!!!---------------------------------------------------------------------------------------

bool CreateDeviceD3D(AppContext& ctx){
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
    sd.OutputWindow = ctx.gfx.hwnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &ctx.gfx.swapChain, &ctx.gfx.d3dDevice, &featureLevel, &ctx.gfx.d3dContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &ctx.gfx.swapChain, &ctx.gfx.d3dDevice, &featureLevel, &ctx.gfx.d3dContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget(ctx);
    return true;
}

void CleanupDeviceD3D(AppContext& ctx){
    CleanupRenderTarget(ctx);
    ctx.gfx.swapChain.Reset();
    ctx.gfx.d3dContext.Reset();
    ctx.gfx.d3dDevice.Reset();
}

void CreateRenderTarget(AppContext& ctx){
    ID3D11Texture2D* pBackBuffer;
    ctx.gfx.swapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    ctx.gfx.d3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &ctx.gfx.renderTargetView);
    pBackBuffer->Release(); 
}

void CleanupRenderTarget(AppContext& ctx){
    ctx.gfx.renderTargetView.Reset();
}




// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam){
    // Get the pointer if it's already been set
    AppContext* ctx = reinterpret_cast<AppContext*>(::GetWindowLongPtrW(hWnd, GWLP_USERDATA));

    //  If it hasn't been set yet, look for the creation message
    if (msg == WM_NCCREATE) {
        CREATESTRUCTW* pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
        ctx = reinterpret_cast<AppContext*>(pCreate->lpCreateParams);
        
        // Glue the pointer directly onto this specific HWND instance
        ::SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ctx));
    }

    // Safety check: early messages might fly by before WM_NCCREATE completes
    if (ctx == nullptr) {
        return ::DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg){
    case WM_NCLBUTTONDOWN:{ // when user presses one of the caption buton regions, windows generates a WM_NCLBUTTONDOWN message, this is handle to prevent the retro white box from rendering
        switch(wParam){
        case HTMINBUTTON:{
            ::PostMessage(hWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
            return 0; // tell windows i completely handled the message, so it doesnt render the accessibilty box
        }
        case HTMAXBUTTON:{
            ::IsZoomed(hWnd) ? ::PostMessage(hWnd, WM_SYSCOMMAND, SC_RESTORE, 0) : ::PostMessage(hWnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
            return 0;
        }
        case HTCLOSE:{
            ::PostMessage(hWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
            return 0;
        }
        break;
        }
    }   break;
    case WM_NCCALCSIZE :{
        if (wParam == TRUE){
            // lParam points to an NCCALCSIZE_PARAMS structure when wParam is TRUE
            NCCALCSIZE_PARAMS* pParams = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);

            // If the window is maximized, clamp its drawing area to the monitor's exact work area
            // This prevents Windows from pushing the top 8 pixels off the screen!
            if (::IsZoomed(hWnd)) {
                HMONITOR hMonitor = ::MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
                MONITORINFO mi = { sizeof(mi) };
                if (::GetMonitorInfoW(hMonitor, &mi)) {
                    pParams->rgrc[0] = mi.rcWork; // rcWork respects the Windows Taskbar
                }
            }
            return 0;   // Keep returning 0 to remove the default ugly title bar
        }
    } break;
    case WM_DPICHANGED:{
        UINT newDpi = HIWORD(wParam);
        ctx->ui.dpiScale = (f32) newDpi / 96.0f;

        RECT* prcNewWindow = reinterpret_cast<RECT*>(lParam);
        ::SetWindowPos(hWnd, nullptr, prcNewWindow->left, prcNewWindow->top,  prcNewWindow->right - prcNewWindow->left, prcNewWindow->bottom - prcNewWindow->top, SWP_NOZORDER | SWP_NOACTIVATE);
        
        // Restore the pristine baseline layout values before rebuilding
        ImGuiStyle baselineStyle;
        ImGui::GetStyle() = baselineStyle; 
        ApplyWindows11DarkTheme(); // Re-apply colors because the reset wiped them!

        // Tell DX11 to release the old texture allocation handles on the GPU
        ImGui_ImplDX11_InvalidateDeviceObjects();

        // Re-bake fonts on the CPU using the new scale
        ImGuiIO& io = ImGui::GetIO();
        BuildFonts(*ctx, io.Fonts);

        int w, h;
        unsigned char* pixels;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &w, &h);
        printf("Font atlas: %d x %d = %.1f MB\n", w, h, (w * h * 4) / (1024.0f * 1024.0f));

        // Force DX11 to upload the brand-new scaled font sheet to the GPU
        ImGui_ImplDX11_CreateDeviceObjects();
    }   break;
    case WM_NCHITTEST :{

        // 1. Get the screen mouse positions from lParam
        POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)}; // LOWORD lparam is mouse x and HIWORD lparam is mouse y
        ::ScreenToClient(hWnd, &pt);

        // window dimensions
        RECT rect;
        ::GetClientRect(hWnd, &rect);

        
        f32 topBarHeight = 32.0f * ctx->ui.dpiScale;
        LONG borderThickness = (LONG) ( 8.0 * ctx->ui.dpiScale);
        
        RECT innerRect = {borderThickness, borderThickness, rect.right - borderThickness, rect.bottom - borderThickness};

        // only resize if not maximized
        if (!::IsZoomed(hWnd)){
            // must check diagonals first
            if (pt.x < innerRect.left && pt.y < innerRect.top) return HTTOPLEFT;
            else if (pt.x >= innerRect.right && pt.y < innerRect.top) return HTTOPRIGHT;
            else if (pt.x < innerRect.left && pt.y >= innerRect.bottom) return HTBOTTOMLEFT;
            else if (pt.x >= innerRect.right && pt.y >= innerRect.bottom) return HTBOTTOMRIGHT;
    
            // check straight edges
            else if (pt.x < innerRect.left) return HTLEFT;
            else if (pt.y < innerRect.top) return HTTOP;
            else if (pt.x >= innerRect.right) return HTRIGHT;
            else if (pt.y >= innerRect.bottom) return HTBOTTOM;
        }
        
        if (pt.y < topBarHeight){

            f32 windowWidth = (f32)(rect.right - rect.left);
            f32 clsBtnWidth = 46.0f;
            f32 maxBtnWidth = 45.0f;
            f32 minBtnWidth = 45.0f;
            f32 controlClusterWidth = (clsBtnWidth + maxBtnWidth + minBtnWidth) * ctx->ui.dpiScale;
            f32 buttonStartX = windowWidth - controlClusterWidth;   // 136px total
            
            if (pt.x >= buttonStartX ){
                f32 captionRelativePosition = pt.x - buttonStartX;

                if (captionRelativePosition < minBtnWidth){
                    return HTMINBUTTON;     // minimize
                }
                else if (captionRelativePosition < (minBtnWidth + maxBtnWidth)){
                    return HTMAXBUTTON;     // maximize
                }
                else{
                    return HTCLOSE;
                }
            }
            // else tell windows this is the title bar
            return HTCAPTION;
        }
        return HTCLIENT;    // todo check is this is redundant
    } 
    break;
    
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        ctx->gfx.resizeWidth = (UINT)LOWORD(lParam); // Queue resize
        ctx->gfx.resizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_CLIPBOARDUPDATE:{
        QueryClipBoardCutItems(*ctx);
    }
    break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

// TODO: Replace WaitMessage() with MsgWaitForMultipleObjects 
// and a 1-second Linger Timer to fix text cursor blinking and UI fade animations.

/*
Background Threading: Loading a folder with 10,000 files without freezing the UI.
Thumbnail Generation: Extracting icons and images for files efficiently.
File Operations: Copy, Paste, Delete, and handling Windows permission errors gracefully.
Navigation State: Handling drag-and-drop, tree-view expansion, and complex path parsing.
*/