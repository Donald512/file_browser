
#include "UI.h"

namespace Style = UI::Style;

namespace NavBar{
    void Render(AppContext& ctx){
        if (!ImGui::BeginChild("NavBar", ImVec2(Width * ctx.ui.dpiScale, Height * ctx.ui.dpiScale), ImGuiChildFlags_None, TopBar::Flags)){
            ImGui::EndChild();
            return;
        }

        
        // ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, navBtnSize * 0.5f); // 50% rounding
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,  4.0f * ctx.ui.dpiScale); 
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, Style::NoBorder);   // remove any outline borders
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,Style::NoPadding);   // Autocenters text inside button, but we need to center button ourselves

        f32 centerY = (ToolBar::Height - BtnSize) * 0.5f * ctx.ui.dpiScale;
        ImGui::SetCursorPosY(centerY);

        f32 btnSize = BtnSize * ctx.ui.dpiScale;
        f32 margin = XPadding * ctx.ui.dpiScale;

        ImGui::BeginDisabled(!ctx.navigation.CanGoBack());
        if (ImGui::Button(ICON_REG_ARROW_LEFT "##nav_backward", ImVec2(btnSize, btnSize))) { 
            ctx.navigation.GoBack();
        }   ImGui::SameLine(0, margin);
        ImGui::EndDisabled();

        ImGui::BeginDisabled(!ctx.navigation.CanGoForward());
        if (ImGui::Button(ICON_REG_ARROW_RIGHT "##nav_forward", ImVec2(btnSize, btnSize))) {
            ctx.navigation.GoForward();
        }   ImGui::SameLine(0, margin);
        ImGui::EndDisabled();

        ImGui::BeginDisabled(!ctx.navigation.CanGoParent());
        if (ImGui::Button(ICON_REG_ARROW_UP "##nav_parent", ImVec2(btnSize, btnSize))) {
            ctx.navigation.GoParent();
        }   ImGui::SameLine(0, margin);
        ImGui::EndDisabled();


        if (ImGui::Button(ICON_REG_ARROW_CLOCKWISE "##refresh", ImVec2(btnSize, btnSize))) { 
            ctx.navigation.Refresh();
        };

        ImGui::PopStyleVar(3);
        ImGui::EndChild();
    }

} // namespace NavBar