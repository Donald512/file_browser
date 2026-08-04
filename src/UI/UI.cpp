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

        ImGui::PushFont(ctx.ui.smallFont);
        // ================================
        f32 sidebarWidth = Sidebar::Width * ctx.ui.dpiScale;
        Sidebar::Render(ctx, sidebarWidth);
        ImGui::SameLine(0, 0);

        f32 splitterWidth = 2.0f * ctx.ui.dpiScale;
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0)); // Make it invisible
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0.47f, 0.83f, 1.0f)); // Blue when clicked
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.2f, 1.0f)); // Grey when hovered

        // Draw the button filling the height of the window
        ImGui::Button("##vsplitter", ImVec2(splitterWidth, ImGui::GetContentRegionAvail().y));

        // Change cursor to resize arrow when hovering the invisible button
        if (ImGui::IsItemHovered()) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        }

        // Logic to resize
        if (ImGui::IsItemActive()) {
            // Add mouse delta to sidebar width
            Sidebar::Width += ImGui::GetIO().MouseDelta.x;
            
            // Clamp the width so they can't shrink it to 0 or make it take the whole screen
            if (Sidebar::Width < 150.0f * ctx.ui.dpiScale) Sidebar::Width = 150.0f * ctx.ui.dpiScale;
            f32 maxWidth = ImGui::GetWindowWidth()/2 - (100.0f * ctx.ui.dpiScale);
            if (Sidebar::Width > maxWidth) Sidebar::Width = maxWidth;
        }
        ImGui::PopStyleColor(3);
        ImGui::SameLine(0, 0);

        // ================================

        FileView::Render(ctx);
        ImGui::PopFont();
        
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
        
        ImGui::End();
    }
}
