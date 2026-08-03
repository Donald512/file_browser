#include "UI.h"



void RenderContextMenuStructure(AppContext& ctx, const std::vector<WShell::ContextMenuItem>& items) {
    float dpi = ctx.ui.dpiScale;

    float iconSize = 16.0f * dpi;
    float iconSpacing = 6.0f * dpi;

    // How many leading spaces are needed to reserve iconSize + iconSpacing
    // of blank space before the real label text.
    float spaceWidth = ImGui::CalcTextSize(" ").x;
    int numSpaces = (int)std::ceil((iconSize + iconSpacing) / spaceWidth);
    std::string padding(numSpaces, ' ');

    int i = 0;
    for (const auto& item : items) {
        if (item.isSeparator) {
            ImGui::Separator();
            continue;
        }
        if (item.text.empty()) continue;

        std::string label = padding + item.text + "##" + std::to_string(i);

        bool isSubMenu = !item.subItems.empty();
        bool open = false;
        bool clicked = false;

        // No SetCursorPosX shift here — the widget's hit-rect starts
        // at the true left edge, so hover covers the icon column too.
        if (isSubMenu) {
            open = ImGui::BeginMenu(label.c_str(), item.enabled);
        } else {
            clicked = ImGui::MenuItem(label.c_str(), nullptr, item.checked, item.enabled);
        }

        // Draw the icon manually into the blank space reserved by `padding`.
        if (item.hIconTex) {
            ImVec2 itemMin = ImGui::GetItemRectMin();
            ImVec2 itemMax = ImGui::GetItemRectMax();
            float rowHeight = itemMax.y - itemMin.y;
            float offsetY = (rowHeight - iconSize) * 0.5f;

            ImVec2 iconMin = ImVec2(itemMin.x, itemMin.y + offsetY);
            ImVec2 iconMax = ImVec2(iconMin.x + iconSize, iconMin.y + iconSize);

            ImGui::GetWindowDrawList()->AddImage(item.hIconTex, iconMin, iconMax);
        }

        if (isSubMenu && open) {
            RenderContextMenuStructure(ctx, item.subItems);
            ImGui::EndMenu();
        }
        else if (!isSubMenu && clicked) {
            if (item.verb == "rename") {
                // Handle rename
            }
            else {
                WShell::ExecuteContextMenuCommand(ctx.activeContextMenu, item.id);
            }
        }
        i++;
    }
}