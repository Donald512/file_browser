#pragma once
#include "wrl/client.h"
#include <string>
#include <vector>
#include "imgui.h"
#include "BasicTypes.h"
#include <ShlObj.h>
#include <d3d11.h>


using Microsoft::WRL::ComPtr;

struct ContextMenuItem {
    std::string text;
    std::string shortcut;
    std::string verb;
    UINT id;            // The offset ID to invoke the command later
    bool isSeparator = false;
    bool enabled = true;
    bool checked = false;

    ComPtr<ID3D11ShaderResourceView> srv; 
    ImTextureID hIconTex{};      

    std::vector<ContextMenuItem> subItems;  // for nested menus

    std::string label;      // per-item cached padded label
    f32 labelDpi = -1.0f;
};


void ExecuteContextMenuCommand(ComPtr<IContextMenu> menu, PCIDLIST_ABSOLUTE parentPidl, std::vector<PCITEMID_CHILD>& childPidls, UINT idOffset, HWND ownerHwnd);


std::vector<ContextMenuItem> GetBackgroundContextMenu(ComPtr<IContextMenu>& outMenu, PCIDLIST_ABSOLUTE folderPidl, ID3D11Device* dev);

std::vector<ContextMenuItem> GetContextMenu(ComPtr<IContextMenu>& outActiveMenu, PCIDLIST_ABSOLUTE parentPidl, std::vector<PCITEMID_CHILD>& childPidls, HWND hwnd, ID3D11Device* pDevice);