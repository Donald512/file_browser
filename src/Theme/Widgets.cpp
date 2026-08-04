#pragma once
#include "Widgets.h"
#include "imgui.h"
#include <string>
#include "ImGuiTheme.h"
#include "imgui_internal.h"

bool Widgets::Button(const char* label, const ImVec2& size_arg) {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

    // Calculate dimensions
    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = ImGui::CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);
    const ImRect rect(pos, {(pos.x + size.x), (pos.y + size.y)});
    
    ImGui::ItemSize(size, style.FramePadding.y);
    if (!ImGui::ItemAdd(rect, id)) return false;

    // Handle clicks and hover states natively
    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(rect, id, &hovered, &held, 0);

    // Fetch colors from our new Modern Theme
    ImU32 bg_col = ImGui::GetColorU32(held ? Theme::Current.palette.SurfaceActive : 
                                      hovered ? Theme::Current.palette.SurfaceHover : 
                                      Theme::Current.palette.Surface);
    ImU32 border_col = ImGui::GetColorU32(Theme::Current.palette.Border);
    ImU32 text_col = ImGui::GetColorU32(hovered ? Theme::Current.palette.Text : Theme::Current.palette.TextDisabled);

    // Draw Background (Filled) and Border (Stroke)
    window->DrawList->AddRectFilled(rect.Min, rect.Max, bg_col, Theme::Current.metrics.FrameRounding);
    window->DrawList->AddRect(rect.Min, rect.Max, border_col, Theme::Current.metrics.FrameRounding);

    // Draw Text centered
    ImVec2 text_pos = ImVec2(
        rect.Min.x + (size.x - label_size.x) * 0.5f,
        rect.Min.y + (size.y - label_size.y) * 0.5f
    );
    window->DrawList->AddText(text_pos, text_col, label);

    return pressed;
}

bool Widgets::BeginMenu(const char* label, bool enabled, ImTextureID icon, float dpi) {
    float iconSize = Theme::Current.metrics.IconSize * dpi;
    float iconSpacing = Theme::Current.metrics.IconSpacing * dpi;

    std::string hiddenLabel = std::string(label) + "##bm";

    // Reserve room on the left for the icon if one is provided
    if (icon) {
        ImGui::Indent(iconSize + iconSpacing);
    }

    bool isOpen = ImGui::BeginMenu(hiddenLabel.c_str(), enabled);

    if (icon) {
        ImGui::Unindent(iconSize + iconSpacing);

        // Draw the icon in the reserved space on the left
        ImVec2 itemMin = ImGui::GetItemRectMin();
        ImVec2 itemMax = ImGui::GetItemRectMax();
        float rowH = itemMax.y - itemMin.y;
        float offY = (rowH - iconSize) * 0.5f;

        ImVec2 iconMin(itemMin.x - iconSize - iconSpacing, itemMin.y + offY);
        
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddImage(icon, iconMin, ImVec2(iconMin.x + iconSize, iconMin.y + iconSize));
    }

    return isOpen;
}

bool Widgets::MenuItem(const char* text, const char* shortcut, bool checked, bool enabled, ImTextureID icon, float dpi) {
    float iconSize = Theme::Current.metrics.IconSize * dpi;
    float iconSpacing = Theme::Current.metrics.IconSpacing * dpi;

    // Push sleek spacing specific to modern menus
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(Theme::Current.metrics.MenuItemPadX * dpi, Theme::Current.metrics.MenuItemPadY * dpi));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f * dpi, 6.0f * dpi));

    std::string label = std::string(text) + "##mi";
    ImGui::Indent(iconSize + iconSpacing);
    
    bool clicked = ImGui::MenuItem(label.c_str(), nullptr, checked, enabled);
    
    ImGui::Unindent(iconSize + iconSpacing);
    ImGui::PopStyleVar(2);

    ImVec2 itemMin = ImGui::GetItemRectMin();
    ImVec2 itemMax = ImGui::GetItemRectMax();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // The rest of your icon drawing logic remains identical...
    if (icon) {
        float rowH = itemMax.y - itemMin.y;
        float offY = (rowH - iconSize) * 0.5f;
        ImVec2 iconMin(ImGui::GetItemRectMin().x - iconSize - iconSpacing, itemMin.y + offY);
        dl->AddImage(icon, iconMin, ImVec2(iconMin.x + iconSize, iconMin.y + iconSize));
    }

    if (shortcut && *shortcut) {
        ImVec2 sz = ImGui::CalcTextSize(shortcut);
        ImVec2 pos(itemMax.x - sz.x - 10.0f * dpi, itemMin.y + (itemMax.y - itemMin.y - sz.y) * 0.5f);
        dl->AddText(pos, ImGui::GetColorU32(Theme::Current.palette.TextDisabled), shortcut);
    }

    return clicked;
}

bool Widgets::IconMenuItem(const char* label, ImTextureID icon, const ImVec2& iconSize, bool selected){
    ImGui::PushID((void*) label);
    float pad = 6.0f;
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 textSize = ImGui::CalcTextSize(label);
    float rowHeight = ImGui::GetTextLineHeightWithSpacing();

    // width = icon + pad + text, so the popup sizes correctly
    bool clicked = ImGui::Selectable(("##sel_" + std::string(label)).c_str(), selected,
                                      ImGuiSelectableFlags_SpanAvailWidth,
                                      ImVec2(iconSize.x + pad + textSize.x, rowHeight));

    ImGui::PopID();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    float iconY = pos.y + (rowHeight - iconSize.y) * 0.5f;
    dl->AddImage(icon, ImVec2(pos.x, iconY), ImVec2(pos.x + iconSize.x, iconY + iconSize.y));
    dl->AddText(ImVec2(pos.x + iconSize.x + pad, pos.y + 2.0f), ImGui::GetColorU32(ImGuiCol_Text), label);

    return clicked;
}