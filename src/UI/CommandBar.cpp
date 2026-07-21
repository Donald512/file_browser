
#include "UI.h"

namespace Colors = UI::Colors;
namespace Style = UI::Style;

namespace Colors = UI::Colors;
namespace Style = UI::Style;

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

    ImGui::SetWindowFontScale(0.85f);

    f32 buttonHeight = ImGui::GetFrameHeight();
    f32 centerY = (Height * ctx.dpiScale - buttonHeight) * 0.5f;
    ImGui::SetCursorPosY(centerY);  

    // =========================== New 
    char* newMenuPopupId = "NewMenuPopup";       
    ImGui::BeginDisabled(ctx.navigation.Contents().Access() ==WShell::FolderAccess::NoCreate);
    const char* newLabel = "New " ICON_REG_CHEVRON_DOWN;

    if (UI::Helpers::IconAndTextButton("NewBtn", ICON_REG_ADD_CIRCLE, newLabel)){
        ImGui::OpenPopup(newMenuPopupId);
    }
    ImGui::EndDisabled();
    if (ImGui::BeginPopup(newMenuPopupId)){
        ImGui::EndPopup();
    }
    // ============================
    ImGui::SameLine();


    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleVar(2);
    ImGui::EndChild();
}
