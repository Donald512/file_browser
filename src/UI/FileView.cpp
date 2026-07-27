#include "UI.h"
#include <algorithm>

namespace Style = UI::Style;
namespace Colors = UI::Colors;
using namespace FileView;

struct GridViewParams {
    f32 iconSize;      
    int maxTextLines;
    f32 xPadding;      // gap between cells
    f32 yGap;          // gap between icon bottom and text top
};

void RenderGrid(AppContext& ctx, const GridViewParams& params);

static GridViewParams GetGridParamsForMode(FileView::ViewMode mode) {
    switch (mode) {
        case FileView::ViewMode::ExtraLarge: return { 218.0f, 4, 16.0f, 8.0f };
        case FileView::ViewMode::Large:      return { 96.0f,  4, 12.0f, 6.0f };
        case FileView::ViewMode::Medium:     return { 48.0f,  4, 12.0f, 4.0f };
        case FileView::ViewMode::Small:      return { 24.0f,  4, 8.0f,  4.0f };
        default:                             return { 96.0f,  4, 12.0f, 6.0f };
    }
}

static int ShilSizeFromViewMode(ViewMode viewMode){
    // todo relook this
    switch (viewMode) {
        case FileView::ViewMode::ExtraLarge: return SHIL_JUMBO;
        case FileView::ViewMode::Large:      return SHIL_JUMBO;
        case FileView::ViewMode::Medium:     return SHIL_EXTRALARGE;
        case FileView::ViewMode::Small:      return SHIL_LARGE;
        default:                             return SHIL_JUMBO;
    }
}

void RenderGrid(AppContext& ctx, const GridViewParams& params){       
    if (!ImGui::BeginChild("FileView", Style::AutoFillRemnantWindow, ImGuiChildFlags_Borders, ImGuiChildFlags_NavFlattened)){
        ImGui::EndChild();
        return;
    }

    f32 dpi = ctx.ui.dpiScale;
    f32 iconSize = params.iconSize * dpi;
    f32 cellWidth = iconSize + (params.xPadding * dpi);
    f32 textLineHeight = ImGui::GetTextLineHeightWithSpacing();
    f32 maxTextHeight = textLineHeight * params.maxTextLines;
    f32 cellHeight = iconSize + (params.yGap * dpi) + maxTextHeight;

    f32 availWidth = ImGui::GetContentRegionAvail().x;
    u16 columnsCount = (u16)(availWidth / cellWidth);
    columnsCount = columnsCount ? columnsCount : 1;

    if (ImGui::BeginTable("ExplorerGrid", columnsCount, ImGuiTableFlags_NoSavedSettings)){
        auto &dir = ctx.navigation.Contents().Items();
        u16 totalRows = (u16) (dir.size() + columnsCount - 1)/columnsCount; // ceiling

        ImGuiListClipper clipper;
        clipper.Begin(totalRows, cellHeight); // fixed row height
        
        // instead of looping through, entries, we loop through visible rows
        while (clipper.Step()){
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++){  // DisplayStart is the first row, DisplayEnd is exclusive
                u64 startIdx = (u64) row * columnsCount;
                u64 endIdx = (std::min)((startIdx + columnsCount), dir.size());

                for (u64 i = startIdx; i < endIdx; i++){
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
                    ImTextureID iconTexture = ctx.icons.GetTexture({item.iconKey, ShilSizeFromViewMode(currentView)});
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
                    ImGui::Dummy(ImVec2(0, params.yGap * dpi));

                    ImGui::SetCursorPosX(columnStartX);
                    UI::Helpers::DrawCenteredWrappedText(item.name.c_str(), realCellWidth, params.maxTextLines);

                    ImGui::EndGroup();
                    ImGui::PopID();
                }
            }
        }
        ImGui::EndTable();
    }
    ImGui::EndChild();
}
    
void FileView::Render(AppContext& ctx){
    GridViewParams params = GetGridParamsForMode(FileView::currentView);
    RenderGrid(ctx, params);
}