#include "UI.h"
#include "ShellAsync.h"
#include <unordered_map>

namespace Style = UI::Style;
namespace Colors = UI::Colors;

namespace Sidebar {

    // Keyed by WShell::HashPidl(pidl) - content hash - Not by the pidl's raw address.

    struct Section {
        const char* id;
        const char* label;
        std::vector<WShell::Item>* items; 
        bool* isOpen;
    };

    void Render(AppContext& ctx, f32 currentWidth) {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, Style::NoBorder);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f * ctx.ui.dpiScale, 4.0f * ctx.ui.dpiScale));
        if (!ImGui::BeginChild("Sidebar", ImVec2(currentWidth, 0), ImGuiChildFlags_None)) {
            ImGui::PopStyleVar(2);
            ImGui::EndChild();
            return;
        }

        f32 dpi = ctx.ui.dpiScale;

        // Transitioning to just displaying the folders, no tree nodes
        // how do i handle hover, tool tip info showing details
        // right click shows Stuff, plus deleting items from list, e.g 
        // a tag/playlist, deleting the item might just be copying every item except that one, idk
        // will need to change from ItemLite to Item
        // td, make it possible to add items to the sidebar
        // td, add bookmarks

        // Gonna start with a default vector of vectors, and more can be pushedback into it

        static bool s_openDocuments = true;
        static bool s_openRecents   = true;
        static bool s_openStorages  = true;
        static bool s_openBookmarks = true;


        Section sections[] = {
            { "documents", ICON_REG_DOCUMENT " Documents",  &ctx.items1, &s_openDocuments },
            { "recents",   ICON_REG_CLOCK " Recents",      &ctx.items2, &s_openRecents },
            { "storages",  ICON_REG_HARD_DRIVE " Storages", &ctx.items3, &s_openStorages },
            { "bookmarks", ICON_REG_BOOKMARK " Bookmarks", nullptr, &s_openBookmarks }
        };

        for (size_t s = 0; s < IM_ARRAYSIZE(sections); s++){
            auto& sec = sections[s];
            UI::Helpers::RenderSectionHeader(sec.id, sec.label, dpi, sec.isOpen, {4, {8, 8}, 4}, 0);
            if (*sec.isOpen){
                if (!sec.items) continue;
                for (size_t i = 0; i < sec.items->size(); i++){
                    auto& item = (*(sec.items))[i];
                    ImGui::PushID((int)i);

                    std::string rowID = item.name + std::to_string(i);

                    if (!item.iconKey.resolved && !item.iconRequestSent){
                        WShell::Async::RequestIcon(ctx, *sec.items, item, i);
                    }

                    ImTextureID icon = item.iconKey.resolved ? ctx.icons.GetTexture({item.iconKey.value, SHIL_SMALL}) : 0;

                    if (UI::Helpers::MenuRow(rowID.c_str(), item.name.c_str(), icon, dpi, {4, {8, 4}, 4, {0, 0}}, 25, 6, 0)){
                        ctx.navigation.NavigateTo(item.pidl);
                    }
                    ImGui::PopID();

                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)){
                        if (item.tooltipInfo.resolved){
                            ImGui::SetTooltip("%s", item.tooltipInfo.value.c_str());
                        }
                        else if (!item.tooltipRequestSent){
                            WShell::Async::RequestTooltip(ctx, item, i);
                        }
                    }
                    
                }
            }
        }
        ImGui::PopStyleVar(2);
    
        ImGui::EndChild();
    }
}


