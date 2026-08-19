#pragma once

#include <d3d11.h>
#include "imgui.h"
#include "BasicTypes.h"
#include <wrl/client.h>

#include "IconManager.h"
#include "TextureManager.h"
#include "sidebarEnum.h"

#include "Tab.h"

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

enum class CmdType{ 
    NewTab, CloseTab, SwitchTab,
    GoTo, GoBack, GoForward, GoParent, Refresh,
    ReSort,
    OpenFile,
    CopySelection, CutSelection, Paste,
    DeleteSelection, RenameSelected, NewFolder
};

struct AppCommand{
    CmdType type;
    WShell::Pidl targetPidl;
    u64 pidlHash;
    size_t tabIndex = 0;
    std::wstring text;
};

struct App{

    UIState ui{};
    GraphicsContext gfx;

    Window window;
    TextureManager textures;
    DirectoryManager directory;
    IconManager icons;
    SidebarManager sidebar;

    std::vector<AppCommand> commandQueue;

    void QueueCommand(AppCommand cmd){
        commandQueue.push_back(std::move(cmd));
    }
    void ProcessCommands();
};



void App::ProcessCommands(){
    for (auto& cmd : commandQueue) {
        switch (cmd.type) {
            case CmdType::NewTab:
                window.NewTab(cmd.targetPidl.get());
                break;
            case CmdType::CloseTab:
                window.CloseTab(cmd.tabIndex);
                break;
            case CmdType::SwitchTab:
                window.SetActiveTab(cmd.tabIndex);
                break;
            case CmdType::GoTo:
                window.GetActiveTab().GoTo(cmd.targetPidl.get(), Actions::Normal);
                break;
            case CmdType::Refresh:
                window.GetActiveTab().Refresh();
                break;
            case CmdType::GoBack:
                window.GetActiveTab().GoBack();
                break;
            case CmdType::GoForward:
                window.GetActiveTab().GoForward();
                break;
            case CmdType::GoParent:
                window.GetActiveTab().GoParent();
                break;
            case CmdType::OpenFile:
                WShell::ExecuteFile(cmd.targetPidl.get());
                break;
            case CmdType::ReSort:
                window.tabs[cmd.tabIndex].ReSort();
                break;
            
        }
    }
    commandQueue.clear(); // Always clear after processing!

}