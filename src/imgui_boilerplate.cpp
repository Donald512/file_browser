// imgui_boilerplate.cpp
#include "imgui_boilerplate.h"

bool CreateMyOSWindow(AppContext &ctx, WNDCLASSEXW &wc){
    ImGui_ImplWin32_EnableDpiAwareness();

    if (::RegisterClassExW(&wc) == 0){
        printf("Register Class failed");
        return false;
    }
    // pass Address of ctx as final param (lpParam), this is to allow us to pass AppContext into WndProc
    ctx.hwnd = ::CreateWindowW(wc.lpszClassName, L"File Browser", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,CW_USEDEFAULT ,CW_USEDEFAULT,   nullptr, nullptr, wc.hInstance, &ctx); 
    if (!ctx.hwnd){
        printf("Create Window failed");
    }

    return ctx.hwnd != nullptr;
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
    ctx.dpiScale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTONEAREST));
    
    // Pass the correct font atlas pointer from ImGuiIO
    BuildFonts(ctx, io.Fonts);

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(ctx.hwnd);
    ImGui_ImplDX11_Init(ctx.d3dDevice, ctx.d3dContext);

    // todo ApplyWindows11DarkTheme();
}

static void BuildFonts(AppContext& ctx, ImFontAtlas* atlas){
    if (atlas == nullptr) return;

    atlas->Clear();

    // 1. Base Font (Text)
    ctx.mainFont = atlas->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 16.0f * ctx.dpiScale);

    // 2. Fluent Icons (Merged directly into Base Font)
    ImFontConfig icon_config;
    icon_config.MergeMode = true;
    icon_config.GlyphOffset.y = 2.0f* ctx.dpiScale; // ion know why, but it put the chevrons on the same line as the breadcrumbs
    icon_config.PixelSnapH = true;
    icon_config.GlyphMinAdvanceX = 16.0f * ctx.dpiScale; 
    
    static const ImWchar icon_ranges[] = { (ImWchar)ICON_MIN_REG, (ImWchar)ICON_MAX_REG, 0 };
    ctx.iconFont = atlas->AddFontFromFileTTF("thirdparty\\fontstuff\\FluentSystemIcons-Regular.ttf", 12.0f * ctx.dpiScale, &icon_config, icon_ranges);

    // 3. Emoji Fallback (Merged directly into Base Font)
    ImFontConfig emoji_config;
    emoji_config.MergeMode = true;
    emoji_config.FontDataOwnedByAtlas = false; // Prevents ImGui from attempting to call free() on system file data

    static const ImWchar32 emoji_ranges[] = {
        0x2000, 0x206F,   // General Punctuation
        0x2600, 0x26FF,   // Misc Symbols (Sun, Moon, etc)
        0x2700, 0x27BF,   // Dingbats
        0x1F300, 0x1F64F, // Emojis & Pictographs
        0x1F680, 0x1F6FF, // Transport & Map
        0x1F900, 0x1F9FF, // Supplemental Emojis
        0
    };

    atlas->AddFontFromFileTTF("C:\\Windows\\Fonts\\seguiemj.ttf", 16.0f * ctx.dpiScale, &emoji_config, emoji_ranges);

    // 4. DPI Style Scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(ctx.dpiScale);    
    style.FontScaleDpi = ctx.dpiScale;    
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
    ImVec4 clear_color = ctx.clearColor;
    const float clear_color_with_alpha[4] = {clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };

    // 3. Tell your GPU to target your main window view
    ctx.d3dContext->OMSetRenderTargets(1, &ctx.renderTargetView, nullptr);

    // 4. Wipe the previous frame's pixels off the screen using your clear color
    ctx.d3dContext->ClearRenderTargetView(ctx.renderTargetView, clear_color_with_alpha);

    // 5. Hand the calculated ImGui triangles over to DirectX 11 to draw them
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    HRESULT hr = ctx.swapChain->Present(1, 0); // 1 = Lock to your monitor's VSync refresh rate
    ctx.swapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
}


