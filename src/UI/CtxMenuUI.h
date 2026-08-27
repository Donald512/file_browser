#pragma once

#include "BasicTypes.h"

#include "WinFramework.h"
#include <ShlObj.h>
#include <ShlObj_core.h>
#include <vector>
#include "CtxMenu.h"


void PushMenuTheme(f32);
void PopMenuTheme();




void RenderContextMenuStructure(ComPtr<IContextMenu>, std::vector<ContextMenuItem>&, PCIDLIST_ABSOLUTE, std::vector<PCITEMID_CHILD>&, HWND, f32);

