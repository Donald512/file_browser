#include "UI.h"
#include <algorithm>
#include <cmath>
#include "ShellAsync.h"

using namespace UI;
using namespace FileView;

struct GridViewParams { f32 width, height; };


static GridViewParams GetGridParamsForMode(ViewMode m) {
    if (m == ViewMode::ExtraLarge) return {271.f, 260.f};
    if (m == ViewMode::Medium) return {74.f, 52.f};
    if (m == ViewMode::Small || m == ViewMode::List || m == ViewMode::Details) return {308.f, 30.f};
    if (m == ViewMode::Tiles) return {250.f, 52.f};
    return {105.f, 100.f};
}

static int ShilSizeFromViewMode(ViewMode m) {
    return (m == ViewMode::ExtraLarge || m == ViewMode::Large) ? SHIL_JUMBO :
           (m == ViewMode::Small || m == ViewMode::List || m == ViewMode::Details) ? SHIL_SMALL : SHIL_EXTRALARGE;
}

static bool HandleInteraction(AppContext& ctx, int i, const WShell::Item& item, f32 w, f32 h, bool& openMenu, ImGuiSelectableFlags flags = ImGuiSelectableFlags_AllowDoubleClick, bool hover = true) {
    bool sel = ctx.navigation.Contents().Selected() == i;
    if (ImGui::Selectable("##file_selectedbox", sel, flags, {w, h})) ctx.navigation.Contents().SelectIndex(i);
    if (hover && UI::Helpers::IsItemHoveredWithDelay(HOVER_DELAY_NORMAL)){
        if (item.tooltipInfo.resolved) ImGui::SetTooltip("%s", item.tooltipInfo.value.c_str());
        else if (!item.tooltipRequestSent) WShell::Async::RequestTooltip(ctx, item, i);
    }
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) ctx.navigation.Contents().SelectIndex(i);
    
    if ((ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && ImGui::IsItemHovered()) || (sel && ImGui::IsKeyPressed(ImGuiKey_Enter))) {
        if (item.attributes & SFGAO_FOLDER) { ctx.navigation.NavigateTo(item.pidl); return true; }
        WShell::ExecuteFile(item.pidl);
    }
    
    if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right)){
        // Select the item
        ctx.navigation.Contents().SelectIndex(i);
        ctx.UpdateContextMenu(item.pidl);
        openMenu = true;
    }
    return false;
}

static void DrawIcon(AppContext& ctx, const WShell::Item& item, int i, f32 sz, int shil) {
    bool isCut = ctx.isFileCutOnClipBoard(item.hash);
    if (isCut){
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
    }
    if (ImTextureID tex = item.iconKey.resolved ? ctx.icons.GetTexture({item.IconKey(), shil}) : 0) {
        ImGui::Image(tex, {sz, sz});
    }
    else {
        if (!item.iconRequestSent) WShell::Async::RequestIcon(ctx, item, i);
        ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddRectFilled(p, {p.x + sz, p.y + sz}, item.attributes & SFGAO_FOLDER ? 0xFF33A5CC : 0xFFB27F4C, 4.f);
    }
    if (isCut){
        ImGui::PopStyleVar();
    }

}

template<typename F>
static void RenderGridBase(AppContext& ctx, f32 w, f32 h, F renderCell) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, WindowPadding);
    bool open = ImGui::BeginChild("FileView", Style::AutoFillRemnantWindow, ImGuiChildFlags_Borders, ImGuiChildFlags_NavFlattened);
    ImGui::PopStyleVar();
    if (!open) { ImGui::EndChild(); return; }

    f32 dpi = ctx.ui.dpiScale, xGap = XGap * dpi;
    int cols = (std::max)(1, (int)(ImGui::GetContentRegionAvail().x / ((w + XGap) * dpi)));
    
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, {xGap * 0.5f, xGap * 0.5f});
    if (ImGui::BeginTable("ExplorerGrid", cols, ImGuiTableFlags_NoSavedSettings)) {
        auto& dir = ctx.navigation.Contents().Items();
        ImGuiListClipper clipper; clipper.Begin(((int)dir.size() + cols - 1) / cols, h);
        while (clipper.Step()) {
            for (int r = clipper.DisplayStart; r < clipper.DisplayEnd; r++) {
                for (int i = r * cols, end = (std::min)(i + cols, (int)dir.size()); i < end; i++) {
                    ImGui::TableNextColumn();
                    if (renderCell(i, dir[i], ImGui::GetContentRegionAvail().x)) {
                        ImGui::EndTable(); 
                        ImGui::PopStyleVar(); 
                        ImGui::EndChild(); 
                        return;
                    }
                }
            }
        }
        ImGui::EndTable();
    }
    ImGui::PopStyleVar(); ImGui::EndChild();
}

