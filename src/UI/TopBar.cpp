#include "UI.h"

namespace Colors = UI::Colors;
namespace Style = UI::Style;

void TopBar::Render(AppContext& ctx){
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Colors::TopBarBackground);
    f32 addedHeight = !::IsZoomed(ctx.hwnd) ? PaddingForIfWindowRestored : 0.0f;
    if (!ImGui::BeginChild("TopBar", ImVec2(0, (Height + addedHeight) * ctx.dpiScale), ImGuiChildFlags_None, Flags)){
        ImGui::PopStyleColor();
        ImGui::EndChild();
        return;
    }
    ImGui::PopStyleColor();

    f32 windowWidth = ImGui::GetWindowWidth();
    // f32 captionBtnsWidth = TotalBtnsWidth * ctx.dpiScale;
    // f32 captionBtnsStartX = windowWidth - captionBtnsWidth;

    f32 btnWidth = BtnWidth * ctx.dpiScale;
    f32 btnHeight = BtnHeight * ctx.dpiScale;
    f32 closeBtnWidth = CloseBtnWidth * ctx.dpiScale;

    f32 closeStartX = windowWidth - closeBtnWidth;
    f32 maximizeStartX = closeStartX - btnWidth;
    f32 minimizeStartX = maximizeStartX - btnWidth;

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, Style::NoRounding); // sharp squares
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, Style::NoBorder);   // Hard-remove any outline borders
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, Style::NoPadding); // Forces razor-sharp centered glyph alignment 

    ImGui::SetCursorPos(ImVec2(minimizeStartX, 0.0f));   
    ImGui::Button(ICON_REG_SUBTRACT "##win_min", ImVec2(btnWidth, btnHeight));

    // NOTE: Windows handles the actual maximize behavior so Snap Layouts continue to work.
    // These buttons are purely visual and forward the interaction to the native title bar.
    ImGui::SetCursorPos(ImVec2(maximizeStartX, 0.0f)); 
    const char* maximizeGlyph = ::IsZoomed(ctx.hwnd) ? ICON_REG_SQUARE_MULTIPLE "##win_max" : ICON_REG_MAXIMIZE "##win_max";
    ImGui::Button(maximizeGlyph, ImVec2(btnWidth, btnHeight));

    // to give hover-red highlight
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.90f, 0.11f, 0.14f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.70f, 0.09f, 0.11f, 1.0f)); // Darker red on click

    ImGui::SetCursorPos(ImVec2(closeStartX, 0.0f)); 
    ImGui::Button(ICON_REG_DISMISS "##win_close", ImVec2(btnWidth, btnHeight));
    
    ImGui::PopStyleColor(2);    // for close button 
    ImGui::PopStyleVar(3);  // rounding, bordersize, padding

    ImGui::EndChild();
}