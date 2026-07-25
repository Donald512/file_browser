#include "UI.h"
#include <unordered_map>

namespace Style = UI::Style;

namespace Sidebar {
    struct SidebarNodeState{
        bool isOpen = false;
        bool childrenLoaded = false;
        std::vector<WShell::ItemLite> children;   // reuse the type you already built for breadcrumb popups
    };

    static std::unordered_map<const void*, SidebarNodeState> s_nodeState; 

    // Helper function to draw a clean tree node with an icon
    bool RenderSidebarNode(AppContext& ctx, const char* label, ImTextureID icon, bool hasChildren, bool isSelected, bool* isOpen){
        ImGui::PushID(label);
        f32 rowHeight = ImGui::GetFrameHeight();
        f32 arrowWidth = rowHeight;

        if (hasChildren){
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
            const char* arrow = *isOpen ? ICON_REG_CHEVRON_DOWN : ICON_REG_CHEVRON_RIGHT;
            if (ImGui::Button(arrow, ImVec2(arrowWidth, rowHeight))){
                ImVec2 rowStart = ImGui::GetCursorScreenPos();
                *isOpen = !*isOpen;
            }
            ImGui::PopStyleColor();
        }
        else{
            ImGui::Dummy(ImVec2(hasChildren ? 0 : arrowWidth, rowHeight)); // keeps leaf rows aligned with expandable ones, no click target
        }
        ImGui::SameLine(0, 0);
        
        // Selectable now only covers the row AFTER the arrow 
        ImVec2 rowStart = ImGui::GetCursorScreenPos();
        bool clicked = ImGui::Selectable("##row", isSelected, ImGuiSelectableFlags_None, ImVec2(0, rowHeight));
    
        ImGui::SameLine(0, 0);

        f32 buttonY = rowStart.y + (ImGui::GetFrameHeight() - ImGui::GetFontSize()) * 0.5f;
        ImGui::SetCursorScreenPos(ImVec2(rowStart.x, buttonY));
        
        if (icon){
            f32 iconSize = 16.0f * ctx.ui.dpiScale;
            ImGui::Image(icon, ImVec2(iconSize, iconSize));
            ImGui::SameLine(0, 2);
        }
        ImGui::TextUnformatted(label);

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
                bool childHasSub = WShell::PidlHasSubFolders(child.pidl.get());
                ImTextureID childIcon = ctx.icons.GetTexture(Icons::GetIconIndex(child.pidl.get(), 0, 0, SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON));
                RenderNodeAndChildren(ctx, child.name, child.pidl, childIcon, childHasSub);   // recursion — a node's children are rendered the same way a node is
            }
            ImGui::Unindent();
        }
    }

    void Render(AppContext& ctx, f32 currentWidth) {
        // Begin the sidebar window with the specific width passed by the resizer
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,  8.0f * ctx.ui.dpiScale); 
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, Style::NoBorder);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f * ctx.ui.dpiScale, 4.0f * ctx.ui.dpiScale));
        if (!ImGui::BeginChild("Sidebar", ImVec2(currentWidth, 0), ImGuiChildFlags_None)) {
            ImGui::PopStyleVar(2);
            ImGui::EndChild();
            return;
        }

        ImGui::Dummy(ImVec2(0.0f, 4.0f));
        for (auto& item : ctx.items1){
            bool isSelected = ILIsEqual(ctx.navigation.CurrentFolder(), item.pidl);
            ImTextureID tex = ctx.icons.GetTexture(item.iconKey);
            RenderNodeAndChildren(ctx, item.name, item.pidl, tex, item.hasSubFolder);
        }
        ImGui::Dummy(ImVec2(0.0f, SectionPaddingY));
        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

        ImGui::Dummy(ImVec2(0.0f, SectionPaddingY));
        for (auto& item : ctx.items2){
            bool isSelected = ILIsEqual(ctx.navigation.CurrentFolder(), item.pidl);
            ImTextureID tex = ctx.icons.GetTexture(item.iconKey);
            RenderNodeAndChildren(ctx, item.name, item.pidl, tex, false);
        }
        ImGui::Dummy(ImVec2(0.0f, SectionPaddingY));
        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
        
        ImGui::Dummy(ImVec2(0.0f, SectionPaddingY));
        for (auto& item : ctx.items3){
            bool isSelected = ILIsEqual(ctx.navigation.CurrentFolder(), item.pidl);
            ImTextureID tex = ctx.icons.GetTexture(item.iconKey);
            RenderNodeAndChildren(ctx, item.name, item.pidl, tex, item.hasSubFolder);
        }
        ImGui::PopStyleVar(3);
        ImGui::EndChild();
    }
}