// main.cpp

#include "core.h"
#include "imgui_boilerplate.h"


int main(void){
    AppContext ctx{};
    // 1. Setup phase
    Utils::InitCOM();
    History::Init(ctx);
    // Get PIDL for "This PC"
    PIDLIST_ABSOLUTE startPidl;
    SHGetKnownFolderIDList(FOLDERID_ComputerFolder, 0, NULL, &startPidl);

    if (Navigation::NavigateTo(ctx, startPidl)){
        History::Append(ctx, ctx.currentFolderPidl);    // currentItem.pidl is freed in NavigateTo
    }
    
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"File Browser Window", nullptr };

    if (!CreateMyOSWindow(ctx, wc)) return 1;

    if (!InitializeGraphicsAPI(ctx, wc)) return 1;
    
    ::ShowWindow(ctx.hwnd, SW_SHOWMAXIMIZED);
    ::UpdateWindow(ctx.hwnd); // irrelevant

    InitializeImGui(ctx);
  
    // todo completely migrate from ImGui::Text to Direct2D + DirectWrite



    bool running = true;
    while (running) {
        
        // A. Handle Windows events (clicks, closes, moves)
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) running = false;
        }

        ctx.swapChainOccluded = false;

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (ctx.resizeWidth != 0 && ctx.resizeHeight != 0){
            CleanupRenderTarget(ctx);
            ctx.swapChain->ResizeBuffers(0, ctx.resizeWidth, ctx.resizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            ctx.resizeWidth = ctx.resizeHeight = 0;
            CreateRenderTarget(ctx);
        }

        ImGui_Backend_NewFrame();
        ImGui::NewFrame();

        UI::Render(ctx);

        ImGui::Render();
  
        MyGraphicsAPI_PresentFrame(ctx); 
    }

    // 3. Cleanup phase (Runs ONCE when exiting)
    History::Destroy(ctx);
    Backend::FreeDirectoryArray(ctx.currentDirArray);
    Backend::FreeBreadcrumbs(ctx.currentBreadcrumbs);
    Utils::FreePidl(ctx.popupCachePidl);
    Backend::FreeLightShellItemArray(ctx.popupCacheList);

    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    ShutdownImGui(ctx, wc);
    printf("Exited succefully\n");
    return 0;
}

/*  ImGuiListClipper for only previewing visible items
    Main thread should only give 60+ frames to ImGui, never touch a pdf file, open a video, or run an expensive SHGetFileInfoW
    UI thread pushes a job request to the crew, while other background threads handle preview

    - Fast search ideas
    Radix Tree / Prefix Tree (Trie)
    By default, explicitly exclude directories like AppData\Local\Temp, system caches, and OS binaries from the deep search index.
    LMDB or a flat SQLite database, and map it into memory (mmap).

*/



