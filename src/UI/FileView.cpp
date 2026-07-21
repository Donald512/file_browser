#include "UI.h"
// #include "..\Icons\icons.h"

namespace Colors = UI::Colors;
namespace Style = UI::Style;

void FileView::Render(AppContext& ctx){        
    if (!ImGui::BeginChild("FileView", Style::AutoFillRemnantWindow, ImGuiChildFlags_Borders, ImGuiChildFlags_NavFlattened)){
        ImGui::EndChild();
        return;
    }
    
    f32 availWidth = ImGui::GetContentRegionAvail().x;    // Calculate how much width is available currently
    f32 iconSize = IconSize * ctx.dpiScale;
    f32 cellWidth  = (IconSize + XPadding) * ctx.dpiScale;

    // Determine how many columns can fit based on width. Minimum 1 column
    u16 columnsCount = (u16) (availWidth / cellWidth);
    columnsCount = columnsCount ? columnsCount: 1;  // if columnsCount is 0, make it 1


    if (ImGui::BeginTable("ExplorerGrid", columnsCount, ImGuiTableFlags_NoSavedSettings)){
        auto &dir = ctx.navigation.Contents().Items();

        u16 totalRows = (u16) (dir.size() + columnsCount - 1)/columnsCount; // the + columnsCount - 1/ columnsCount is there to celiling the result
        ImGuiListClipper clipper;
        clipper.Begin(totalRows);
        
        // instead of looping through, entries, we loop through visible rows
        while (clipper.Step()){
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++){  // DisplayStart is the first row, DisplayEnd is exclusive
                u64 startItemIdx = (u64) row * columnsCount;
                u64 endItemIdex = startItemIdx + columnsCount;
                endItemIdex = (endItemIdex < dir.size()) ? endItemIdex : dir.size();  // get the min, important for last row

                for (u64 i = startItemIdx; i < endItemIdex; i++){
                    ImGui::TableNextColumn();

                    f32 realCellWidth = ImGui::GetContentRegionAvail().x;
                    
                    auto& item = dir[i];
                    bool isFolder = item.attributes & SFGAO_FOLDER;
                    bool isSelected = (ctx.navigation.Contents().Selected() == i);

                    ImGui::PushID((int)i);
                    ImVec2 startPos = ImGui::GetCursorPos(); // save where we are to draw an invisible bounding box first

                    if (ImGui::Selectable("##file_selectedbox", isSelected, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(cellWidth, iconSize + ImGui::GetTextLineHeightWithSpacing() * 2))){
                        ctx.navigation.Contents().SelectIndex(i);
                    }
                                        
                    //  - INTERATION HANDLING -
                    // Hovering
                    if (ImGui::IsItemHovered()){
                        ImGui::SetTooltip("Type: %s", isFolder ? "Folder" : "File");
                    }
                    // clicking
                    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)){
                        ctx.navigation.Contents().SelectIndex(i);
                    }
                    // Double click or enter key
                    if ((ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && ImGui::IsItemHovered()) || (isSelected && ImGui::IsKeyPressed(ImGuiKey_Enter ))){
                        if (isFolder){
                            ctx.navigation.NavigateTo(dir[i].pidl);
                            ImGui::PopID();
                            ImGui::EndTable();
                            ImGui::EndChild();
                            return;
                        }
                        else{
                           WShell::ExecuteFile(dir[i].pidl);
                        }
                    }
                    // Move cursor back to start so we can draw visuals on top of the selectable
                    ImGui::SetCursorPos(startPos);
                    ImGui::BeginGroup();

                    f32 columnStartX = ImGui::GetCursorPosX();
                    f32 centeredIconX = columnStartX + (realCellWidth - iconSize) * 0.5f;
                    ImGui::SetCursorPosX(centeredIconX);

                    // Draw a colored box as placeholder using ImDrawList, so it doesnt steal clicks like Button()
                    ImTextureID iconTexture = ctx.icons.GetTexture(item.iconCacheKey);
                    if (iconTexture){
                        ImGui::Image(iconTexture, ImVec2(iconSize, iconSize));
                    }
                    else{
                        ImVec2 p_min = ImGui::GetCursorScreenPos();
                        ImVec2 p_max = ImVec2(p_min.x + iconSize, p_min.y + iconSize);
                        ImU32 iconColor = isFolder ? IM_COL32(204, 165, 51, 255) : IM_COL32(76, 127, 178, 255);
                        ImGui::GetWindowDrawList()->AddRectFilled(p_min, p_max, iconColor, 4.0f); // 4.0f is corner rounding
                    }
                    
                    // Push the layout cursor past our custom drawn box
                    ImGui::Dummy(ImVec2(iconSize, iconSize));
                    
                    // center text
                    f32 textWidth = ImGui::CalcTextSize(item.name.c_str()).x; 
                    if (textWidth < realCellWidth){
                        ImGui::SetCursorPosX(columnStartX + (realCellWidth - textWidth) * 0.5f);
                    }
                    else{
                        ImGui::SetCursorPosX(columnStartX);
                    }

                    // Wraps text smoothly if name exceeds column size limit
                    ImGui::PushTextWrapPos(columnStartX + realCellWidth);
                    ImGui::Text("%s", item.name.c_str());
                    ImGui::PopTextWrapPos();
                    
                    ImGui::EndGroup();
                    ImGui::PopID();
                }
            }
        }
        ImGui::EndTable();
    }
    ImGui::EndChild();
}
    