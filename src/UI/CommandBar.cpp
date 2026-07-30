
#include "UI.h"

namespace Colors = UI::Colors;
namespace Style = UI::Style;

static void DrawNewMenuDropdown(AppContext& ctx);
static void PositionPopupBelowWindow(const char* popupId, float dpiScale, float offsetPxY);
static void DrawViewMenuDropdown(AppContext& ctx);
static void DrawSortMenuDropdown(AppContext& ctx);

void CommandBar::Render(AppContext& ctx){
    if (!ImGui::BeginChild("CommandBar", ImVec2(0, Height * ctx.ui.dpiScale), ImGuiChildFlags_None, TopBar::Flags) ){
        ImGui::EndChild();
        return;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f * ctx.ui.dpiScale, 8.0f * ctx.ui.dpiScale));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, Style::NoBorder);

    UI::Helpers::AlignCursorVertically(Height * ctx.ui.dpiScale);
    
    // =========================== New Button & Popup
    char* newMenuPopupId = "NewMenuPopup";       
    ImGui::BeginDisabled(ctx.navigation.Contents().Access() == WShell::FolderAccess::NoCreate);
    const char* newLabel = ICON_REG_ADD_CIRCLE " New " ICON_REG_CHEVRON_DOWN;

    if (ImGui::Button(newLabel)){
        ImGui::OpenPopup(newMenuPopupId);
    }
    ImGui::EndDisabled();
    PositionPopupBelowWindow(newMenuPopupId, ctx.ui.dpiScale, 5);
    if (ImGui::BeginPopup(newMenuPopupId)){
        DrawNewMenuDropdown(ctx);
        ImGui::EndPopup();
    }

    // ============================
    ImGui::SameLine(0.0f, 8.0f);
    
     UI::Helpers::AlignCursorVertically(Height * ctx.ui.dpiScale);

    ImGui::Button(ICON_REG_CUT);
    ImGui::SameLine(0.0f, 8.0f);

     UI::Helpers::AlignCursorVertically(Height * ctx.ui.dpiScale);

    ImGui::Button(ICON_REG_COPY);
    ImGui::SameLine(0.0f, 8.0f);

     UI::Helpers::AlignCursorVertically(Height * ctx.ui.dpiScale);

    ImGui::Button(ICON_REG_CLIPBOARD_PASTE);
    ImGui::SameLine(0.0f, 8.0f);

     UI::Helpers::AlignCursorVertically(Height * ctx.ui.dpiScale);

    ImGui::Button(ICON_REG_RENAME);
    ImGui::SameLine(0.0f, 8.0f);

     UI::Helpers::AlignCursorVertically(Height * ctx.ui.dpiScale);

    ImGui::Button(ICON_REG_SHARE);
    ImGui::SameLine(0.0f, 8.0f);

     UI::Helpers::AlignCursorVertically(Height * ctx.ui.dpiScale);

    ImGui::Button(ICON_REG_BIN_RECYCLE);
    ImGui::SameLine(0.0f, 8.0f);

     UI::Helpers::AlignCursorVertically(Height * ctx.ui.dpiScale);



    // =========================== SORT & POPUP
    char* sortMenuPopupId = "SortMenuPopup";       

    if (ImGui::Button( ICON_REG_ARROW_SORT " Sort " ICON_REG_CHEVRON_DOWN)){
        ImGui::OpenPopup(sortMenuPopupId);
    }

    if (ImGui::BeginPopup(sortMenuPopupId)){
        DrawSortMenuDropdown(ctx);
        ImGui::EndPopup();
    }
    // ============================
    ImGui::SameLine(0.0f, 8.0f);

    UI::Helpers::AlignCursorVertically(Height * ctx.ui.dpiScale);

    // =========================== View
    char* viewMenuPopupId = "ViewMenuPopup";       
    if (ImGui::Button( ICON_REG_LIST " View " ICON_REG_CHEVRON_DOWN)){
        ImGui::OpenPopup(viewMenuPopupId);
    }
    PositionPopupBelowWindow(viewMenuPopupId, ctx.ui.dpiScale, 5);
    if (ImGui::BeginPopup(viewMenuPopupId)){
        DrawViewMenuDropdown(ctx);
        ImGui::EndPopup();
    }
    // ============================



    ImGui::PopStyleVar(2);
    ImGui::EndChild();
}

static void PositionPopupBelowWindow(const char* popupId, float dpiScale, float offsetPxY){
    if (ImGui::IsPopupOpen(popupId)){

        // Set NextWindowPos here to move the popup 2px directly below, aligned with View button?
        ImVec2 buttonMin = ImGui::GetItemRectMin(); // Top left of button
        ImVec2 buttonSize = ImGui::GetItemRectSize();
        
        ImVec2 parentPos = ImGui::GetWindowPos();   // Top-left of current window
        ImVec2 parentSize = ImGui::GetWindowSize();
        
        ImVec2 popupPos = ImVec2(buttonMin.x, (offsetPxY * dpiScale) + parentPos.y + parentSize.y);
        ImGui::SetNextWindowPos(popupPos);
    }
}

