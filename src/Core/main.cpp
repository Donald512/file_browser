#include "AppContext.h"
#include "imgui_boilerplate.h"
#include "UI.h"
#include <ShlDisp.h>
#include "ShellAsync.h"


// {f874310e-b6b7-47dc-bc84-b9e6b38f5903}
constexpr CLSID CLSID_HOME = 
    { 0xf874310e, 0xb6b7, 0x47dc, { 0xbc, 0x84, 0xb9, 0xe6, 0xb3, 0x8f, 0x59, 0x03 } };

int main (void){
    // Init com
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    AppContext ctx{};

    // Must happen before the first NavigateTo (and before anything enqueues background work)
    ctx.navigation.BindTaskSystem(ctx.tasks);
    // Get PIDL for "This PC"
    SHGetKnownFolderIDList(FOLDERID_ComputerFolder, 0, NULL, ctx.pidlThisPC.GetAddressOf());
    SHGetKnownFolderIDList(FOLDERID_Desktop, 0, NULL, ctx.pidlDesktop.GetAddressOf()); 
    SHGetKnownFolderIDList(FOLDERID_NetworkFolder, 0, NULL, ctx.pidlNetwork.GetAddressOf()); 
    SHParseDisplayName(L"shell:::{f874310e-b6b7-47dc-bc84-b9e6b38f5903}", NULL, ctx.pidlHome.GetAddressOf(), 0, NULL);
    SHParseDisplayName(L"shell:::{679F85CB-0220-4080-B29B-5540CC05AAB6}", NULL, ctx.pidlQuickAccess.GetAddressOf(), 0, NULL);


    WShell::Async::RequestSidebarItems(ctx);

    ctx.navigation.NavigateTo(ctx.pidlThisPC.get());

    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"File Browser Window", nullptr };
    if (!CreateMyOSWindow(ctx, wc)) return 1;

    if (!InitializeGraphicsAPI(ctx, wc)) return 1;
    ctx.icons.Init(ctx.gfx.d3dDevice.Get(), ctx.gfx.d3dContext.Get());

    ::ShowWindow(ctx.gfx.hwnd, SW_SHOWMAXIMIZED);
    ::UpdateWindow(ctx.gfx.hwnd); // irrelevant

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

        ctx.gfx.swapChainOccluded = false;

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (ctx.gfx.resizeWidth != 0 && ctx.gfx.resizeHeight != 0){
            CleanupRenderTarget(ctx);
            ctx.gfx.swapChain->ResizeBuffers(0, ctx.gfx.resizeWidth, ctx.gfx.resizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            ctx.gfx.resizeWidth = ctx.gfx.resizeHeight = 0;
            CreateRenderTarget(ctx);
        }

        ctx.tasks.RunMainThreadJobs();
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

// ! Check if Virtual is worth eliminating in threading 
// ! \\192.168.1.5\SharedFolder try this in addressBar


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

