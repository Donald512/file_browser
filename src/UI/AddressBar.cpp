#include "UI.h"
#include "..\Str\Str.h"
// #include "Str.h"

namespace Colors = UI::Colors;
namespace Style = UI::Style;
using namespace AddressBar;

static bool s_isEditing = false;
static bool s_justOpened = false;
static char s_pathInputBuffer[1024] = {0};
WShell::Pidl cachedPopupFolder;
std::vector<WShell::ItemLite> cachedPopupItems;

static void PathEditor(AppContext& ctx){
    // if y parameter in below function changes, change it in inputHeight also, use variable later
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f * ctx.dpiScale, 4.0f* ctx.dpiScale));

    // Center text input cursor
    f32 inputHeight = ImGui::GetFontSize() + (4.0f * ctx.dpiScale * 2.0f);  // Font + (Top + Bottom)Padding
    f32 centerY = (Height * ctx.dpiScale - inputHeight) * 0.5f;
    ImGui::SetCursorPosY(centerY);

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
    
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f * ctx.dpiScale, 4.0f * ctx.dpiScale));

    auto RenderPopup = [&](const char* popupID, PCIDLIST_ABSOLUTE folder){
        // cache the directory contents so we dont fetch every frame
        if (ImGui::BeginPopup(popupID)){
            if (!ILIsEqual(cachedPopupFolder, folder)){
                cachedPopupItems =WShell::GetLiteItems(folder);
                cachedPopupFolder =WShell::Pidl(ILClone(folder));
            }
            for (auto& item : cachedPopupItems){
                ImGui::PushID(&item);
                if (ImGui::Selectable(item.name.c_str())){
                    ctx.navigation.NavigateTo(item.pidl);
                    ImGui::CloseCurrentPopup();
                    ImGui::PopID();
                    break;
                }
                ImGui::PopID();
            }
            ImGui::EndPopup();
        }
    };


    f32 buttonHeight = ImGui::GetFrameHeight();
    f32 centerY = (Height * ctx.dpiScale - buttonHeight) * 0.5f;
    ImGui::SetCursorPosY(centerY);

    const char* breadcrumbIcon = (ILIsEqual(ctx.pidlHome, ctx.navigation.CurrentFolder())) ? ICON_REG_HOME : ICON_REG_DESKTOP;
    ImGui::BeginDisabled();
    ImGui::Button(breadcrumbIcon, ImVec2(buttonHeight, buttonHeight));
    ImGui::EndDisabled();
    ImGui::SameLine();
    
    
    // you need two IDs for each popup, one for the popup window, and one for the popup button
    std::string firstPopupID = "##firstbcrumb_popup";
    std::string firstSign = ImGui::IsPopupOpen(firstPopupID.c_str()) ? ICON_REG_CHEVRON_DOWN : ICON_REG_CHEVRON_RIGHT;
    std::string firstPopupBtnID = firstSign + firstPopupID;

    if (ImGui::Button(firstPopupBtnID.c_str())){
        ImGui::OpenPopup(firstPopupID.c_str());
    }
    ImVec2 btnRect = ImGui::GetItemRectMax();
    ImGui::SetNextWindowPos(ImVec2(btnRect.x, btnRect.y + 2.0f));

    RenderPopup(firstPopupID.c_str(), ctx.pidlDesktop.get());
    
    ImGui::SameLine(0.0f, 8.0f * ctx.dpiScale);

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
            btnRect = ImGui::GetItemRectMax();
            ImGui::SetNextWindowPos(ImVec2(btnRect.x, btnRect.y + 2.0f));
            RenderPopup(popupID, crumb.pidl );
        
        }
        ImGui::PopID();
        ImGui::SameLine(0.0, 8.0f * ctx.dpiScale);
    }
    ImGui::PopStyleVar();
}

void AddressBar::Render(AppContext& ctx){
    f32 windowWidth = ImGui::GetWindowWidth();
    f32 remainingWidth = windowWidth - ((ToolBar::LeftPadding + NavBar::Width + ToolBar::AddressToSearchGap + ToolBar::RightPadding) * ctx.dpiScale);
    f32 addressWidth = remainingWidth * ToolBar::AddressRatio;
    
    f32 verticalPadding = (Height - AddressBar::Height) * 0.5f * ctx.dpiScale;

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f * ctx.dpiScale);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.16f, 0.16f, 0.16f, 1.0f)); // FrameBg color
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, verticalPadding));
    f32 centerY = (ToolBar::Height - Height) * ctx.dpiScale * 0.5f;
    ImGui::SetCursorPosY(centerY);
    if (!ImGui::BeginChild("AddressBar", ImVec2(addressWidth, Height * ctx.dpiScale), ImGuiChildFlags_None, TopBar::Flags)){
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(3);
        ImGui::EndChild();
        return;
    }
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);

    if (s_isEditing){
        PathEditor(ctx);
    }
    else{
        Breadcrumbs(ctx);
    }

    // Empty Space Click Detection
    ImVec2 min = ImGui::GetWindowPos();
    ImVec2 max = ImVec2(min.x + addressWidth, min.y + Height * ctx.dpiScale);

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