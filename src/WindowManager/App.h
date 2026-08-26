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



struct Cmd_NewTab      { WShell::Pidl targetPidl; };
struct Cmd_CloseTab    { size_t tabIndex; };
struct Cmd_SwitchTab   { size_t tabIndex; };
struct Cmd_GoTo        { size_t tabIndex; WShell::Pidl targetPidl; };
struct Cmd_Rename      { std::wstring newName; };
struct Cmd_Delete      { std::vector<PCITEMID_CHILD> items; bool permanent = false; };
struct Cmd_Refresh     { size_t tabIndex;};
struct Cmd_GoBack      { size_t tabIndex;};
struct Cmd_GoForward   { size_t tabIndex;};
struct Cmd_GoParent    { size_t tabIndex;};
struct Cmd_OpenFile    { WShell::Pidl targetPidl; };
struct Cmd_ReSort      { size_t tabIndex; };

using AppCommand = std::variant<
    Cmd_NewTab, Cmd_CloseTab, Cmd_SwitchTab, Cmd_GoTo,
    Cmd_Rename, Cmd_Delete, Cmd_Refresh, Cmd_GoBack,
    Cmd_GoForward, Cmd_GoParent, Cmd_OpenFile, Cmd_ReSort
>;

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;


struct App{

    UIState ui{};
    GraphicsContext gfx;

    Window window;
    TextureManager textures;
    
    TaskSystem tasks;

    DirectoryManager directory;
    IconManager icons{tasks};
    SidebarManager sidebar;
    TypenameStore typeStore;

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
            [&](Cmd_ReSort& c)    { window.tabs[c.tabIndex].ReSort(typeStore); },
        }, cmd);
    }
    commandQueue.clear();
}