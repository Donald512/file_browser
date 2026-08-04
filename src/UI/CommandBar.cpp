#include "UI.h"
#include "ShellAsync.h"

namespace Colors = UI::Colors;
namespace Style  = UI::Style;

// Forward declarations
static void DrawNewMenuDropdown(AppContext& ctx);
static void DrawViewMenuDropdown(AppContext& ctx);
static void DrawSortMenuDropdown(AppContext& ctx);
static void PositionPopupBelowWindow(const char* popupId, float dpiScale, float offsetPxY);
static bool CustomMenuItem(const char* label, bool selected, bool isRadioStyle = true);

// ============================================================================
// Internal Helpers
// ============================================================================

// Helper to render inline command bar action buttons with standardized spacing
static bool ToolbarButton(const char* label, float spacing = 8.0f) {
    bool clicked = ImGui::Button(label);
    ImGui::SameLine(0.0f, spacing);
    return clicked;
}

static void PositionPopupBelowWindow(const char* popupId, float dpiScale, float offsetPxY) {
    if (ImGui::IsPopupOpen(popupId)) {
        ImVec2 parentPos  = ImGui::GetWindowPos();
        ImVec2 parentSize = ImGui::GetWindowSize();
        ImVec2 buttonMin  = ImGui::GetItemRectMin();

        ImVec2 popupPos = ImVec2(buttonMin.x, (offsetPxY * dpiScale) + parentPos.y + parentSize.y);
        ImGui::SetNextWindowPos(popupPos);
    }
}

// ============================================================================
// Main Command Bar Render Pass
// ============================================================================

void CommandBar::Render(AppContext& ctx) {
    f32 dpi = ctx.ui.dpiScale;
    const f32 scaledHeight = Height * dpi;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2.0f * dpi, 0));

    if (!ImGui::BeginChild("CommandBar", ImVec2(0, scaledHeight), ImGuiChildFlags_None, TopBar::Flags)) {
        ImGui::EndChild();
        ImGui::PopStyleVar();
        return;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f * dpi, 8.0f * dpi));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, Style::NoBorder);

    UI::Helpers::AlignCursorVertically(scaledHeight);

    // --- New Menu Button & Dropdown ---
    constexpr const char* newMenuPopupId = "NewMenuPopup";
    ImGui::BeginDisabled(ctx.navigation.Contents().Access() == WShell::FolderAccess::NoCreate);

    if (ImGui::Button(ICON_REG_ADD_CIRCLE " New " ICON_REG_CHEVRON_DOWN)) {
        ImGui::OpenPopup(newMenuPopupId);
    }
    ImGui::EndDisabled();

    PositionPopupBelowWindow(newMenuPopupId, dpi, 5.0f);
    if (ImGui::BeginPopup(newMenuPopupId)) {
        DrawNewMenuDropdown(ctx);
        ImGui::EndPopup();
    }
    ImGui::SameLine(0.0f, 8.0f);

    // --- Action Buttons ---
    ToolbarButton(ICON_REG_CUT);
    ToolbarButton(ICON_REG_COPY);
    ToolbarButton(ICON_REG_CLIPBOARD_PASTE);
    ToolbarButton(ICON_REG_RENAME);
    ToolbarButton(ICON_REG_SHARE);
    ToolbarButton(ICON_REG_BIN_RECYCLE);

    // --- Sort Menu Dropdown ---
    constexpr const char* sortMenuPopupId = "SortMenuPopup";
    if (ImGui::Button(ICON_REG_ARROW_SORT " Sort " ICON_REG_CHEVRON_DOWN)) {
        ImGui::OpenPopup(sortMenuPopupId);
    }

    PositionPopupBelowWindow(sortMenuPopupId, dpi, 5.0f);
    if (ImGui::BeginPopup(sortMenuPopupId)) {
        DrawSortMenuDropdown(ctx);
        ImGui::EndPopup();
    }
    ImGui::SameLine(0.0f, 8.0f);

    // --- View Menu Dropdown ---
    constexpr const char* viewMenuPopupId = "ViewMenuPopup";
    if (ImGui::Button(ICON_REG_LIST " View " ICON_REG_CHEVRON_DOWN)) {
        ImGui::OpenPopup(viewMenuPopupId);
    }
    PositionPopupBelowWindow(viewMenuPopupId, dpi, 5.0f);
    if (ImGui::BeginPopup(viewMenuPopupId)) {
        DrawViewMenuDropdown(ctx);
        ImGui::EndPopup();
    }
    
    ImGui::PopStyleVar(2);
    ImGui::PopStyleVar(1);
    ImGui::EndChild();
}

