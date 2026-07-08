// main.cpp

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

        RenderMainInterface();

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


    void Render(AppContext& ctx){
        // Get NavBar width
        // 3 pixel padding from left wall, 198 pixels of nav bar, address bar starts immediately
        // 8 pixel padding between Address bar and search bar and 11 pixel padding from search bar to right wall
        // Address bar then takes 0.706 of the remaining space (Window Width - (3 + 198 + 8 + 11))
        // Search bar takes 0.294 of the remaining space
        f32 windowWidth = ImGui::GetWindowWidth();
        f32 remainingWidth = windowWidth - ((ToolBarLayout::LeftPadding + NavBar::Width + ToolBarLayout::AddressToSearchGap + ToolBarLayout::RightPadding) * ctx.dpiScale);
        f32 addressWidth = remainingWidth * ToolBarLayout::AddressRatio;
        
        f32 startX = BarStartX * ctx.dpiScale;
        f32 startY = (TopBar::Height + ((NavBar::Height - Height)/2)) * ctx.dpiScale; // center with nav bar
        ImGui::SetCursorPos(ImVec2(startX, startY));

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f * ctx.dpiScale);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.16f, 0.16f, 0.16f, 1.0f)); // FrameBg color

        if (!ImGui::BeginChild("AddressBar", ImVec2(addressWidth, Height * ctx.dpiScale), ImGuiChildFlags_None, TopBar::Flags)){
            ImGui::PopStyleColor();
            ImGui::PopStyleVar(2);
            ImGui::EndChild();
            return;
        }
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(2);

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));

        char buffer[MAX_PATH] = {0}; 
        for (u64 i = 0; i < ctx.currentBreadcrumbs.count; i++){
            BreadcrumbItem& crumb = ctx.currentBreadcrumbs.breadcrumbs[i];

            // Draw Folder button
            const char* safeName = crumb.displayName.data ? crumb.displayName.data : "Unknown";
            snprintf(buffer, sizeof(buffer), "%s##bcrumb_%lld", safeName, i);
            if (ImGui::Button(buffer)){
                Navigation::NavigateTo(ctx, crumb.pidl);
            }
            ImGui::SameLine(0.0f, 8.0f);

            if (i < ctx.currentBreadcrumbs.count - 1 || (i == ctx.currentBreadcrumbs.count - 1 && ctx.currentBreadcrumbs.hasSubFolders)){
                const char arrowSign = ImGui::IsPopupOpen(buffer) ? 'v' : '>'; // do i use real icons or nah
                snprintf(buffer, sizeof(buffer), "%c##bcrumb_arrow_%lld", arrowSign, i);   // use a new buffer or nah
                if (ImGui::Button(buffer)){
                    ImGui::OpenPopup(buffer);
                }

                ImVec2 btnRect = ImGui::GetItemRectMax();
                ImGui::SetNextWindowPos(ImVec2(btnRect.x, btnRect.y + 2.0f));

                if (ImGui::BeginPopup(buffer)){
                    // cache the directory contents so we dont fetch every frame
                    if (ctx.popupCachePidl != crumb.pidl){
                        Backend::FreeLightShellItemArray(ctx.popupCacheList);
                        ctx.popupCacheList = Backend::GetDirectoryContents(ctx.currentBreadcrumbs.breadcrumbs[i].pidl);
                        ctx.popupCachePidl = crumb.pidl;
                    }

                    for (u64 j = 0; j < ctx.popupCacheList.numEntries; j++){
                        if (ImGui::Selectable(ctx.popupCacheList.entries[j].name.data)){
                            Navigation::NavigateTo(ctx, ctx.popupCacheList.entries[j].pidl);
                            ImGui::CloseCurrentPopup();
                            break;
                        }
                    }
                    ImGui::EndPopup();
                }
                ImGui::SameLine(0.0f, 8.0f);
            }
        }
        ImGui::PopStyleVar();   // FramePadding
        ImGui::EndChild();
    } 