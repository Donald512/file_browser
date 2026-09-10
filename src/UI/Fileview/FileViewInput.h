#pragma once

#include "imgui.h"
#include "imgui_internal.h"
#include <vector>
#include <ShlObj.h>

#include "BasicTypes.h"

struct App;
class Tab;
struct DirParent;
class DirChildren;
struct ItemView;
struct SelectionState;

struct ItemInteraction {
    bool hovered;
    bool clicked;
};

ItemInteraction HandleItemInteraction(App& app, const DirParent& parent, const ItemView& child, int visualIndex, ImGuiID id, const ImRect& rect);

void KeyboardNavigationInteraction(f32 dpi, App& app);

std::vector<PCITEMID_CHILD> GetSelectedItems(Tab& tab, const DirChildren& children);

void ClearFocusState(SelectionState& selState);
