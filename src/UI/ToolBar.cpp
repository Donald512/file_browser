#include "UI.h"

namespace Colors = UI::Colors;



void ToolBar::Render(AppContext& ctx){

    f32 height = Height * ctx.dpiScale;
    f32 leftPadding =  LeftPadding * ctx.dpiScale;
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Colors::WindowBackground);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(leftPadding, 0.0f));
        
    if (!ImGui::BeginChild("ToolBar", ImVec2(0, height), ImGuiChildFlags_None, Flags)){
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
        ImGui::EndChild();
        return;
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    NavBar::Render(ctx);
    ImGui::SameLine();
    
    AddressBar::Render(ctx);

    ImGui::EndChild();
    } 