void ShutdownImGui(AppContext& ctx, WNDCLASSEXW& wc){
    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D(ctx);
    ::DestroyWindow(ctx.hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
}


void SetBackgroundColor(AppContext& ctx, float r, float g, float b, float a) {      // think this function is useless
    ctx.clearColor = ImVec4(r, g, b, a);    
}


void ApplyWindows11DarkTheme(){
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // - - - Windown and Panel Layout Properties 
    style.WindowRounding    = 7.0f;  // Windows 11 signature slightly rounded corners
    style.FrameRounding     = 4.0f;
    style.PopupRounding     = 6.0f;
    style.ChildRounding     = 4.0f;
    style.WindowBorderSize  = 1.0f;  // Thin borders separating panels
    style.ChildBorderSize   = 1.0f;
    style.FrameBorderSize   = 1.0f;
    style.ItemSpacing       = ImVec2(8.0f, 6.0f);

    // --- 2. The Exact Windows 11 Color Tokens ---
    // Top-level app background (Mica / Acrylic dark slate)
    colors[ImGuiCol_WindowBg]             = ImVec4(0.12f, 0.12f, 0.12f, 1.00f); // #1E1E1E
    
    // Left Sidebar / Navigation Pane (Slightly darker for depth)
    colors[ImGuiCol_ChildBg]              = ImVec4(0.10f, 0.10f, 0.10f, 1.00f); // #1A1A1A
    
    // Popups, Dropdowns, and Dialogs
    colors[ImGuiCol_PopupBg]              = ImVec4(0.14f, 0.14f, 0.14f, 0.98f); // #242424

    // Subtle divider lines and panel borders
    colors[ImGuiCol_Border]               = ImVec4(0.18f, 0.18f, 0.18f, 1.00f); // #2E2E2E
    colors[ImGuiCol_BorderShadow]         = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    // Text (Off-white / Light grey)
    colors[ImGuiCol_Text]                 = ImVec4(0.90f, 0.90f, 0.90f, 1.00f); // #E6E6E6
    colors[ImGuiCol_TextDisabled]         = ImVec4(0.50f, 0.50f, 0.50f, 1.00f); // #808080

    // --- 3. Interactive States (Buttons, Selectables, Hovers) ---
    // The classic Windows Accent Blue (#0078D4 or updated Win11 #005A9E variant)
    ImVec4 winAccentBlue = ImVec4(0.00f, 0.45f, 0.83f, 1.00f); 
    
    // Normal button / background elements
    colors[ImGuiCol_FrameBg]              = ImVec4(0.16f, 0.16f, 0.16f, 1.00f); // #292929
    colors[ImGuiCol_FrameBgHovered]       = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBgActive]        = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);

    // Folder selectables and Header rows (Light highlight on hover)
    colors[ImGuiCol_Header]               = ImVec4(0.18f, 0.18f, 0.18f, 1.00f); // #2E2E2E
    colors[ImGuiCol_HeaderHovered]        = ImVec4(0.22f, 0.22f, 0.22f, 1.00f); // #383838
    colors[ImGuiCol_HeaderActive]         = winAccentBlue;

    // Address Bar Buttons & standard clickables
    colors[ImGuiCol_Button]               = ImVec4(0.14f, 0.14f, 0.14f, 0.00f); // Invisible by default like Win11 toolbars
    colors[ImGuiCol_ButtonHovered]        = ImVec4(0.22f, 0.22f, 0.22f, 1.00f); // Light grey on hover
    colors[ImGuiCol_ButtonActive]         = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);

    // Scrollbars
    colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.10f, 0.10f, 0.10f, 0.00f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
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
    sd.OutputWindow = ctx.hwnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &ctx.swapChain, &ctx.d3dDevice, &featureLevel, &ctx.d3dContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &ctx.swapChain, &ctx.d3dDevice, &featureLevel, &ctx.d3dContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget(ctx);
    return true;
}

void CleanupDeviceD3D(AppContext& ctx){
    CleanupRenderTarget(ctx);
    if (ctx.swapChain) { ctx.swapChain->Release(); ctx.swapChain = nullptr; }
    if (ctx.d3dContext) { ctx.d3dContext->Release(); ctx.d3dContext = nullptr; }
    if (ctx.d3dDevice) { ctx.d3dDevice->Release(); ctx.d3dDevice = nullptr; }
}

void CreateRenderTarget(AppContext& ctx){
    ID3D11Texture2D* pBackBuffer;
    ctx.swapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    ctx.d3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &ctx.renderTargetView);
    pBackBuffer->Release(); 
}

void CleanupRenderTarget(AppContext& ctx){
    if (ctx.renderTargetView) { ctx.renderTargetView->Release(); ctx.renderTargetView = nullptr; }
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
        ctx->dpiScale = (f32) newDpi / 96.0f;

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

        
        f32 topBarHeight = 32.0f * ctx->dpiScale;
        LONG borderThickness = (LONG) ( 8.0 * ctx->dpiScale);
        
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
            f32 controlClusterWidth = (clsBtnWidth + maxBtnWidth + minBtnWidth) * ctx->dpiScale;
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
        ctx->resizeWidth = (UINT)LOWORD(lParam); // Queue resize
        ctx->resizeHeight = (UINT)HIWORD(lParam);
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