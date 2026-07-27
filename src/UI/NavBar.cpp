
#include "UI.h"

namespace Style = UI::Style;
namespace Helpers = UI::Helpers;

namespace NavBar{
    void Render(AppContext& ctx){
        if (!ImGui::BeginChild("NavBar", ImVec2(Width * ctx.ui.dpiScale, Height * ctx.ui.dpiScale), ImGuiChildFlags_None, TopBar::Flags)){
            ImGui::EndChild();
            return;
        }
        
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, Style::NoBorder);   // remove any outline borders
        
        f32 btnSize = BtnSize * ctx.ui.dpiScale;
        f32 margin = XPadding * ctx.ui.dpiScale;
        UI::Helpers::AlignCursorVertically(ToolBar::Height * ctx.ui.dpiScale, btnSize);
        
        if (Helpers::IconButton(ICON_REG_ARROW_LEFT, btnSize, !ctx.navigation.CanGoBack())){
            ctx.navigation.GoBack();
        }
        ImGui::SameLine(0, margin);

        if (Helpers::IconButton(ICON_REG_ARROW_RIGHT, btnSize, !ctx.navigation.CanGoForward())){
            ctx.navigation.GoForward();
        }
        ImGui::SameLine(0, margin);
            
        if (Helpers::IconButton(ICON_REG_ARROW_UP, btnSize, !ctx.navigation.CanGoParent())){
            ctx.navigation.GoParent();
        }
        ImGui::SameLine(0, margin);
            

        if (Helpers::IconButton(ICON_REG_ARROW_CLOCKWISE, btnSize)){
            ctx.navigation.Refresh();
        }

        ImGui::PopStyleVar();
        ImGui::EndChild();
    }

} // namespace NavBar