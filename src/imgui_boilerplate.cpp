// imgu i_boilerplate.cpp


#include "imgui_boilerplate.h"
#include "renderer.h"
#include <windowsx.h>


///////////////////////
// Global Definitions
ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
bool                    g_SwapChainOccluded = false;
UINT                    g_ResizeWidth = 0;   // Don't forget this one, it was in your header!
UINT                    g_ResizeHeight = 0;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
ImVec4 g_clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
HWND g_hwnd;
f32 g_DpiScale = 1.0f;
ImFont* g_MainFont = nullptr;
ImFont* g_IconFont = nullptr;
///////////////////////

HWND CreateMyOSWindow(){
    // Make process DPI aware
    ImGui_ImplWin32_EnableDpiAwareness();

    
    // Create application window
    // ! changed TEXT() to L""
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"File Browser Window", nullptr };
    if (::RegisterClassExW(&wc) == 0){
        printf("Register Class failed");
        return false;
    }

    // ! changed TEXT() to L"" and used wide version
    // DWORD dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    // HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"File Browser", WS_OVERLAPPEDWINDOW, 100, 100, (int)(1280 * main_scale), (int)(800 * main_scale), nullptr, nullptr, wc.hInstance, nullptr);
    g_hwnd = ::CreateWindowW(wc.lpszClassName, L"File Browser", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,CW_USEDEFAULT ,CW_USEDEFAULT,   nullptr, nullptr, wc.hInstance, nullptr);
    
    return g_hwnd;
}

bool InitializeGraphicsAPI(HWND &window){
    // Initialize Direct3D
    if (!CreateDeviceD3D(window))
    {
        CleanupDeviceD3D();
        // todo ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return false;
    }
    return true;
}

void InitializeImGui(HWND &window){
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    
    // Setup scaling
    f32 main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTONEAREST));
    g_DpiScale = main_scale;
    

    // Base font (text)
    g_MainFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 16.0f * g_DpiScale);

    // Fluent Icons (buttons)
    ImFontConfig config;
    config.MergeMode = true;
    config.PixelSnapH = true;
    config.GlyphMinAdvanceX = 16.0f * g_DpiScale; // ensure icons are at least this wide
    
    static const ImWchar icon_ranges[] = { (ImWchar) ICON_MIN_REG, (ImWchar) ICON_MAX_REG, 0};
    g_IconFont = io.Fonts->AddFontFromFileTTF("thirdparty\\fontstuff\\FluentSystemIcons-Regular.ttf", 10.0f * g_DpiScale, &config, icon_ranges);

    // Emoji fallback
    ImFontConfig emoji_config;
    emoji_config.MergeMode = true;
    emoji_config.FontDataOwnedByAtlas = false;

    static const ImWchar32 emoji_ranges[] = {
            0x2000, 0x206F,   // General Punctuation
            0x2600, 0x26FF,   // Misc Symbols (Sun, Moon, etc)
            0x2700, 0x27BF,   // Dingbats
            0x1F300, 0x1F64F, // Emojis & Pictographs
            0x1F680, 0x1F6FF, // Transport & Map
            0x1F900, 0x1F9FF, // Supplemental Emojis
            0
        };

    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\seguiemj.ttf", 16.0f * g_DpiScale, &emoji_config, emoji_ranges);


    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    ApplyWindows11DarkTheme();
}

void RebuildFontAtlas(){
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();

    // Base font (text)
    g_MainFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 16.0f * g_DpiScale);

    // Fluent Icons (buttons)
    ImFontConfig config;
    config.MergeMode = true;
    config.PixelSnapH = true;
    config.GlyphMinAdvanceX = 16.0f * g_DpiScale; // ensure icons are at least this wide
    
    static const ImWchar icon_ranges[] = { (ImWchar) ICON_MIN_REG, (ImWchar) ICON_MAX_REG, 0};
    g_IconFont = io.Fonts->AddFontFromFileTTF("thirdparty\\fontstuff\\FluentSystemIcons-Regular.ttf", 12.0f * g_DpiScale, &config, icon_ranges);

    // Emoji fallback
    ImFontConfig emoji_config;
    emoji_config.MergeMode = true;
    emoji_config.FontDataOwnedByAtlas = false;

    static const ImWchar32 emoji_ranges[] = {
            0x2000, 0x206F,   // General Punctuation
            0x2600, 0x26FF,   // Misc Symbols (Sun, Moon, etc)
            0x2700, 0x27BF,   // Dingbats
            0x1F300, 0x1F64F, // Emojis & Pictographs
            0x1F680, 0x1F6FF, // Transport & Map
            0x1F900, 0x1F9FF, // Supplemental Emojis
            0
        };

    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\seguiemj.ttf", 16.0f * g_DpiScale, &emoji_config, emoji_ranges);


    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(g_DpiScale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = g_DpiScale;        // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)

}

void ImGui_Backend_NewFrame(){
    // B. Tell ImGui you are starting a new frame
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
}

void MyGraphicsAPI_PresentFrame(){
    // Rendering
    // 1. Calculate the raw triangle data (Step D placeholder 1)
    ImGui::Render();

    // 2. Prep your clear color (handles alpha blending math)
    ImVec4 clear_color = g_clear_color;
    const float clear_color_with_alpha[4] = {clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };

    // 3. Tell your GPU to target your main window view
    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);

    // 4. Wipe the previous frame's pixels off the screen using your clear color
    g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);

    // 5. Hand the calculated ImGui triangles over to DirectX 11 to draw them
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    HRESULT hr = g_pSwapChain->Present(1, 0); // 1 = Lock to your monitor's VSync refresh rate
    g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
}


void ShutdownImGui(HWND &window){
    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(window);
    // todo ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
}


void SetBackgroundColor(float r, float g, float b, float a) {
    g_clear_color = ImVec4(r, g, b, a);
}


// !!!!!!---------------------------------------------------------------------------------------

bool CreateDeviceD3D(HWND hWnd){
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
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D(){
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget(){
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
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
        g_DpiScale = (f32) newDpi / 96.0f;

        RECT* prcNewWindow = reinterpret_cast<RECT*>(lParam);
        ::SetWindowPos(hWnd, nullptr, prcNewWindow->left, prcNewWindow->top,  prcNewWindow->right - prcNewWindow->left, prcNewWindow->bottom - prcNewWindow->top, SWP_NOZORDER | SWP_NOACTIVATE);
        
        //  Tell DX11 to release the old texture allocation handles on the GPU
        ImGui_ImplDX11_InvalidateDeviceObjects();

        // Re-bake fonts on the CPU
        RebuildFontAtlas();

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

        
        f32 topBarHeight = 32.0f * g_DpiScale;
        LONG borderThickness = (LONG) ( 8.0 * g_DpiScale);
        
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
            f32 controlClusterWidth = (clsBtnWidth + maxBtnWidth + minBtnWidth) * g_DpiScale;
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
    } break;
    
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
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