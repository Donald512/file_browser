#include "file_browser.h"
#include "imgui_boilerplate.h"
#include "string_helper.h"
#include "history_helper.h"
#include "file_backend.h"
#include "renderer.h"


String g_currentDir;
DirectoryList g_currentDirList;
PathHistory g_pathHistory;

int main(void){
    // 1. Setup phase (Runs ONCE)
    HWND window = CreateMyOSWindow(); 
    if (!window) return 1;

    if (!InitializeGraphicsAPI(window)) return 1;
    
    ::ShowWindow(window, SW_SHOWMAXIMIZED);
    ::UpdateWindow(window);

    InitializeImGui(window);
  
    g_currentDir = CreateString("C:");
    g_currentDirList = GetDirectoryContents(g_currentDir);
    g_pathHistory = InitHistory();
    NewBranch(g_currentDir);

    bool running = true;
    while (running) {
        
        // A. Handle Windows events (clicks, closes, moves)
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) running = false;
        }

        g_SwapChainOccluded = false;

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        ImGui_Backend_NewFrame();
        ImGui::NewFrame();

        RenderMainInterface(&g_currentDirList);

        ImGui::Render();
        MyGraphicsAPI_PresentFrame(); 
    }

    // 3. Cleanup phase (Runs ONCE when exiting)
    DestroyDirectoryList(&g_currentDirList);
    ShutdownImGui(window);
    printf("Exited succefully\n");
    return 0;
}


/*
    Error in the beginning, with g_pathHistory 
*/