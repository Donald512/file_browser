#include "UI.h"
#include "Str.h"

using namespace AddressBar;
namespace Helpers = UI::Helpers;


static bool s_isEditing = false;
static bool s_justOpened = false;
static char s_pathInputBuffer[1024] = {0};

static u64 s_cachedPopupFolder;
static std::vector<WShell::ItemLite> s_cachedPopupItems;
static f32 s_cachedPopupMaxTextWidth = 0.0f;
static f32 s_cachedDpi = 0.0f;  // since it depends on dpi too

static void PathEditor(AppContext& ctx){
    const f32 verticalPadding = 4.0f * ctx.ui.dpiScale;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f * ctx.ui.dpiScale, verticalPadding));

    // Center text input cursor vertically
    f32 inputHeight = ImGui::GetFontSize() + verticalPadding * 2.0f;  // Font + (Top + Bottom)Padding
    Helpers::AlignCursorVertically(Height * ctx.ui.dpiScale, inputHeight);

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);  // done to make textbox not the size of text

    if (s_justOpened){
        ImGui::SetKeyboardFocusHere();
        s_justOpened = false;
    }

    ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll;
    if (ImGui::InputText("##address_bar_path_input", s_pathInputBuffer, sizeof(s_pathInputBuffer), flags)){

        wchar_t* widePath = Str::Utf8ToWide(s_pathInputBuffer);
       WShell::Pidl targetPidl = WShell::Pidl(WShell::TypeablePathToPidl(widePath));
    
        ctx.navigation.NavigateTo(targetPidl.get());
        free(widePath);
        s_isEditing = false;
    }

    // End editing if user clicks elsewhere
    if (ImGui::IsItemDeactivated() && !ImGui::IsItemActivated()){
        s_isEditing = false;
    }
    ImGui::PopStyleVar();   // FramePadding
}    

static void Breadcrumbs(AppContext& ctx){
    f32 dpi = ctx.ui.dpiScale;

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f * ctx.ui.dpiScale, 4.0f * ctx.ui.dpiScale));
    auto RenderPopup = [&](const char* popupID, PCIDLIST_ABSOLUTE folder, u64 hash){
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f * dpi, 8.0f * dpi));
        if (ImGui::BeginPopup(popupID)){

            bool folderChanged = (s_cachedPopupFolder != hash);
            if (folderChanged){
                s_cachedPopupItems = WShell::GetLiteItems(folder);
                s_cachedPopupFolder = hash;
            }

            MenuRowStyle rowStyle;
            rowStyle.outerMargin = 2.0f;
            rowStyle.innerPad    = 8.0f;
            rowStyle.rounding    = 4.0f;
            rowStyle.itemSpacing = ImVec2(0.0f, 6.0f);

            // has to be recomputed when sidebar changes, 
            if (!folderChanged || dpi != s_cachedDpi){
                f32 maxTextWidth = 0.0f;
                for (auto& item : s_cachedPopupItems){
                    f32 w = ImGui::CalcTextSize(item.name.c_str()).x;
                    if (w > maxTextWidth) maxTextWidth = w;
                }
                s_cachedPopupMaxTextWidth = maxTextWidth;
                s_cachedDpi = dpi;
            }
            f32 rowWidth = s_cachedPopupMaxTextWidth + (rowStyle.innerPad * 2.0f + rowStyle.outerMargin * 2.0f) * dpi;

            int i = 0;
            for (auto& item : s_cachedPopupItems){
                std::string rowId = "row" + std::to_string(i++);
                if (UI::Helpers::MenuRow(rowId.c_str(), item.name.c_str(), dpi, false, rowStyle, rowWidth)){
                    ctx.navigation.NavigateTo(item.pidl);
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::EndPopup();
        }
        ImGui::PopStyleVar();
    };

    f32 buttonHeight = ImGui::GetFrameHeight();

    Helpers::AlignCursorVertically(Height * ctx.ui.dpiScale);

    // Push This PC icon a little forward
    ImVec2 startPos = ImGui::GetCursorPos();
    ImGui::SetCursorPos({startPos.x + (8.0f * dpi), startPos.y} );

    const char* breadcrumbIcon = (ILIsEqual(ctx.pidlHome, ctx.navigation.CurrentFolder())) ? ICON_REG_HOME : ICON_REG_DESKTOP;
    ImGui::BeginDisabled();
    ImGui::Button(breadcrumbIcon, ImVec2(buttonHeight, buttonHeight));
    ImGui::EndDisabled();
    ImGui::SameLine(0, (8.0f * dpi) + ImGui::GetStyle().FramePadding.x * 0.5f);
    
    
    // you need two IDs for each popup, one for the popup window, and one for the popup button
    std::string firstPopupID = "##firstbcrumb_popup";
    std::string firstSign = ImGui::IsPopupOpen(firstPopupID.c_str()) ? ICON_REG_CHEVRON_DOWN : ICON_REG_CHEVRON_RIGHT;
    std::string firstPopupBtnID = firstSign + firstPopupID;

    if (ImGui::Button(firstPopupBtnID.c_str())){
        ImGui::OpenPopup(firstPopupID.c_str());
    }
    ImVec2 btnMin = ImGui::GetItemRectMin();
    ImVec2 btnMax = ImGui::GetItemRectMax();
    ImGui::SetNextWindowPos(ImVec2(btnMin.x, btnMax.y + 2.0f));

    static u64 s_desktopHash = WShell::HashPidl(ctx.pidlDesktop.get());
    RenderPopup(firstPopupID.c_str(), ctx.pidlDesktop.get(), s_desktopHash);
    
    ImGui::SameLine(0.0f, 8.0f * ctx.ui.dpiScale);

    auto& crumbs = ctx.navigation.Breadcrumbs().Crumbs();
    for (auto& crumb : crumbs){
        ImGui::PushID(&crumb);
        
        if (ImGui::Button(crumb.displayName.c_str())){
            ctx.navigation.NavigateTo(crumb.pidl);
        }

        bool isLast = &crumb == &crumbs.back();

        if (!isLast || ctx.navigation.Breadcrumbs().hasSubFolders){
            ImGui::SameLine();
            const char* popupID = "##bcrumbMenu";
            const char* sign = ImGui::IsPopupOpen(popupID) ? ICON_REG_CHEVRON_DOWN : ICON_REG_CHEVRON_RIGHT;

            if (ImGui::Button(sign)){
                ImGui::OpenPopup(popupID);
            }
            btnMin = ImGui::GetItemRectMin();
            btnMax = ImGui::GetItemRectMax();
            ImGui::SetNextWindowPos(ImVec2(btnMin.x, btnMax.y + 2.0f));
            RenderPopup(popupID, crumb.pidl, crumb.hash);
        
        }
        ImGui::PopID();
        ImGui::SameLine(0.0, 8.0f * ctx.ui.dpiScale);
    }
    ImGui::PopStyleVar();
}

