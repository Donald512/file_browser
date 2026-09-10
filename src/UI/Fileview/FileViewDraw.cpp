#include "FileViewDraw.h"
#include "TabStates.h"
#include "ImGuiHelpers.h"
#include "Shell.h"
#include "FileViewInput.h"
#include "Item.h"
#include "App.h"
#include "Tab.h"
#include "FileViewHelpers.h"


void RenderRenameWidget(const char* strId, ImVec2 pos, ImVec2 baseSize, ImVec2 maxSize, GrowAxis axis, HWND hwnd, RenameState& renameState, PCIDLIST_ABSOLUTE parentPidl, PCIDLIST_ABSOLUTE childPidl, const char* childName, ImU32 bgCol){
    AutoInputColors cols;
    cols.bg = ImGui::ColorConvertU32ToFloat4(bgCol);
    cols.border = {};
    cols.selectionBg = {};
    cols.text = ImGui::ColorConvertU32ToFloat4(Theme::Current.palette.Text);

    bool justOpened = (renameState.renameFocusHandledFor != renameState.renamingItemId.value());
    if (justOpened) renameState.renameFocusHandledFor = renameState.renamingItemId.value();

    InputResult res = RenderAutoResizingInputText(strId, pos, baseSize, maxSize, renameState.renameBuffer, sizeof(renameState.renameBuffer), axis, false, &cols, justOpened);
    
    if (res == InputResult::Committed){
        WShell::CommitRename(hwnd, parentPidl, {childPidl, childName}, renameState.renameBuffer);
        renameState.renamingItemId = std::nullopt;
        renameState.renameFocusHandledFor = std::nullopt;
    }
    else if (res == InputResult::Cancelled){
        renameState.renamingItemId = std::nullopt;
        renameState.renameFocusHandledFor = std::nullopt;
    }
}


ItemInteraction DrawItemChrome(ImDrawList* dl, ImGuiWindow* window, App& app, f32 dpi, const DirParent& parent, const ItemView& child, int visualIndex, const ImRect& fullRect, f32 rounding, bool drawFocusRing, bool drawBg){
    auto& activeTab = app.window.GetActiveTab();

    ImGuiID id = window->GetID((void*)(intptr_t)child.hash);
    ItemInteraction ia = HandleItemInteraction(app, parent, child, visualIndex, id, fullRect);

    bool isSelected = activeTab.isSelected(child.hash);
    bool isFocused  = activeTab.selState.focusHash == child.hash;

    if (drawBg) DrawSelectableBg(dl, fullRect, ia.hovered, isSelected, rounding);

    if (drawFocusRing && isFocused){
        // todo change it from white to palette.focused
        dl->AddRect(fullRect.Min, fullRect.Max, 0xFFFFFFFF, rounding, 1.0f * dpi);
    }
    return ia;
}


void DrawItemIcon(ImDrawList* dl, App& app, const DirParent& parent, const ItemView& child, ImVec2 pos, f32 iconSize, int shilSize){
    u32 iconIndex = app.icons.GetIconIndex(parent.pidl.get(), child.pidl, child.hash);
    
    auto iconFallback = [&](){
        const char* fallback = child.IsFolder() ? ICON_REG_FOLDER : ICON_REG_DOCUMENT;
        DrawTextCenteredSingleLine(dl, pos, ImVec2(pos.x + iconSize, pos.y + iconSize), fallback, Theme::Current.palette.TextMuted, iconSize);
    };

    if (iconIndex == UINT32_MAX){
        iconFallback();
        return;
    }

    ImTextureID iconTex = app.textures.GetTexture({iconIndex, shilSize});
    if (!iconTex){
        iconFallback();
        return;
    }

    bool isCut =  isFileCutOnClipBoard(app.clipBoardCutItems, child.hash);

    ImU32 tint = (child.IsHidden() || isCut) ? IM_COL32(255, 255, 255, 128) : IM_COL32(255, 255, 255, 255);

    dl->AddImage(iconTex, pos, ImVec2(pos.x + iconSize, pos.y + iconSize), ImVec2(0, 0), ImVec2(1, 1), tint);
}


void DrawItemText(HWND hwnd, ImDrawList* dl, Tab& activeTab, const DirParent& parent, const ItemView& child, const ImRect& textRect, TextRenderMode textRenderMode, int maxLines){
    auto& renameState = activeTab.renameState;
    if (renameState.renamingItemId == child.hash) {
        bool isSelected = activeTab.isSelected(child.hash);
        ImU32 bgCol = isSelected ? Theme::Current.palette.SurfaceActive : Theme::Current.palette.Surface;
 
        ImVec2 baseSize = textRect.GetSize();
        
        RenderRenameWidget("iconsRename", textRect.Min, baseSize, baseSize, GrowAxis::X, hwnd, renameState, parent.pidl.get(), child.pidl, child.name, bgCol);
        return; 
    }

    switch(textRenderMode){
        case TextRenderMode::WrappedCentered:       RenderTextWrappedCenteredEllipsis(dl, textRect, child.name, nullptr, maxLines);
        break;
        case TextRenderMode::SingleLineEllipsis:    DrawTextEllipsisSingleLine(dl, textRect, child.name, Theme::Current.palette.Text);
        break;
        default: assert(false);
    }
}