static void DrawNewMenuDropdown(AppContext& ctx){
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f * ctx.ui.dpiScale, 8.0f * ctx.ui.dpiScale));
    for (auto& item : ctx.newMenuItems){
        ImGui::PushID(&item);
        ImTextureID iconTex = ctx.icons.GetTexture({item.IconKey(), SHIL_LARGE});
        if (iconTex){
            ImGui::Image(iconTex, ImVec2(16.0f, 16.0f));
            ImGui::SameLine();
        }
        if (ImGui::Selectable(item.displayName.c_str())){
            
            ImGui::CloseCurrentPopup();
            ImGui::PopID();
            break;
        }
        ImGui::PopID();

    }
    ImGui::PopStyleVar();
}
static bool CustomMenuItem(const char* label, bool selected, bool isRadioStyle = true) {
    ImGui::PushID(label); // Prevents ID collisions between items
    
    ImVec2 cursorStart = ImGui::GetCursorPos();
    
    // Using AllowOverlap lets us draw text/icons on top of the selectable
    bool clicked = ImGui::Selectable("##selectable", selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap);

    ImGui::SetCursorPos(cursorStart);

    if (selected) {
        const char* checkMark = isRadioStyle ? ICON_REG_RADIO_BUTTON : ICON_REG_CHECKMARK;
        ImGui::TextUnformatted(checkMark);
    } else {
        // Space holder for alignment
        ImGui::Dummy(ImVec2(ImGui::GetFontSize(), 0.0f));
    }

    ImGui::SameLine();
    ImGui::TextUnformatted(label);

    ImGui::PopID();
    return clicked;
}




static void DrawSortMenuDropdown(AppContext& ctx){
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f * ctx.ui.dpiScale, 8.0f * ctx.ui.dpiScale));
    if (ImGui::MenuItem(" Name ", nullptr, ctx.navigation.Contents().GetSort() == WShell::SortMode::Name)){
        ctx.navigation.Contents().SetSort(WShell::SortMode::Name, ctx.navigation.Contents().GetSortDir());
    }
    if (ImGui::MenuItem(" Date Modified ", nullptr, ctx.navigation.Contents().GetSort() == WShell::SortMode::DateModified)){
        ctx.navigation.Contents().SetSort(WShell::SortMode::DateModified, ctx.navigation.Contents().GetSortDir());
    }
    if (ImGui::MenuItem(" Type ", nullptr, ctx.navigation.Contents().GetSort() == WShell::SortMode::Type)){
        ctx.navigation.Contents().SetSort(WShell::SortMode::Type, ctx.navigation.Contents().GetSortDir());
    }
    if (ImGui::BeginMenu(" More ")){
        if (ImGui::MenuItem(" Size ", nullptr, ctx.navigation.Contents().GetSort() == WShell::SortMode::Size)){ 
            ctx.navigation.Contents().SetSort(WShell::SortMode::Size, ctx.navigation.Contents().GetSortDir());
        }
        ImGui::EndMenu();
    }

    ImGui::Separator();
        
    if (ImGui::MenuItem(" Ascending ", nullptr, ctx.navigation.Contents().GetSortDir() == WShell::SortDirection::Ascending)){
        ctx.navigation.Contents().SetSort(ctx.navigation.Contents().GetSort(), WShell::SortDirection::Ascending);
    }

    if (ImGui::MenuItem(" Descending ", nullptr, ctx.navigation.Contents().GetSortDir() == WShell::SortDirection::Descending)){
        ctx.navigation.Contents().SetSort(ctx.navigation.Contents().GetSort(), WShell::SortDirection::Descending);
    }
    
    ImGui::Separator();
    
    // The "Show >" sub-menu
    if (ImGui::BeginMenu("Group by")) {
        ImGui::EndMenu();
    }

    ImGui::PopStyleVar();
    
}

static void DrawViewMenuDropdown(AppContext& ctx){
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f * ctx.ui.dpiScale, 8.0f * ctx.ui.dpiScale));
    if (CustomMenuItem(ICON_REG_DESKTOP_28 " Extra large icons", FileView::currentView == FileView::ViewMode::ExtraLarge)) FileView::currentView = FileView::ViewMode::ExtraLarge;
    if (CustomMenuItem(ICON_REG_DESKTOP_20 " Large icons", FileView::currentView == FileView::ViewMode::Large)) FileView::currentView = FileView::ViewMode::Large;
    if (CustomMenuItem(ICON_REG_DESKTOP_MAC " Medium icons", FileView::currentView == FileView::ViewMode::Medium)) FileView::currentView = FileView::ViewMode::Medium;
    if (CustomMenuItem(ICON_REG_GRID " Small icons", FileView::currentView == FileView::ViewMode::Small)) FileView::currentView = FileView::ViewMode::Small;
    if (CustomMenuItem(ICON_REG_LIST " List", FileView::currentView == FileView::ViewMode::List)) FileView::currentView = FileView::ViewMode::List;
    if (CustomMenuItem(ICON_REG_DOCUMENT_BULLET_LIST " Details", FileView::currentView == FileView::ViewMode::Details)) FileView::currentView = FileView::ViewMode::Details;
    if (CustomMenuItem(ICON_REG_APPS_LIST_DETAIL " Tiles", FileView::currentView == FileView::ViewMode::Tiles)) FileView::currentView = FileView::ViewMode::Tiles;
    // if (CustomMenuItem(ICON_REG_APPS_LIST " Content", FileView::currentView == FileView::ViewMode::Content)) FileView::currentView = FileView::ViewMode::Content;
    
    ImGui::Separator();
    
    CustomMenuItem(ICON_REG_PANEL_LEFT " Details pane", false);
    CustomMenuItem(ICON_REG_PANEL_RIGHT " Preview pane", false);
    
    ImGui::Separator();
    
    // The "Show >" sub-menu
    if (ImGui::BeginMenu("Show")) {
        CustomMenuItem(ICON_REG_PANEL_LEFT " Navigation pane", true, false); // True by default based on your image
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