// ============================================================================
// Sub-Menu Dropdown Implementations
// ============================================================================

static void DrawNewMenuDropdown(AppContext& ctx) {
    f32 dpi = ctx.ui.dpiScale;

    static f32 maxTextWidth = GetLongestStringWidth(ctx.newMenuItems);
    for (auto& item : ctx.newMenuItems) {
        int i = 0;
        ImGui::PushID(&item);

        ImTextureID iconTex = 0;
        if (item.iconKey.resolved) {
            iconTex = ctx.icons.GetTexture({item.iconKey.value, SHIL_LARGE});
        } else if (!item.iconRequestSent) {
            WShell::Async::RequestNewMenuIcon(ctx, item);
        }

        MenuRowStyle rowStyle;
        rowStyle.outerMargin = 2.0f;
        rowStyle.innerPad.x    = 8.0f;
        rowStyle.rounding    = 4.0f;
        rowStyle.itemSpacing = ImVec2(0.0f, 2.0f);
        f32 iconSize = 16.0f * dpi;
        f32 spaceBetweenIconAndText = 8.0f;


        // No comments, no logic, pure eyeballing and correcting. dont try to reason it 
        if (UI::Helpers::MenuRow(item.displayName.c_str(), item.displayName.c_str(), iconTex, dpi, rowStyle, 0, spaceBetweenIconAndText, maxTextWidth + rowStyle.outerMargin * 2 + rowStyle.innerPad.x * 2 + rowStyle.itemSpacing.x + spaceBetweenIconAndText * 2 + iconSize, false)){

        }
        ImGui::PopID();
        i++;
    }

}

static bool CustomMenuItem(const char* label, bool selected, bool isRadioStyle) {
    ImGui::PushID(label);
    
    ImVec2 cursorStart = ImGui::GetCursorPos();
    bool clicked = ImGui::Selectable("##selectable", selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap);

    ImGui::SetCursorPos(cursorStart);

    if (selected) {
        const char* checkMark = isRadioStyle ? ICON_REG_RADIO_BUTTON : ICON_REG_CHECKMARK;
        ImGui::TextUnformatted(checkMark);
    } else {
        ImGui::Dummy(ImVec2(ImGui::GetFontSize(), 0.0f));
    }

    ImGui::SameLine();
    ImGui::TextUnformatted(label);

    ImGui::PopID();
    return clicked;
}

struct SortMode{
    std::string displayName;
    WShell::SortMode sortMode;
};

struct SortDir{
    std::string displayName;
    WShell::SortDirection sortDir;
};

std::vector<const char*> sortModesStr = {"Name", "Date Modified", "Type", "Size"  "Ascending", "Descending", "Hidden Items"};