static void RenderGrid(AppContext& ctx, GridViewParams p, bool& openMenu) {
    f32 dpi = ctx.ui.dpiScale, imgH = p.height * dpi, iconSz = imgH * ImageToContainerRatio, rowH = imgH + 4 * ImGui::GetTextLineHeight() + 8.f * dpi;
    RenderGridBase(ctx, p.width, rowH, [&](int i, const auto& item, f32 cellW) {
        f32 maxTw = (std::min)(TextToContainerWidthRatio * p.width * dpi, cellW - 4.f * dpi);
        int lines = std::clamp((int)std::ceil(ImGui::CalcTextSize(item.name.c_str(), 0, false, maxTw).y / ImGui::GetTextLineHeight()), 1, 4);
        f32 selH = imgH + lines * ImGui::GetTextLineHeight() + 4.f * dpi;
        
        ImGui::PushID(i); ImVec2 pos = ImGui::GetCursorPos();
        bool nav = HandleInteraction(ctx, i, item, cellW, selH, openMenu);
        if (!nav) {
            ImGui::SetCursorPos({pos.x + (cellW - iconSz) * 0.5f, pos.y + (imgH - iconSz) * 0.5f});
            DrawIcon(ctx, item, i, iconSz, ShilSizeFromViewMode(currentView));
            ImGui::SetCursorPos({pos.x, pos.y + imgH});
            Helpers::DrawCenteredWrappedText(item.name.c_str(), cellW, maxTw, 4);
            ImGui::SetCursorPos(pos); ImGui::Dummy({cellW, rowH});
        }
        ImGui::PopID(); return nav;
    });
}

static void RenderViewSmall(AppContext& ctx, GridViewParams p, bool& openMenu) {
    f32 dpi = ctx.ui.dpiScale, cellH = p.height * dpi, iconSz = 16.f * dpi;
    RenderGridBase(ctx, p.width, cellH, [&](int i, const auto& item, f32 cellW) {
        ImGui::PushID(i); ImVec2 pos = ImGui::GetCursorPos();
        bool nav = HandleInteraction(ctx, i, item, cellW, cellH, openMenu);
        if (!nav) {
            ImGui::SetCursorPos({pos.x + 6.f * dpi, pos.y + (cellH - iconSz) * 0.5f});
            DrawIcon(ctx, item, i, iconSz, SHIL_SMALL);
            ImGui::SetCursorPos({pos.x + 14.f * dpi + iconSz, pos.y + (cellH - ImGui::GetTextLineHeight()) * 0.5f});
            Helpers::DrawSingleLineTruncatedText(item.name.c_str(), cellW - (14.f * dpi + iconSz));
            ImGui::SetCursorPos(pos); ImGui::Dummy({cellW, cellH});
        }
        ImGui::PopID(); return nav;
    });
}

static void RenderViewTiles(AppContext& ctx, GridViewParams p, bool& openMenu) {
    f32 dpi = ctx.ui.dpiScale, cellH = p.height * dpi, iconSz = cellH * ImageToContainerRatio;
    RenderGridBase(ctx, p.width, cellH, [&](int i, const auto& item, f32 cellW) {
        ImGui::PushID(i); ImVec2 pos = ImGui::GetCursorPos(), sPos = ImGui::GetCursorScreenPos();
        bool nav = HandleInteraction(ctx, i, item, cellW, cellH, openMenu);
        if (!nav) {
            ImGui::SetCursorPos({pos.x + 6.f * dpi, pos.y + (cellH - iconSz) * 0.5f});
            DrawIcon(ctx, item, i, iconSz, ShilSizeFromViewMode(currentView));
            ImGui::SetCursorPos({pos.x + 14.f * dpi + iconSz, pos.y + (cellH - iconSz) * 0.5f});
            
            if (!item.tileViewInfo.resolved && !item.tileInfoRequestSent) WShell::Async::RequestTileInfo(ctx, item, i);
            std::string txt = item.name + '\n' + (item.tileViewInfo.resolved ? item.tileViewInfo.value : "");
            
            ImGui::PushClipRect(sPos, {sPos.x + cellW, sPos.y + cellH}, true);
            ImGui::PushTextWrapPos(pos.x + cellW - 8.f * dpi);
            ImGui::TextUnformatted(txt.c_str());
            ImGui::PopTextWrapPos(); 
            ImGui::PopClipRect();
            
            ImGui::SetCursorPos(pos); 
            ImGui::Dummy({cellW, cellH});
        }
        ImGui::PopID(); 
        return nav;
    });
}

