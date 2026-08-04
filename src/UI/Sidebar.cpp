#include "UI.h"
#include "ShellAsync.h"
#include <unordered_map>

namespace Style = UI::Style;
namespace Colors = UI::Colors;

namespace Sidebar {
    struct SidebarNodeState{
        bool isOpen = false;
        enum class LoadState {NotLoaded, Loading, Loaded} childState = LoadState::NotLoaded;
        std::vector<WShell::ItemLite> children;   // reuse the type you already built for breadcrumb popups
    };
    bool documentsDropdownOpen = true;

    // Keyed by WShell::HashPidl(pidl) - content hash - Not by the pidl's raw address.

    static std::unordered_map<u64, SidebarNodeState> s_nodeState; 

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

    // 'owner' is whichever 'item' actually lives in one of ctx.items1/2/3 or a perent node's 'state.children' - passed through so an async icon/hasSubFolders request knows where to patch its result back into once it completes. 
    // 'allowExpand' creates the behaviour where the Quick access section (items2) dont show an arrow regardless of whether the target has subfolder.
    void RenderNodeAndChildren(AppContext& ctx, std::vector<WShell::ItemLite>& owner, WShell::ItemLite& item, bool allowExpand = true){
        u64 key = WShell::HashPidl(item.pidl.get());
        SidebarNodeState& state = s_nodeState[key];   // creates on first access, persists across frames
        if (!item.iconKey.resolved && !item.iconRequestSent){
            WShell::Async::RequestLiteIcon(ctx, owner, item);
        }
        // Intentionally SHIL_SMALL, not SHIL_LARGE because SHIL_LARGE produces a weird onedrive icon
        ImTextureID icon = item.iconKey.resolved ? ctx.icons.GetTexture({item.iconKey.value, SHIL_SMALL}) : 0;

        bool hasChildren = false;
        if (allowExpand){
            if (!item.hasSubFolders.resolved && !item.hasSubFoldersRequestSent){
                WShell::Async::RequestHasSubFolders(ctx, owner, item);
            }
            hasChildren = item.hasSubFolders.resolved && item.hasSubFolders.value;
        }
        bool isSelected = ILIsEqual(ctx.navigation.CurrentFolder(), item.pidl.get());
        bool clicked = RenderSidebarNode(ctx, item.name.c_str(), icon, hasChildren, isSelected, &state.isOpen);

        if (clicked){
            ctx.navigation.NavigateTo(item.pidl.get());   // clicking the row (not the arrow) still navigates, same as before
        }

        if (state.isOpen && hasChildren){
            if (state.childState == SidebarNodeState::LoadState::NotLoaded){
                state.childState = SidebarNodeState::LoadState::Loading;
                ctx.tasks.RunAsync(
                    [pidl = item.pidl.Clone()]() mutable {
                        return WShell::GetLiteItems(pidl.get());
                    },
                    [&state](std::vector<WShell::ItemLite> result) mutable {
                        state.children = std::move(result);
                        state.childState = SidebarNodeState::LoadState::Loaded;
                    }
                );
            }
            if (state.childState == SidebarNodeState::LoadState::Loaded){
                ImGui::Indent();
                for (auto& child : state.children){
                    RenderNodeAndChildren(ctx, state.children, child); // recursion — a node's children are rendered the same way a node is
                } 
                ImGui::Unindent();
            }
            // else  Loading. Cosmetic filler
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
            RenderNodeAndChildren(ctx, ctx.items1, item);
        }
        ImGui::Dummy(ImVec2(0.0f, SectionPaddingY));
        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

        ImGui::Dummy(ImVec2(0.0f, SectionPaddingY));
        for (auto& item : ctx.items2){
            RenderNodeAndChildren(ctx, ctx.items2, item, false);   // Quick access: never expandable, same as before
        }
        ImGui::Dummy(ImVec2(0.0f, SectionPaddingY));
        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
        
        ImGui::Dummy(ImVec2(0.0f, SectionPaddingY));
        for (auto& item : ctx.items3){
            RenderNodeAndChildren(ctx, ctx.items3, item);
        }
        ImGui::PopStyleVar(2);
        ImGui::EndChild();
    }

    static bool s_openRecents   = true;
    static bool s_openBookmarks = true;
    static bool s_openPlaces    = true;

}


