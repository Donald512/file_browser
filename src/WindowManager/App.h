#pragma once

#include <d3d11.h>
#include "imgui.h"
#include "BasicTypes.h"
#include <wrl/client.h>
#include <variant>

#include "IconManager.h"
#include "TextureManager.h"
#include "sidebarEnum.h"
#include "TypenameManager.h"
#include "Tab.h"
#include "TaskSystem.h"
#include "Watcher.h"
#include "AppCommands.h"

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
    f32 width = 0;
    f32 height = 0;
};

struct UIState{
    f32 dpi = 1.0f;
    ImVec4 clearColor = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
};


template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;


struct App{

    UIState ui{};
    GraphicsContext gfx;

    TextureManager textures;
    
    TaskSystem tasks;
    
    IconManager icons{tasks};
    
    TypenameStore typeStore;
    DirectoryManager directory{tasks, typeStore};
    
    DirectoryWatcher watcher{tasks, directory, [this](AppCommand cmd) { QueueCommand(std::move(cmd)); }};
    
    Window window{directory, watcher};

    SidebarManager sidebar;

    std::vector<AppCommand> commandQueue;
    std::unordered_set<u64> clipBoardCutItems{};


    void QueueCommand(AppCommand cmd){
        commandQueue.push_back(std::move(cmd));
    }
    void ProcessCommands();
};


inline void App::ProcessCommands() {
    for (auto& cmd : commandQueue) {
        std::visit(overloaded{
            [&](Cmd_NewTab& c)    { window.NewTab(c.targetPidl.get()); },
            [&](Cmd_CloseTab& c)  { window.CloseTab(c.tabIndex); },
            [&](Cmd_SwitchTab& c) { window.SetActiveTab(c.tabIndex); },
            [&](Cmd_GoTo& c)      { window.tabs[c.tabIndex].GoTo(c.targetPidl.get(), Actions::Normal); },
            [&](Cmd_Rename& c)    { (void)c; },
            [&](Cmd_Delete& c)    { (void)c; },
            [&](Cmd_Refresh& c)   { window.tabs[c.tabIndex].Refresh(); },
            [&](Cmd_GoBack& c)    { window.tabs[c.tabIndex].GoBack(); },
            [&](Cmd_GoForward& c) { window.tabs[c.tabIndex].GoForward(); },
            [&](Cmd_GoParent& c)  { window.tabs[c.tabIndex].GoParent(); },
            [&](Cmd_OpenFile& c)  { WShell::ExecuteFile(c.targetPidl.get()); },
            [&](Cmd_ReSort& c)    { window.tabs[c.tabIndex].ReSort(); },
            [&](Cmd_RefreshByHash& c){ 
                u64 hashToInvalidate = c.hash;
                for (auto& tab : window.tabs){
                    if (tab.dir.parent.hash == hashToInvalidate){ tab.Refresh(); }
                }
            },
        }, cmd);
    }
    commandQueue.clear();
}