static void RenderViewList(AppContext& ctx, GridViewParams p, bool& openMenu) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, WindowPadding);
    bool open = ImGui::BeginChild("List", Style::AutoFillRemnantWindow, ImGuiChildFlags_Borders | ImGuiChildFlags_NavFlattened, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::PopStyleVar();
    if (!open) { 
        ImGui::EndChild(); 
        return; 
    }

    f32 dpi = ctx.ui.dpiScale, cellH = p.height * dpi, iconSz = 16.f * dpi, xGap = XGap * dpi;
    if (ImGui::IsWindowHovered() && ImGui::GetIO().MouseWheel != 0.f) {
        ImGui::SetScrollX(ImGui::GetScrollX() - ImGui::GetIO().MouseWheel * 60.f * dpi);
    }

    auto& dir = ctx.navigation.Contents().Items();
    if (dir.empty()) { 
        ImGui::EndChild(); 
        return; 
    }

    int rows = (std::max)(1, (int)(ImGui::GetContentRegionAvail().y / cellH));
    int cols = ((int)dir.size() + rows - 1) / rows;
    std::vector<f32> w(cols, 120.f * dpi), x(cols + 1, 0.f);
    
    for (int i = 0; i < (int)dir.size(); i++) {
        w[i / rows] = (std::max)(w[i / rows], (std::min)(ImGui::CalcTextSize(dir[i].name.c_str()).x + iconSz + 3.f * xGap, p.width * dpi));
    }
    for (int c = 0; c < cols; c++) x[c + 1] = x[c] + w[c];

    ImGui::Dummy({x[cols], (f32)(rows * cellH)});
    f32 sx = ImGui::GetScrollX(), wx = ImGui::GetWindowWidth();
    int c0 = 0; while (c0 < cols && x[c0 + 1] < sx) c0++;
    int c1 = c0; while (c1 < cols && x[c1] < sx + wx) c1++;

    for (int c = c0; c < c1; c++) {
        for (int r = 0; r < rows; r++) {
            int i = c * rows + r; if (i >= (int)dir.size()) break;
            auto& item = dir[i]; ImGui::PushID(i);
            ImVec2 pos(x[c], (f32)(r * cellH)); ImGui::SetCursorPos(pos);
            if (HandleInteraction(ctx, i, item, w[c], cellH, openMenu)) { ImGui::PopID();   ImGui::EndChild(); 
                return;
            }
            ImGui::SetCursorPos({pos.x + xGap, pos.y + (cellH - iconSz) * 0.5f});
            DrawIcon(ctx, item, i, iconSz, SHIL_SMALL);
            ImGui::SetCursorPos({pos.x + 2.f * xGap + iconSz, pos.y + (cellH - ImGui::GetTextLineHeight()) * 0.5f});
            Helpers::DrawSingleLineTruncatedText(item.name.c_str(), w[c] - 3.f * xGap - iconSz);
            ImGui::PopID();
        }
    }
    ImGui::EndChild();
}

