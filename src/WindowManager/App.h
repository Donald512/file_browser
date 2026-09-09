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

    std::vector<ShellNewEntry> newEntries;


    void QueueCommand(AppCommand cmd){
        commandQueue.push_back(std::move(cmd));
    }
    void ProcessCommands();
};
