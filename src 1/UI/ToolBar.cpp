#include "UI.h"

namespace Colors = UI::Colors;



void ToolBar::Render(AppContext& ctx){

    f32 height = Height * ctx.ui.dpiScale;
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Colors::WindowBackground);
        
    if (!ImGui::BeginChild("ToolBar", ImVec2(0, height), ImGuiChildFlags_None, Flags)){
        ImGui::PopStyleColor();
        ImGui::EndChild();
        return;
    }
    ImGui::PopStyleColor();

    NavBar::Render(ctx);
    ImGui::SameLine();
    
    AddressBar::Render(ctx);

    ImGui::EndChild();
    } 