void AddressBar::Render(AppContext& ctx){
    f32 windowWidth = ImGui::GetWindowWidth();
    f32 remainingWidth = windowWidth - ((ToolBar::LeftPadding + NavBar::Width + ToolBar::AddressToSearchGap + ToolBar::RightPadding) * ctx.ui.dpiScale);
    f32 addressWidth = remainingWidth * ToolBar::AddressRatio;
    
    f32 verticalPadding = (ToolBar::Height - AddressBar::Height) * 0.5f * ctx.ui.dpiScale;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, verticalPadding));

    UI::Helpers::AlignCursorVertically(ToolBar::Height * ctx.ui.dpiScale, Height * ctx.ui.dpiScale);

    if (!ImGui::BeginChild("AddressBar", ImVec2(addressWidth, Height * ctx.ui.dpiScale), ImGuiChildFlags_None, TopBar::Flags)){
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        ImGui::EndChild();
        return;
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    // ======== Real lines =========
    if (s_isEditing) PathEditor(ctx);
    else Breadcrumbs(ctx);

    // Empty Space Click Detection
    ImVec2 min = ImGui::GetWindowPos();
    ImVec2 max = ImVec2(min.x + addressWidth, min.y + Height * ctx.ui.dpiScale);

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsMouseHoveringRect(min, max)){
        if (!ImGui::IsAnyItemActive() && !ImGui::IsAnyItemHovered()){
            s_isEditing = true;

            s_justOpened = true;
            if (ctx.navigation.Breadcrumbs().fullPath.c_str()){
                strncpy_s(s_pathInputBuffer, sizeof(s_pathInputBuffer), ctx.navigation.Breadcrumbs().fullPath.c_str(), _TRUNCATE);
            }

            else{
                s_pathInputBuffer[0] = '\0';
            }
        }
    }
    ImGui::EndChild();
}