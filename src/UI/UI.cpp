#include "UI.h"

void UI::Render(AppContext& ctx){
    // Make the root ImGui window fill the entire Windows window.
    const ImGuiViewport* viewport = ImGui::GetMainViewport();   
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, Style::NoPadding);
    if (ImGui::Begin("Main UI Workspace", nullptr, WindowFlags)){
        ImGui::PopStyleVar();
        
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); // Transparent
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
    
        TopBar::Render(ctx);
        ToolBar::Render(ctx); // contains NavBar, Address Bar, and Search Bar
        CommandBar::Render(ctx);
        FileView::Render(ctx);
        
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
        
        ImGui::End();
    }
}

bool UI::Helpers::IconAndTextButton(const char* str_id, const char* icon, const char* label, const ImVec4& icon_color){
    ImGui::PushID(str_id);

    f32 iconWidth = ImGui::CalcTextSize(icon).x;
    f32 textWidth = ImGui::CalcTextSize(label).x;
    f32 innerSpacing = ImGui::GetStyle().ItemInnerSpacing.x;
    f32 btnWidth = iconWidth + textWidth + (innerSpacing * 2.0f);
    f32 btnHeight = ImGui::GetFrameHeight();

    ImVec2 startCursorPos = ImGui::GetCursorPos();

    // clickable bounding box
    bool pressed = ImGui::Button("##bg", ImVec2(btnWidth, btnHeight));

    ImGui::SetCursorPos(ImVec2(startCursorPos.x + innerSpacing, startCursorPos.y + (btnHeight - ImGui::GetTextLineHeight()) * 0.5f));

    ImGui::PushStyleColor(ImGuiCol_Text, icon_color);
    ImGui::TextUnformatted(icon);
    ImGui::PopStyleColor();

    ImGui::SameLine(0.0f, innerSpacing);
    ImGui::TextUnformatted(label);

    ImGui::PopID();
    return pressed;
}