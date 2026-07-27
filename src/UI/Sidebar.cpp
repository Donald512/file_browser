#include "UI.h"
#include <unordered_map>

namespace Style = UI::Style;
namespace Colors = UI::Colors;

namespace Sidebar {
    struct SidebarNodeState{
        bool isOpen = false;
        bool childrenLoaded = false;
        std::vector<WShell::ItemLite> children;   // reuse the type you already built for breadcrumb popups
    };

    static std::unordered_map<const void*, SidebarNodeState> s_nodeState; 

    // Helper function to draw a clean tree node with an icon
    bool RenderSidebarNode(AppContext& ctx, const char* label, ImTextureID icon, bool hasChildren, bool isSelected, bool* isOpen) {
        ImGui::PushID((void*)label);
        
        f32 rowHeight = ImGui::GetFrameHeight();
        f32 arrowWidth = rowHeight;
        f32 availX = ImGui::GetContentRegionAvail().x;
        
        ImVec2 startPos = ImGui::GetCursorScreenPos();
        
        ImGui::Dummy(ImVec2(availX, rowHeight));

        f32 selectableX = startPos.x + arrowWidth;
        ImGui::SetCursorScreenPos(ImVec2(selectableX, startPos.y));
        bool clicked = ImGui::Selectable("##row", isSelected, ImGuiSelectableFlags_AllowOverlap, ImVec2(availX - arrowWidth, rowHeight));

        if (hasChildren) {
            ImGui::SetCursorScreenPos(startPos);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
            const char* arrow = *isOpen ? ICON_REG_CHEVRON_DOWN : ICON_REG_CHEVRON_RIGHT;
            if (ImGui::Button(arrow, ImVec2(arrowWidth, rowHeight))) {
                *isOpen = !*isOpen;
            }
            ImGui::PopStyleColor();
        }

        f32 currentX = selectableX + (4.0f * ctx.ui.dpiScale); // Padding after arrow
        if (icon) {
            f32 iconSize = 16.0f * ctx.ui.dpiScale;
            f32 iconY = startPos.y + (rowHeight - iconSize) * 0.5f; 
            
            ImGui::SetCursorScreenPos(ImVec2(currentX, iconY));
            ImGui::Image(icon, ImVec2(iconSize, iconSize));
            
            currentX += iconSize + (6.0f * ctx.ui.dpiScale); // Padding after icon
        }

        f32 textY = startPos.y + (rowHeight - ImGui::GetFontSize()) * 0.5f; 
        ImGui::SetCursorScreenPos(ImVec2(currentX, textY));
        ImGui::TextUnformatted(label);

        ImGui::SetCursorScreenPos(ImVec2(startPos.x, startPos.y + rowHeight + ImGui::GetStyle().ItemSpacing.y));

        ImGui::PopID();
        return clicked;
    }

    void RenderNodeAndChildren(AppContext& ctx, const std::string& name, const WShell::Pidl& pidl, ImTextureID icon, bool hasChildren){
        SidebarNodeState& state = s_nodeState[(const void*)pidl.get()];   // creates on first access, persists across frames

        bool isSelected = ILIsEqual(ctx.navigation.CurrentFolder(), pidl.get());
        bool clicked = RenderSidebarNode(ctx, name.c_str(), icon, hasChildren, isSelected, &state.isOpen);

        if (clicked){
            ctx.navigation.NavigateTo(pidl.get());   // clicking the row (not the arrow) still navigates, same as before
        }

        if (state.isOpen && hasChildren){
            if (!state.childrenLoaded){
                state.children = WShell::GetLiteItems(pidl.get());   // fetch ONCE, on first expand — not every frame
                state.childrenLoaded = true;
            }
            ImGui::Indent();
            for (auto& child : state.children){
                ImTextureID childIcon = ctx.icons.GetTexture({child.IconKey(), SHIL_LARGE});
                RenderNodeAndChildren(ctx, child.name, child.pidl, childIcon, child.HasSubFolders());   // recursion — a node's children are rendered the same way a node is
            }
            ImGui::Unindent();
        }
    }

    void Render(AppContext& ctx, f32 currentWidth) {
        // Begin the sidebar window with the specific width passed by the resizer
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, Style::NoBorder);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f * ctx.ui.dpiScale, 4.0f * ctx.ui.dpiScale));
        if (!ImGui::BeginChild("Sidebar", ImVec2(currentWidth, 0), ImGuiChildFlags_None)) {
            ImGui::PopStyleVar(2);
            ImGui::EndChild();
            return;
        }

        ImGui::Dummy(ImVec2(0.0f, 4.0f));
        for (auto& item : ctx.items1){
            ImTextureID tex = ctx.icons.GetTexture({item.IconKey(), SHIL_LARGE});
            RenderNodeAndChildren(ctx, item.name, item.pidl, tex, item.HasSubFolders());
        }
        ImGui::Dummy(ImVec2(0.0f, SectionPaddingY));
        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

        ImGui::Dummy(ImVec2(0.0f, SectionPaddingY));
        for (auto& item : ctx.items2){
            ImTextureID tex = ctx.icons.GetTexture({item.IconKey(), SHIL_LARGE});
            RenderNodeAndChildren(ctx, item.name, item.pidl, tex, false);
        }
        ImGui::Dummy(ImVec2(0.0f, SectionPaddingY));
        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
        
        ImGui::Dummy(ImVec2(0.0f, SectionPaddingY));
        for (auto& item : ctx.items3){
            ImTextureID tex = ctx.icons.GetTexture({item.IconKey(), SHIL_LARGE});
            RenderNodeAndChildren(ctx, item.name, item.pidl, tex, item.HasSubFolders());
        }
        ImGui::PopStyleVar(2);
        ImGui::EndChild();
    }
}