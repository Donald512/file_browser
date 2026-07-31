#pragma once

#include <d3d11.h>
#include <imgui.h>
#include <ShlObj.h>
#include <string>
#include <vector>
#include <wrl/client.h>

#include "Types.h"
#include "Shell.h"
#include "Navigation.h"
#include "icons.h"

#include "Threading.h"
#include <mutex>
#include <queue>


using Microsoft::WRL::ComPtr;

struct GraphicsContext{
    HWND hwnd = nullptr;
    ComPtr<ID3D11Device> d3dDevice;
    ComPtr<ID3D11DeviceContext> d3dContext;
    ComPtr<IDXGISwapChain> swapChain;
    ComPtr<ID3D11RenderTargetView> renderTargetView;

    bool swapChainOccluded = false;
    UINT resizeWidth = 0;
    UINT resizeHeight = 0;
};

struct UiState{
    f32 dpiScale = 1.0f;
    ImFont* mainFont = nullptr;   // non-owning — ImGui's font atlas owns these
    ImFont* smallFont = nullptr;   // non-owning — ImGui's font atlas owns these
    ImFont* iconFont = nullptr;
    ImVec4 clearColor = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
};

struct TaskSystem{

    // Threading
    Threading::ThreadPool threadPool{ std::thread::hardware_concurrency() };

    std::mutex mainThreadMutex;
    std::queue<Threading::MoveOnlyTask> mainThreadJobs;

    void DispatchToMain(Threading::MoveOnlyTask job) {
        std::lock_guard<std::mutex> lock(mainThreadMutex);  // just locks the list of jobs so only one thing is using it at once
        mainThreadJobs.push(std::move(job));    // push the new job to the list 
    }

    void RunMainThreadJobs() {
        std::queue<Threading::MoveOnlyTask > jobs; // temporary list to hold all the jobs
        {
            std::lock_guard<std::mutex> lock(mainThreadMutex);  // lock job
            std::swap(jobs, mainThreadJobs); // Fast swap,. moves all the jobs from main thread to temporary less
        }   //  unlocks Lock
        while (!jobs.empty()) { // do the jobs one by one
            jobs.front()(); // run the top job
            jobs.pop(); // discard when done
        }
    }

    template<typename BackgroundFunc>
    void RunAsync(BackgroundFunc&& bgFunc) {
        threadPool.Enqueue(std::forward<BackgroundFunc>(bgFunc));
    }
};


struct AppContext{
    GraphicsContext gfx;
    UiState ui;

    // Navigation & icon caching
    Navigation::NavigationController navigation;
    Icons::IconManager icons;

    // Special, frequently-referenced pidls
    WShell::Pidl pidlThisPC;
    WShell::Pidl pidlHome;
    WShell::Pidl pidlDesktop;
    WShell::Pidl pidlQuickAccess;
    WShell::Pidl pidlNetwork;

    std::vector<WShell::NewMenuItem> newMenuItems = WShell::EnumerateNewMenu();
    const std::vector<WShell::ItemLite>& items1 = WShell::GetSidebarItems(1);
    const std::vector<WShell::ItemLite>& items2 = WShell::GetSidebarItems(2);
    const std::vector<WShell::ItemLite>& items3 = WShell::GetSidebarItems(3);

    TaskSystem tasks;
};
