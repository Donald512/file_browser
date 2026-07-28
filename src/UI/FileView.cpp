#include "UI.h"
#include <algorithm>
#include <cmath>

namespace Style = UI::Style;
namespace Colors = UI::Colors;
namespace Helpers = UI::Helpers;
using namespace FileView;

struct GridViewParams {
    f32 width;
    f32 height;  
};

void RenderGrid(AppContext& ctx, const GridViewParams& params);

static GridViewParams GetGridParamsForMode(ViewMode mode) {
    switch (mode) {
        case ViewMode::ExtraLarge: return {271.0f, 260.0f};
        case ViewMode::Large:      return {105.0f, 100.0f };
        case ViewMode::Medium:     return {74.00f, 52.0f};
        case ViewMode::Small:      return {308.0f, 30.0f};
        default:                   return GetGridParamsForMode(ViewMode::Large);
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
void RenderGrid(AppContext& ctx, GridViewParams& params){ // MAIN ONE
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, WindowPadding);
    if (!ImGui::BeginChild("FileView", Style::AutoFillRemnantWindow, ImGuiChildFlags_Borders, ImGuiChildFlags_NavFlattened)){
        ImGui::PopStyleVar();
        ImGui::EndChild();
        return;
    }
    ImGui::PopStyleVar();

    f32 dpi = ctx.ui.dpiScale;
    f32 xGap = XGap * dpi;
    f32 lineHeight = ImGui::GetTextLineHeight();
    
    f32 imageHeightRegion = params.height * dpi;
    f32 iconSize = imageHeightRegion * ImageToContainerRatio; 
    
    // Calculate a STRICT max stride for the row so the ListClipper never overlaps items.
    // The visual selectable box will be smaller than this, but the grid spacing must be consistent!
    f32 maxRowHeight = imageHeightRegion + (4  * lineHeight) + (8.0f * dpi);

    f32 availWidth = ImGui::GetContentRegionAvail().x;
    u16 columnsCount = (u16)(availWidth / ((params.width + XGap) * dpi));
    columnsCount = (std::max)(columnsCount, (u16)1);

    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(xGap * 0.5f, xGap * 0.5f));

    if (ImGui::BeginTable("ExplorerGrid", columnsCount, ImGuiTableFlags_NoSavedSettings)){
        auto &dir = ctx.navigation.Contents().Items();
        u16 totalRows = (u16)(std::ceil((f32)dir.size() / columnsCount));

        ImGuiListClipper clipper;
        // CRITICAL: We pass maxRowHeight so the clipper strides perfectly and prevents overlap!
        clipper.Begin(totalRows, maxRowHeight); 

        while (clipper.Step()){
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++){
                u64 startIdx = (u64) row * columnsCount;
                u64 endIdx = (std::min)((u64)(startIdx + columnsCount), (u64)dir.size());

                for (u64 i = startIdx; i < endIdx; i++){
                    ImGui::TableNextColumn();

                    f32 realCellWidth = ImGui::GetContentRegionAvail().x;
                    auto& item = dir[i];
                    bool isFolder = item.attributes & SFGAO_FOLDER;
                    bool isSelected = (ctx.navigation.Contents().Selected() == i);
                    
                    //  PREVENT CUT-OFF: Ensure text doesn't bleed out of small columns ---
                    f32 idealTextWidth = TextToContainerWidthRatio * params.width * dpi;
                    f32 actualMaxTextWidth = (std::min)(idealTextWidth, realCellWidth - (4.0f * dpi));

                    // -DYNAMIC SELECTABLE SIZE ---
                    // Peek at how many lines this specific item takes up to size its visual box!
                    f32 rawTextHeight = ImGui::CalcTextSize(item.name.c_str(), NULL, false, actualMaxTextWidth).y;
                    int actualLines = (int)std::ceil((float)rawTextHeight / ImGui::GetTextLineHeight());
                    actualLines = std::clamp(actualLines, 1, 4); // clamp between 1 and 4 lines
                    
                    // The visual box is EXACTLY the image region + the lines of text.
                    f32 selectableHeight = imageHeightRegion + (actualLines * lineHeight) + (4.0f * dpi);

                    ImGui::PushID((int)i);  
                    ImVec2 startPos = ImGui::GetCursorPos();

                    // 1. Draw Selectable with the EXACT dynamic height
                    if (ImGui::Selectable("##file_selectedbox", isSelected, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(realCellWidth, selectableHeight))){
                        ctx.navigation.Contents().SelectIndex(i);
                    }

                    if (ImGui::IsItemHovered()){
                        ImGui::SetTooltip("Type: %s", isFolder ? "Folder" : "File");
                    }
                    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)){
                        ctx.navigation.Contents().SelectIndex(i);
                    }
                    if ((ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && ImGui::IsItemHovered()) || (isSelected && ImGui::IsKeyPressed(ImGuiKey_Enter ))){
                        if (isFolder){
                            ctx.navigation.NavigateTo(dir[i].pidl);
                            ImGui::PopID();
                            ImGui::EndTable();
                            ImGui::PopStyleVar();
                            ImGui::EndChild();
                            return;
                        }
                        else WShell::ExecuteFile(dir[i].pidl);
                    }

                    // 2. Draw Image perfectly centered in the imageHeightRegion
                    ImGui::SetCursorPos(ImVec2(startPos.x + (realCellWidth - iconSize) * 0.5f, 
                                               startPos.y + (imageHeightRegion - iconSize) * 0.5f));

                    ImTextureID iconTexture = ctx.icons.GetTexture({item.IconKey(), ShilSizeFromViewMode(currentView)});
                    if (iconTexture){
                        ImGui::Image(iconTexture, ImVec2(iconSize, iconSize));
                    }
                    else{
                        ImVec2 p_min = ImGui::GetCursorScreenPos();
                        ImVec2 p_max = ImVec2(p_min.x + iconSize, p_min.y + iconSize);
                        ImU32 iconColor = isFolder ? IM_COL32(204, 165, 51, 255) : IM_COL32(76, 127, 178, 255);
                        ImGui::GetWindowDrawList()->AddRectFilled(p_min, p_max, iconColor, 4.0f);
                    }
                    
                    // 3. Draw Text exactly below the image container
                    ImGui::SetCursorPos(ImVec2(startPos.x, startPos.y + imageHeightRegion));
                    Helpers::DrawCenteredWrappedText(item.name.c_str(), realCellWidth, actualMaxTextWidth, 4);
                    
                    ImGui::SetCursorPos(startPos);
                    ImGui::Dummy(ImVec2(realCellWidth, maxRowHeight));
                    
                    ImGui::PopID();
                }
            }
        }
        ImGui::EndTable();
    }
    ImGui::PopStyleVar();
    ImGui::EndChild();
}

