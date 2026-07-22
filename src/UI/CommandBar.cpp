
#include "UI.h"

namespace Colors = UI::Colors;
namespace Style = UI::Style;

static void DrawNewMenuDropdown(AppContext& ctx);


void CommandBar::Render(AppContext& ctx){
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Colors::WindowForeground);
    if (!ImGui::BeginChild("CommandBar", ImVec2(0, Height * ctx.dpiScale), ImGuiChildFlags_None, TopBar::Flags) ){
        ImGui::PopStyleColor();
        ImGui::EndChild();
        return;
    }
    ImGui::PopStyleColor();

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f * ctx.dpiScale, 4.0f * ctx.dpiScale));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, Style::NoBorder);

    ImGui::SetWindowFontScale(1.2f);

    f32 buttonHeight = ImGui::GetFrameHeight();
    f32 centerY = (Height * ctx.dpiScale - buttonHeight) * 0.5f;
    ImGui::SetCursorPosY(centerY);  

    // =========================== New Button & Popup
    char* newMenuPopupId = "NewMenuPopup";       
    ImGui::BeginDisabled(ctx.navigation.Contents().Access() == WShell::FolderAccess::NoCreate);
    const char* newLabel = ICON_REG_ADD_CIRCLE " New " ICON_REG_CHEVRON_DOWN;

    if (ImGui::Button(newLabel)){
        ImGui::OpenPopup(newMenuPopupId);
    }
    ImGui::EndDisabled();

    if (ImGui::BeginPopup(newMenuPopupId)){
        DrawNewMenuDropdown(ctx);
        ImGui::EndPopup();
    }
    // ============================
    ImGui::SameLine(0.0f, 8.0f);
    
    ImGui::SetCursorPosY(centerY);  
    if (ImGui::Button(ICON_REG_CUT));
    ImGui::SameLine(0.0f, 8.0f);
    ImGui::SetCursorPosY(centerY);  
    if (ImGui::Button(ICON_REG_COPY));
    ImGui::SameLine(0.0f, 8.0f);
    ImGui::SetCursorPosY(centerY);  
    if (ImGui::Button(ICON_REG_CLIPBOARD_PASTE));
    ImGui::SameLine(0.0f, 8.0f);
    ImGui::SetCursorPosY(centerY);  
    if (ImGui::Button(ICON_REG_RENAME));
    ImGui::SameLine(0.0f, 8.0f);
    ImGui::SetCursorPosY(centerY);  
    if (ImGui::Button(ICON_REG_SHARE));
    ImGui::SameLine(0.0f, 8.0f);
    ImGui::SetCursorPosY(centerY);  
    if (ImGui::Button(ICON_REG_BIN_RECYCLE));
    ImGui::SameLine(0.0f, 8.0f);
    ImGui::SetCursorPosY(centerY);  


    // =========================== SORT & POPUP
    char* sortMenuPopupId = "SortMenuPopup";       
    const char* sortLabel = "Sort " ICON_REG_CHEVRON_DOWN;

    if (ImGui::Button( ICON_REG_ARROW_SORT " Sort " ICON_REG_CHEVRON_DOWN)){
        ImGui::OpenPopup(sortMenuPopupId);
    }

    if (ImGui::BeginPopup(sortMenuPopupId)){
        ImGui::EndPopup();
    }
    // ============================
    ImGui::SameLine(0.0f, 8.0f);

    ImGui::SetCursorPosY(centerY);  
    // =========================== View
    char* viewMenuPopupId = "ViewMenuPopup";       
    if (ImGui::Button( ICON_REG_LIST " View " ICON_REG_CHEVRON_DOWN)){
        ImGui::OpenPopup(viewMenuPopupId);
    }
    if (ImGui::BeginPopup(viewMenuPopupId)){
        ImGui::EndPopup();
    }
    // ============================



    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleVar(2);
    ImGui::EndChild();
}

static void DrawNewMenuDropdown(AppContext& ctx){
    for (auto& item : ctx.newMenuItems){
        ImGui::PushID(&item);
        ImTextureID iconTex = ctx.icons.GetTexture(item.iconIndex);
        if (iconTex){
            ImGui::Image(iconTex, ImVec2(16.0f, 16.0f));
            ImGui::SameLine();
        }
        if (ImGui::Selectable(item.displayName.c_str())){
            
            ImGui::CloseCurrentPopup();
            ImGui::PopID();
            break;
        }
        ImGui::PopID();

    }
}