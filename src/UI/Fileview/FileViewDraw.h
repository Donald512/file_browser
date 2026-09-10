#pragma once

#include "imgui.h"
#include <ShlObj.h>
#include "BasicTypes.h"

struct ItemInteraction;
enum class GrowAxis;
struct App;
class Tab;
struct ItemView;
struct DirParent;
struct RenameState;


enum class TextRenderMode {WrappedCentered, SingleLineEllipsis};

void RenderRenameWidget(const char* strId, ImVec2 pos, ImVec2 baseSize, ImVec2 maxSize, GrowAxis axis, HWND hwnd, RenameState& renameState, PCIDLIST_ABSOLUTE parentPidl, PCIDLIST_ABSOLUTE childPidl, const char* childName, ImU32 bgCol);

ItemInteraction DrawItemChrome(ImDrawList* dl, ImGuiWindow* window, App& app, f32 dpi, const DirParent& parent, const ItemView& child, int visualIndex, const ImRect& fullRect, f32 rounding, bool drawFocusRing = true, bool drawBg = true);

void DrawItemIcon(ImDrawList* dl, App& app, const DirParent& parent, const ItemView& child, ImVec2 pos, f32 iconSize, int shilSize);

void DrawItemText(HWND hwnd, ImDrawList* dl, Tab& activeTab, const DirParent& parent, const ItemView& child, const ImRect& textRect, TextRenderMode textRenderMode, int maxLines);