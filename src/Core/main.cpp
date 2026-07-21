#include "AppContext.h"
#include "imgui_boilerplate.h"
#include "UI.h"
#include <ShlDisp.h>


// {f874310e-b6b7-47dc-bc84-b9e6b38f5903}
constexpr CLSID CLSID_HOME = 
    { 0xf874310e, 0xb6b7, 0x47dc, { 0xbc, 0x84, 0xb9, 0xe6, 0xb3, 0x8f, 0x59, 0x03 } };

int main (void){
    // Init com
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    AppContext ctx{};

    // Get PIDL for "This PC"
    SHGetKnownFolderIDList(FOLDERID_ComputerFolder, 0, NULL, ctx.pidlThisPC.GetAddressOf());
    SHGetKnownFolderIDList(FOLDERID_Desktop, 0, NULL, ctx.pidlDesktop.GetAddressOf());
    SHParseDisplayName(L"shell:::{f874310e-b6b7-47dc-bc84-b9e6b38f5903}", NULL, ctx.pidlHome.GetAddressOf(), 0, NULL);

    ctx.navigation.NavigateTo(ctx.pidlThisPC.get());

    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"File Browser Window", nullptr };
    if (!CreateMyOSWindow(ctx, wc)) return 1;

    if (!InitializeGraphicsAPI(ctx, wc)) return 1;
    ctx.icons.Init(ctx.d3dDevice.Get(), ctx.d3dContext.Get());

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

        ctx.icons.NextFrame(); 
        ImGui_Backend_NewFrame();
        ImGui::NewFrame();

        UI::Render(ctx); 

        ImGui::Render();
  
        MyGraphicsAPI_PresentFrame(ctx); 
    }

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

    Decouple "data is ready" from "frame draws" — render whatever's currently known immediately, backfill icons/thumbnails asynchronously as they arrive instead of waiting.

    Shell extensions:
        Extension
        Average response
        Failure rate
        Timeouts
    If one extension constantly takes 2.8 seconds, mark it:
        Slow
        Disable?
        Always load
        Never load
        Ask me
        
    Ability to completely ignore extensions if a user is savvy enough

    Give every task a budget
        Icon: 100 ms budget.
        Thumbnail: 500 ms budget. 
        Metadata: 300 ms budget.
    If it misses: Stop. Use placeholder. Put at the end of queue.

    
    Performance Mode
        Maximum compatibility (default)
        Balanced
        Maximum speed
    Then an Advanced page for people who care:
        Ignore network metadata
        Disable slow shell extensions
        Thumbnail generation limit
        Thumbnail cache size
        Network timeout
        Maximum concurrent workers
    So that Grandma never sees it.
*/

