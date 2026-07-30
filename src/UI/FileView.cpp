#include "UI.h"
#include <algorithm>
#include <cmath>
#include "imgui_internal.h"

namespace Style = UI::Style;
namespace Colors = UI::Colors;
namespace Helpers = UI::Helpers;

using namespace FileView;

struct GridViewParams {
    f32 width;
    f32 height;  
};

static GridViewParams GetGridParamsForMode(ViewMode mode) {
    switch (mode) {
        case ViewMode::ExtraLarge: return {271.0f, 260.0f};
        case ViewMode::Large:      return {105.0f, 100.0f };
        case ViewMode::Medium:     return {74.00f, 52.0f};
        case ViewMode::Small:      return {308.0f, 30.0f};
        case ViewMode::List:       return {308.0f, 30.0f};
        case ViewMode::Details:    return {308.0f, 30.0f};
        case ViewMode::Tiles:      return {250.0f, 52.0f};
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

static bool HandleItemInteraction(AppContext& ctx, u64 index, const WShell::Item& item, bool isSelected, ImVec2 size, ImGuiSelectableFlags flags = ImGuiSelectableFlags_AllowDoubleClick, bool handleHover = true){
    if (ImGui::Selectable("##file_selectedbox", isSelected, flags, size)) {
        ctx.navigation.Contents().SelectIndex(index);
    }
    if (handleHover){
        if (ImGui::IsItemHovered()){
            ImGui::SetTooltip(item.TooltipInfo().c_str());
        }
    }
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
        ctx.navigation.Contents().SelectIndex(index);
    }
    
    bool isFolder = (item.attributes & SFGAO_FOLDER) != 0;
    
    if ((ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && ImGui::IsItemHovered()) || 
        (isSelected && ImGui::IsKeyPressed(ImGuiKey_Enter))) {
        if (isFolder) {
            ctx.navigation.NavigateTo(item.pidl);
            return true; // Navigated!
        } else {
            WShell::ExecuteFile(item.pidl);
        }
    }
    return false;
}

static void DrawItemIcon(AppContext& ctx, const WShell::Item& item, f32 iconSize, int shilSize){
    ImTextureID iconTexture = ctx.icons.GetTexture({item.IconKey(), shilSize});
    
    if (iconTexture) {
        ImGui::Image(iconTexture, ImVec2(iconSize, iconSize));
    } else {
        bool isFolder = (item.attributes & SFGAO_FOLDER) != 0;
        ImVec2 p_min = ImGui::GetCursorScreenPos();
        ImVec2 p_max = ImVec2(p_min.x + iconSize, p_min.y + iconSize);
        ImU32 iconColor = isFolder ? IM_COL32(204, 165, 51, 255) : IM_COL32(76, 127, 178, 255);
        ImGui::GetWindowDrawList()->AddRectFilled(p_min, p_max, iconColor, 4.0f);
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

                    if (HandleItemInteraction(ctx, i, item, isSelected, ImVec2(realCellWidth, selectableHeight))){
                        ImGui::PopID();
                        ImGui::EndTable();
                        ImGui::PopStyleVar();
                        ImGui::EndChild();
                        return;
                    }

                    ImGui::SetCursorPos(ImVec2(startPos.x + (realCellWidth - iconSize) * 0.5f, startPos.y + (imageHeightRegion - iconSize) * 0.5f));
                    DrawItemIcon(ctx, item, iconSize, ShilSizeFromViewMode(currentView));

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

                    if (HandleItemInteraction(ctx, i, item, isSelected, ImVec2(realCellWidth, cellHeight))){
                        ImGui::PopID();
                        ImGui::EndTable();
                        ImGui::PopStyleVar();
                        ImGui::EndChild();
                        return;
                    }
                    
                    // 2. Draw Icon (Calculated explicit exact position)
                    f32 iconPaddingX = 6.0f * dpi; 
                    f32 iconX = startPos.x + iconPaddingX;
                    f32 iconY = startPos.y + (cellHeight - iconSize) * 0.5f; // Perfect vertical center
                    
                    ImGui::SetCursorPos(ImVec2(iconX, iconY));
                    DrawItemIcon(ctx, item, iconSize, ShilSizeFromViewMode(currentView));

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
    f32 maxColWidth = params.width * dpi; 

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

    // Now that we have the exact dynamic width, we force the scrollbar to match it!
    ImGui::Dummy(ImVec2(totalVirtualWidth, rowsPerColumn * cellHeight));

    // -CUSTOM HORIZONTAL CLIPPER ---
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

    // -DRAW VISIBLE ITEMS ---
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
            if (HandleItemInteraction(ctx, i, item, isSelected, ImVec2(currentColWidth, cellHeight))){
                ImGui::PopID();
                ImGui::EndChild();
                return;
            }

            // B. Draw Icon (Vertically centered)
            f32 iconY = cellPos.y + (cellHeight - iconSize) * 0.5f;
            ImGui::SetCursorPos(ImVec2(cellPos.x + xGap, iconY));

            DrawItemIcon(ctx, item, iconSize, SHIL_SMALL);

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

void RenderViewDetails(AppContext& ctx, GridViewParams& params) {
    // Push these colors before the early returns so we don't leak state
    ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_TableBorderLight, IM_COL32(0, 0, 0, 0));

    // Begin a group to keep our tight Table Child and Empty Space Child on the same horizontal plane
    ImGui::BeginGroup();

    // 1. TIGHT CHILD WINDOW: We use AutoResizeX so the child wraps perfectly around the columns!
    ImGuiChildFlags tableChildFlags = ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_NavFlattened;
    if (ImGui::BeginChild("FileViewDetails_TableSpace", Style::AutoFillRemnantWindow, tableChildFlags, ImGuiWindowFlags_None)) {
        
        f32 dpi = ctx.ui.dpiScale;
        f32 cellHeight = params.height * dpi;
        f32 iconSize = cellHeight * ImageToContainerRatio; 
        
        f32 nameWidth = params.width * dpi;  
        f32 dateWidth = 150.0f * dpi;
        f32 typeWidth = 120.0f * dpi;
        f32 sizeWidth = 100.0f * dpi;

        ImGuiTableFlags flags = 
            ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | 
            ImGuiTableFlags_Sortable | ImGuiTableFlags_ScrollY | ImGuiTableFlags_PadOuterX | 
            ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoBordersInBody;

        if (ImGui::BeginTable("DetailsGrid", 4, flags)) {
            
            ImGui::TableSetupScrollFreeze(0, 1); 
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthFixed, nameWidth);
            ImGui::TableSetupColumn("Date modified", ImGuiTableColumnFlags_WidthFixed, dateWidth);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, typeWidth);
            ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, sizeWidth);
            ImGui::TableHeadersRow();

            auto &dir = ctx.navigation.Contents().Items();
            ImGuiListClipper clipper;
            clipper.Begin((int)dir.size(), cellHeight); 

            while (clipper.Step()){
                for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++){
                    auto& item = dir[row];
                    bool isFolder = item.attributes & SFGAO_FOLDER;
                    bool isSelected = (ctx.navigation.Contents().Selected() == row);

                    ImGui::PushID(row);
                    ImGui::TableNextRow(ImGuiTableRowFlags_None, cellHeight);
                    ImGui::TableNextColumn();

                    f32 availWidth = ImGui::GetContentRegionAvail().x;
                    ImVec2 startPos = ImGui::GetCursorPos();

                    // SpanAllColumns will now naturally STOP at the edge of the TableSpace child!
                    ImGuiSelectableFlags selectFlags = ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_AllowOverlap;

                    if (HandleItemInteraction(ctx, row, item, isSelected, ImVec2(0, cellHeight), selectFlags, false)) {
                        ImGui::PopID();
                        ImGui::EndTable();
                        ImGui::EndChild();
                        ImGui::EndGroup();
                        ImGui::PopStyleColor(2);
                        return;
                    }
                    bool isRowHovered = ImGui::IsItemHovered();

                    if (isRowHovered && ImGui::TableGetHoveredColumn() == 0){
                        ImGui::SetTooltip(item.TooltipInfo().c_str());
                    }

                    // Draw Icon 
                    f32 iconPaddingX = 4.0f * dpi; 
                    f32 iconY = startPos.y + (cellHeight - iconSize) * 0.5f;
                    ImGui::SetCursorPos(ImVec2(startPos.x + iconPaddingX, iconY));
                    
                    DrawItemIcon(ctx, item, iconSize, SHIL_SMALL);

                    // Draw Text
                    f32 textGapX = 6.0f * dpi;
                    f32 textX = startPos.x + iconPaddingX + iconSize + textGapX;
                    f32 textY = startPos.y + (cellHeight - ImGui::GetTextLineHeight()) * 0.5f;
                    ImGui::SetCursorPos(ImVec2(textX, textY));
                    
                    f32 maxTextWidth = availWidth - (textX - startPos.x);
                    UI::Helpers::DrawSingleLineTruncatedText(item.name.c_str(), maxTextWidth);

                    ImGui::TableNextColumn();
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (cellHeight - ImGui::GetTextLineHeight()) * 0.5f);
                    char dateBuf[64];
                    WShell::FileTime(item.lastWriteTime, dateBuf, sizeof(dateBuf));
                    UI::Helpers::DrawTableTextWithTooltip(dateBuf, isRowHovered);

                    ImGui::TableNextColumn();
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (cellHeight - ImGui::GetTextLineHeight()) * 0.5f);
                    UI::Helpers::DrawTableTextWithTooltip(item.TypeName().c_str(), isRowHovered);

                    ImGui::TableNextColumn();
                    if (!isFolder) {
                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (cellHeight - ImGui::GetTextLineHeight()) * 0.5f);
                        char sizeBuf[32];
                        WShell::Size(item.Size(), sizeBuf, sizeof(sizeBuf));
                        f32 colWidth = ImGui::GetContentRegionAvail().x;
                        f32 sizeTextWidth = ImGui::CalcTextSize(sizeBuf).x;
                        if (colWidth > sizeTextWidth) {
                            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (colWidth - sizeTextWidth) - (4.0f * dpi));
                        }
                        UI::Helpers::DrawTableTextWithTooltip(sizeBuf, isRowHovered);
                    }

                    ImGui::PopID();
                }
            }
            ImGui::EndTable();
        }
    }
    ImGui::EndChild();

    // To make the hover hug the last column
    ImGui::SameLine(0, 0);
    if (ImGui::BeginChild("FileViewDetails_EmptySpace", Style::AutoFillRemnantWindow, ImGuiChildFlags_None, ImGuiWindowFlags_None)) {
        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            ctx.navigation.Contents().SelectIndex(-1);
        }

    }
    ImGui::EndChild();
    ImGui::EndGroup();

    ImGui::PopStyleColor(2);
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
        case ViewMode::Details:{
            RenderViewDetails(ctx, params);
        }
        break;
    }

    
}