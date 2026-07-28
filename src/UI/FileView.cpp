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
        case ViewMode::List:       return {308.0f, 30.0f};
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
                    f32 textAvailWidth = (startPos.x + realCellWidth) - textX;

                    ImGui::SetCursorPos(ImVec2(textX, textY));
                    
                    // Actually render the text this time!
                    UI::Helpers::DrawSingleLineTruncatedText(item.name.c_str(), textAvailWidth);

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

void RenderViewList(AppContext& ctx, GridViewParams& params){
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, WindowPadding);
    
    // FIX 1: Separate Child Flags and Window Flags properly!
    ImGuiChildFlags childFlags = ImGuiChildFlags_Borders | ImGuiChildFlags_NavFlattened;
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_HorizontalScrollbar; // Force horizontal scroll
    
    if (!ImGui::BeginChild("FileViewList", Style::AutoFillRemnantWindow, childFlags, windowFlags)){
        ImGui::PopStyleVar();
        ImGui::EndChild();
        return;
    }
    ImGui::PopStyleVar();

    f32 dpi = ctx.ui.dpiScale;

    // - Map vertical mouse wheel to horizontal scrolling ---
    if (ImGui::IsWindowHovered() && ImGui::GetIO().MouseWheel != 0.0f) {
        // Scroll speed: You can adjust the 60.0f to make it scroll faster or slower
        f32 scrollAmount = ImGui::GetIO().MouseWheel * (60.0f * dpi); 
        ImGui::SetScrollX(ImGui::GetScrollX() - scrollAmount);
    }
    
    f32 xGap = XGap * dpi;
    f32 cellHeight = params.height * dpi;
    f32 iconSize = cellHeight * ImageToContainerRatio; 
    
    // Set a minimum and maximum limit for column widths so they don't look ridiculous
    f32 minColWidth = 120.0f * dpi; 
    f32 maxColWidth = params.width * dpi; // (e.g. 308px based on your GetGridParamsForMode)

    f32 availY = ImGui::GetContentRegionAvail().y;
    
    // Calculate how many rows fit vertically.
    int rowsPerColumn = (int)(availY / cellHeight);
    if (rowsPerColumn < 1) rowsPerColumn = 1;

    auto &dir = ctx.navigation.Contents().Items();
    int totalItems = (int)dir.size();
    
    if (totalItems == 0) {
        ImGui::EndChild();
        return;
    }

    int totalColumns = (totalItems + rowsPerColumn - 1) / rowsPerColumn; // Ceiling division

    // --- FIX 2: DYNAMIC COLUMN WIDTH CALCULATION ---
    // Pass 1: Find the maximum width needed for each column
    std::vector<f32> colWidths(totalColumns, minColWidth);
    for (int i = 0; i < totalItems; i++) {
        int c = i / rowsPerColumn;
        
        // Measure text exactly as it will render (unformatted single line)
        f32 textWidth = ImGui::CalcTextSize(dir[i].name.c_str()).x;
        f32 requiredWidth = textWidth + iconSize + (xGap * 3.0f); // icon + text + padding
        
        // Keep the largest width, but clamp it to our maximum allowed width
        if (requiredWidth > colWidths[c]) {
            colWidths[c] = (std::min)(requiredWidth, maxColWidth);
        }
    }

    // Pass 2: Calculate the absolute starting X position (offset) for every column
    std::vector<f32> colOffsets(totalColumns + 1, 0.0f);
    for (int c = 0; c < totalColumns; c++) {
        colOffsets[c + 1] = colOffsets[c] + colWidths[c];
    }
    
    f32 totalVirtualWidth = colOffsets[totalColumns];

    // --- ALLOCATE VIRTUAL SPACE ---
    // Now that we have the exact dynamic width, we force the scrollbar to match it!
    ImGui::Dummy(ImVec2(totalVirtualWidth, rowsPerColumn * cellHeight));

    // --- CUSTOM HORIZONTAL CLIPPER ---
    f32 scrollX = ImGui::GetScrollX();
    f32 windowX = ImGui::GetWindowWidth();

    // Figure out which columns are currently visible on screen
    int startCol = 0;
    while (startCol < totalColumns && colOffsets[startCol + 1] < scrollX) {
        startCol++;
    }
    
    int endCol = startCol;
    while (endCol < totalColumns && colOffsets[endCol] < scrollX + windowX) {
        endCol++;
    }

    // --- DRAW VISIBLE ITEMS ---
    for (int c = startCol; c < endCol; c++){
        
        f32 currentColWidth = colWidths[c];
        f32 currentColOffset = colOffsets[c];
        
        for (int r = 0; r < rowsPerColumn; r++){
            int i = (c * rowsPerColumn) + r;
            if (i >= totalItems) break; // Reached the end of the files!

            auto& item = dir[i];
            bool isFolder = item.attributes & SFGAO_FOLDER;
            bool isSelected = (ctx.navigation.Contents().Selected() == i);

            ImGui::PushID(i);
            
            // Calculate absolute top-left coordinate of this specific cell
            ImVec2 cellPos(currentColOffset, r * cellHeight);
            
            // Move ImGui's layout cursor to this spot
            ImGui::SetCursorPos(cellPos);

            // A. Draw Selectable bounding box
            if (ImGui::Selectable("##file_selectedbox", isSelected, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(currentColWidth, cellHeight))){
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
                    ImGui::EndChild();
                    return;
                }
                else WShell::ExecuteFile(dir[i].pidl);
            }

            // B. Draw Icon (Vertically centered)
            f32 iconY = cellPos.y + (cellHeight - iconSize) * 0.5f;
            ImGui::SetCursorPos(ImVec2(cellPos.x + xGap, iconY));
            
            ImTextureID iconTexture = ctx.icons.GetTexture({item.IconKey(), SHIL_SMALL});
            if (iconTexture){
                ImGui::Image(iconTexture, ImVec2(iconSize, iconSize));
            }
            else{
                ImVec2 p_min = ImGui::GetCursorScreenPos();
                ImVec2 p_max = ImVec2(p_min.x + iconSize, p_min.y + iconSize);
                ImU32 iconColor = isFolder ? IM_COL32(204, 165, 51, 255) : IM_COL32(76, 127, 178, 255);
                ImGui::GetWindowDrawList()->AddRectFilled(p_min, p_max, iconColor, 4.0f);
            }

            // C. Draw Text (Vertically centered, truncated if necessary)
            f32 textStartX = cellPos.x + xGap + iconSize + xGap;
            f32 textY = cellPos.y + (cellHeight - ImGui::GetTextLineHeight()) * 0.5f;
            
            ImGui::SetCursorPos(ImVec2(textStartX, textY));
            
            // We give it the remaining space inside THIS specific dynamic column
            f32 maxTextWidth = currentColWidth - (xGap * 3.0f) - iconSize;
            UI::Helpers::DrawSingleLineTruncatedText(item.name.c_str(), maxTextWidth);

            ImGui::PopID();
        }
    }
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
        case ViewMode::List:{
            RenderViewList(ctx, params);
        }
        break;
    }

    
}