static void DrawSortMenuDropdown(AppContext& ctx) {
    f32 dpi = ctx.ui.dpiScale;
    auto& contents = ctx.navigation.Contents();

    SortMode sortModes[] = {
        {"Name",            WShell::SortMode::Name },
        {"Date Modified",   WShell::SortMode::DateModified},
        {"Type",            WShell::SortMode::Type},
        {"Size",            WShell::SortMode::Size}
    };
    SortDir sortDirs[] = {
        {"Ascending",    WShell::SortDirection::Ascending },
        {"Descending",   WShell::SortDirection::Descending}
    };

    auto GetMaxWidth = [&](std::vector<const char*> strings){
        f32 maxWidth = 0.0f;

        for (const auto& displayName : strings) {
            f32 textWidth = ImGui::CalcTextSize(displayName).x;
            if (textWidth > maxWidth){
                maxWidth = textWidth;
            }
        }
        return maxWidth;
    };

    auto maxTextWidth = GetMaxWidth(sortModesStr);
    
    MenuRowStyle rowStyle;
    rowStyle.outerMargin = 4.0f;
    rowStyle.innerPad.x    = 16.0f;
    rowStyle.innerPad.y    = 4.0f;
    rowStyle.rounding    = 6.0f;
    rowStyle.itemSpacing = ImVec2(0.0f, 6.0f);
    
    f32 rowWidth = maxTextWidth + (rowStyle.innerPad.x * 2.0f + rowStyle.outerMargin * 2.0f) * dpi;
    for (auto& sortMode : sortModes) {
        int i = 0;
        ImGui::PushID(i);

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {0.0f, 0.0f});  // was messing up my buttons, but i didnt want to remove the FramePadding in the Main Command::Render because it gives the cut, New, and even the sort label the padding, dont know why its transferring to the ones in this function now

        if (UI::Helpers::MenuRow(sortMode.displayName.c_str(), sortMode.displayName.c_str(), dpi, ctx.navigation.Contents().GetSort() == sortMode.sortMode, rowStyle, rowWidth)){
            contents.SetSort(sortMode.sortMode, contents.GetSortDir());
        }
        ImGui::PopStyleVar();
        ImGui::PopID();

        i++;
    }
    
    ImGui::Separator();
    ImGui::Dummy({0, 4*dpi});

    
    for (auto& sortDir : sortDirs) {
        int i = 0;
        ImGui::PushID(i);

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {0.0f, 0.0f});  

        if (UI::Helpers::MenuRow(sortDir.displayName.c_str(), sortDir.displayName.c_str(), dpi, ctx.navigation.Contents().GetSortDir() == sortDir.sortDir, rowStyle, rowWidth)){
            contents.SetSort(contents.GetSort(), sortDir.sortDir);
        }
        ImGui::PopStyleVar();
        ImGui::PopID();

        i++;
    }

    ImGui::Separator();

    ImGui::Dummy({0, 4*dpi});

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {0.0f, 0.0f});  
    bool hidden =  FileView::ShowHidden == true;
    if (UI::Helpers::MenuRow("hidden items", "Hidden Items", dpi, hidden, rowStyle, rowWidth)){
        hidden != hidden;
    }
    ImGui::PopStyleVar();

}

static void DrawViewMenuDropdown(AppContext& ctx) {
    f32 dpi = ctx.ui.dpiScale;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f * dpi, 8.0f * dpi));

    // View Modes
    struct ViewOption { const char* label; FileView::ViewMode mode; };
    const ViewOption options[] = {
        { ICON_REG_DESKTOP_28 " Extra large icons", FileView::ViewMode::ExtraLarge },
        { ICON_REG_DESKTOP_20 " Large icons",       FileView::ViewMode::Large },
        { ICON_REG_DESKTOP_MAC " Medium icons",     FileView::ViewMode::Medium },
        { ICON_REG_GRID " Small icons",             FileView::ViewMode::Small },
        { ICON_REG_LIST " List",                   FileView::ViewMode::List },
        { ICON_REG_DOCUMENT_BULLET_LIST " Details", FileView::ViewMode::Details },
        { ICON_REG_APPS_LIST_DETAIL " Tiles",       FileView::ViewMode::Tiles }
    };

    for (const auto& opt : options) {
        if (CustomMenuItem(opt.label, FileView::currentView == opt.mode)) {
            FileView::currentView = opt.mode;
        }
    }

    ImGui::Separator();

    CustomMenuItem(ICON_REG_PANEL_LEFT " Details pane", false);
    CustomMenuItem(ICON_REG_PANEL_RIGHT " Preview pane", false);

    ImGui::Separator();

    if (ImGui::BeginMenu("Show")) {
        CustomMenuItem(ICON_REG_PANEL_LEFT " Navigation pane", true, false);
        ImGui::Separator();
        CustomMenuItem(ICON_REG_ARROW_BIDIRECTIONAL_UP_DOWN "Compact view", false, false);
        ImGui::Separator();
        CustomMenuItem(ICON_REG_CHECKMARK_SQUARE "Item check boxes", false, false);
        CustomMenuItem(ICON_REG_DOCUMENT_ARROW_UP "File name extensions", false, false);
        CustomMenuItem(ICON_REG_EYE "Hidden items", false, false);

        ImGui::EndMenu();
    }

    ImGui::PopStyleVar();
}