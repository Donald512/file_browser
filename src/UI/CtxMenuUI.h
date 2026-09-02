#pragma once

#include "BasicTypes.h"

#include "WinFramework.h"
#include <ShlObj.h>
#include <ShlObj_core.h>
#include <vector>
#include "CtxMenu.h"
#include "App.h"



void PushMenuTheme(f32);
void PopMenuTheme();


void RenderContextMenuStructure(App& app, ComPtr<IContextMenu> ctxMenu, std::vector<ContextMenuItem>& items, PCIDLIST_ABSOLUTE parentPidl, std::vector<PCITEMID_CHILD>& childPidls, HWND hwnd, f32 dpi);


