#pragma once

#include "imgui.h"

namespace Widgets {
    bool Button(const char* label, const ImVec2& size = ImVec2(0, 0));
    bool MenuItem(const char* text, const char* shortcut, bool checked, bool enabled, ImTextureID icon, float dpi);
    bool BeginMenu(const char* text, bool enabled, ImTextureID icon, float dpi);
    bool IconMenuItem(const char* label, ImTextureID icon, const ImVec2& iconSize, bool selected = false);
}