void RenderViewSmall(AppContext& ctx, GridViewParams& params){
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, WindowPadding);
    if (!ImGui::BeginChild("FileView", Style::AutoFillRemnantWindow, ImGuiChildFlags_Borders, ImGuiChildFlags_NavFlattened)){
        ImGui::PopStyleVar();
        ImGui::EndChild();
        return;
    }
    ImGui::PopStyleVar();

    f32 dpi = ctx.ui.dpiScale;
    f32 xGap = XGap * dpi;
    f32 lineHeight = ImGui::GetTextLineHeight();
    
    f32 cellHeight = params.height * dpi;
    f32 iconSize = cellHeight * ImageToContainerRatio; 
    
    f32 availWidth = ImGui::GetContentRegionAvail().x;
    u16 columnsCount = (u16)(availWidth / ((params.width + XGap) * dpi));
    columnsCount = (std::max)(columnsCount, (u16)1);

    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(xGap * 0.5f, xGap * 0.5f));

    if (ImGui::BeginTable("ExplorerGrid", columnsCount, ImGuiTableFlags_NoSavedSettings)){
        auto &dir = ctx.navigation.Contents().Items();
        u16 totalRows = (u16)(std::ceil((f32)dir.size() / columnsCount));

        ImGuiListClipper clipper;
        clipper.Begin(totalRows, cellHeight); 

        while (clipper.Step()){
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++){
                u64 startIdx = (u64) row * columnsCount;
                u64 endIdx = (std::min)((u64)(startIdx + columnsCount), (u64)dir.size());

                for (u64 i = startIdx; i < endIdx; i++){
                    ImGui::TableNextColumn();

                    f32 realCellWidth = ImGui::GetContentRegionAvail().x;
                    auto& item = dir[i];
                    bool isFolder = item.attributes & SFGAO_FOLDER;
                    bool isSelected = (ctx.navigation.Contents().Selected() == i);
                
                    ImGui::PushID((int)i);  
                    ImVec2 startPos = ImGui::GetCursorPos();

                    // 1. Draw Selectable
                    if (ImGui::Selectable("##file_selectedbox", isSelected, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(realCellWidth, cellHeight))){
                        ctx.navigation.Contents().SelectIndex(i);
                    }
                    if (ImGui::IsItemHovered()){
                        ImGui::SetTooltip("Type: %s", isFolder ? "Folder" : "File");
                    }
                    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)){
                        ctx.navigation.Contents().SelectIndex(i);
                    }
                    if ((ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && ImGui::IsItemHovered()) || (isSelected && ImGui::IsKeyPressed(ImGuiKey_Enter ))){
                        if (isFolder){
                            ctx.navigation.NavigateTo(dir[i].pidl);
                            ImGui::PopID();
                            ImGui::EndTable();
                            ImGui::PopStyleVar();
                            ImGui::EndChild();
                            return;
                        }
                        else WShell::ExecuteFile(dir[i].pidl);
                    }

                    // 2. Draw Icon (Calculated explicit exact position)
                    f32 iconPaddingX = 6.0f * dpi; 
                    f32 iconX = startPos.x + iconPaddingX;
                    f32 iconY = startPos.y + (cellHeight - iconSize) * 0.5f; // Perfect vertical center
                    
                    ImGui::SetCursorPos(ImVec2(iconX, iconY));
                    
                    ImTextureID iconTexture = ctx.icons.GetTexture({item.IconKey(), ShilSizeFromViewMode(currentView)});
                    if (iconTexture){
                        ImGui::Image(iconTexture, ImVec2(iconSize, iconSize));
                    }
                    else{
                        ImVec2 p_min = ImGui::GetCursorScreenPos();
                        ImVec2 p_max = ImVec2(p_min.x + iconSize, p_min.y + iconSize);
                        ImU32 iconColor = isFolder ? IM_COL32(204, 165, 51, 255) : IM_COL32(76, 127, 178, 255);
                        ImGui::GetWindowDrawList()->AddRectFilled(p_min, p_max, iconColor, 4.0f);
                    }

                    // 3. Draw Text (Calculated explicit exact position)
                    f32 textGapX = 8.0f * dpi;
                    f32 textX = iconX + iconSize + textGapX;
                    f32 textY = startPos.y + (cellHeight - lineHeight) * 0.5f; // Perfect vertical center
                    
                    ImGui::SetCursorPos(ImVec2(textX, textY));
                    
                    // Actually render the text this time!
                    ImGui::TextUnformatted(item.name.c_str());

                    // 4. Safely advance layout cursor
                    ImGui::SetCursorPos(startPos);
                    ImGui::Dummy(ImVec2(realCellWidth, cellHeight));
                    ImGui::PopID();
                }
            }
        }
        ImGui::EndTable();
    }
    ImGui::PopStyleVar();
    ImGui::EndChild();
}

void FileView::Render(AppContext& ctx){
    GridViewParams params = GetGridParamsForMode(FileView::currentView);
    switch (currentView){
        case ViewMode::ExtraLarge: case ViewMode::Large: case ViewMode::Medium:{
            RenderGrid(ctx, params);
        }
        break;
        case ViewMode::Small:{
            RenderViewSmall(ctx, params);
        }
        break;
    }

    
}