static void RenderViewDetails(AppContext& ctx, GridViewParams p, bool& openMenu) {
    ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, 0); ImGui::PushStyleColor(ImGuiCol_TableBorderLight, 0);
    ImGui::BeginGroup();
    if (ImGui::BeginChild("DetTbl", Style::AutoFillRemnantWindow,  ImGuiChildFlags_NavFlattened)) {
        f32 dpi = ctx.ui.dpiScale, cellH = p.height * dpi, iconSz = 16.f * dpi;
        if (ImGui::BeginTable("DetGrid", 4, ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable | ImGuiTableFlags_Sortable | ImGuiTableFlags_ScrollY | ImGuiTableFlags_PadOuterX | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoBordersInBodyUntilResize | ImGuiTableFlags_NoHostExtendX)) {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthFixed, p.width * dpi);
            ImGui::TableSetupColumn("Date modified", ImGuiTableColumnFlags_WidthFixed, 150.f * dpi);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 120.f * dpi);
            ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 100.f * dpi);
            ImGui::TableHeadersRow();

            auto& dir = ctx.navigation.Contents().Items();
            ImGuiListClipper clipper; clipper.Begin((int)dir.size(), cellH); 
            while (clipper.Step()) {
                for (int r = clipper.DisplayStart; r < clipper.DisplayEnd; r++) {
                    auto& item = dir[r]; ImGui::PushID(r);
                    ImGui::TableNextRow(0, cellH); ImGui::TableNextColumn();
                    ImVec2 pos = ImGui::GetCursorPos();
                    if (HandleInteraction(ctx, r, item, 0, cellH, openMenu,  ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick | ImGuiSelectableFlags_AllowOverlap, false)) {
                        ImGui::PopID(); ImGui::EndTable(); ImGui::EndChild(); ImGui::EndGroup(); ImGui::PopStyleColor(2); return;
                    }
                    
                    bool hov = ImGui::IsItemHovered();
                    if (hov && ImGui::TableGetHoveredColumn() == 0) {
                        if (item.tooltipInfo.resolved) ImGui::SetTooltip("%s", item.tooltipInfo.value.c_str());
                        else if (!item.tooltipRequestSent) WShell::Async::RequestTooltip(ctx, item, r);
                    }
                    
                    ImGui::SetCursorPos({pos.x + 4.f * dpi, pos.y + (cellH - iconSz) * 0.5f});
                    DrawIcon(ctx, item, r, iconSz, SHIL_SMALL);
                    ImGui::SetCursorPos({pos.x + 10.f * dpi + iconSz, pos.y + (cellH - ImGui::GetTextLineHeight()) * 0.5f});
                    Helpers::DrawSingleLineTruncatedText(item.name.c_str(), ImGui::GetContentRegionAvail().x - (10.f * dpi + iconSz));
                    
                    f32 textY = pos.y + (cellH - ImGui::GetTextLineHeight()) * 0.5f;
                    ImGui::TableNextColumn(); ImGui::SetCursorPosY(textY);
                    char buf[64]; WShell::FileTime(item.lastWriteTime, buf, sizeof(buf));
                    Helpers::DrawTableTextWithTooltip(buf, hov);

                    if (!item.typeName.resolved && !item.metaRequestSent) WShell::Async::RequestMeta(ctx, item, r);
                    ImGui::TableNextColumn(); ImGui::SetCursorPosY(textY);
                    Helpers::DrawTableTextWithTooltip(item.typeName.resolved ? item.typeName.value.c_str() : "", hov);

                    ImGui::TableNextColumn(); 
                    if (!(item.attributes & SFGAO_FOLDER)) {
                        ImGui::SetCursorPosY(textY);
                        char sz[32] = {}; if (item.size.resolved) WShell::Size(item.size.value, sz, sizeof(sz));
                        f32 cw = ImGui::GetContentRegionAvail().x, tw = ImGui::CalcTextSize(sz).x;
                        if (cw > tw) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + cw - tw - 4.f * dpi);
                        Helpers::DrawTableTextWithTooltip(sz, hov);
                    }
                    ImGui::PopID();
                }
            }
            ImGui::EndTable();
        }
    }

    ImGui::EndChild(); 
    ImGui::EndGroup(); 
    ImGui::PopStyleColor(2);
}

void FileView::Render(AppContext& ctx) {
    bool openMenu = false;
    auto p = GetGridParamsForMode(currentView);
    if (currentView == ViewMode::Small) RenderViewSmall(ctx, p, openMenu);
    else if (currentView == ViewMode::List) RenderViewList(ctx, p, openMenu);
    else if (currentView == ViewMode::Details) RenderViewDetails(ctx, p, openMenu);
    else if (currentView == ViewMode::Tiles) RenderViewTiles(ctx, p, openMenu);
    else RenderGrid(ctx, p, openMenu);

    
    if (openMenu){
        ImGui::OpenPopup("ItemContextMenu");
    }

    if (ImGui::BeginPopup("ItemContextMenu")) {
        RenderContextMenuStructure(ctx, ctx.activeContextMenuData);
        ImGui::EndPopup();
    }

}
