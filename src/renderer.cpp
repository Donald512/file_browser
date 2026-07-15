// renderer.cpp

#include "core.h"

namespace UI{
    void Render(AppContext& ctx){

        // Make the root ImGui window fill the entire Windows window.
        const ImGuiViewport* viewport = ImGui::GetMainViewport();   
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, Style::NoPadding);
        if (ImGui::Begin("Main UI Workspace", nullptr, WindowFlags)){
            ImGui::PopStyleVar();

            TopBar::Render(ctx);
            ToolBar::Render(ctx);
            FileView::Render(ctx);

            ImGui::End();
        }
    }
}

namespace TopBar{
    namespace Colors = UI::Colors;
    namespace Style = UI::Style;
    
    void Render(AppContext& ctx){
        f32 height = ::IsZoomed(ctx.hwnd) ? (Height * ctx.dpiScale) : (40.0f * ctx.dpiScale);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, Colors::TopBarBackground);
        if (!ImGui::BeginChild("TopBar", ImVec2(0, height), ImGuiChildFlags_None, Flags)){
            ImGui::PopStyleColor();
            ImGui::EndChild();
            return;
        }
        ImGui::PopStyleColor();

        f32 windowWidth = ImGui::GetWindowWidth();
        f32 captionBtnsWidth = TotalBtnsWidth * ctx.dpiScale;
        f32 captionBtnsStartX = windowWidth - captionBtnsWidth;

        f32 btnWidth = BtnWidth * ctx.dpiScale;
        f32 closeBtnWidth = CloseBtnWidth * ctx.dpiScale;

        f32 closeStartX = windowWidth - closeBtnWidth;
        f32 maximizeStartX = closeStartX - btnWidth;
        f32 minimizeStartX = maximizeStartX - btnWidth;
  
        ImGui::SetCursorPos(ImVec2(captionBtnsStartX, 0.0f));
        ImGui::TextDisabled("|");
        
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, Style::NoRounding); // sharp squares
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, Style::NoBorder);   // Hard-remove any outline borders
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, Style::NoPadding); // Forces razor-sharp centered glyph alignment 

        ImGui::SetCursorPos(ImVec2(minimizeStartX, 0.0f));   
        ImGui::Button(ICON_REG_SUBTRACT "##win_min", ImVec2(btnWidth, BtnHeight));

        // NOTE: Windows handles the actual maximize behavior so Snap Layouts continue to work.
        // These buttons are purely visual and forward the interaction to the native title bar.
        ImGui::SetCursorPos(ImVec2(maximizeStartX, 0.0f)); 
        const char* maximizeGlyph = ::IsZoomed(ctx.hwnd) ? ICON_REG_SQUARE_MULTIPLE "##win_max" : ICON_REG_MAXIMIZE "##win_max";
        ImGui::Button(maximizeGlyph, ImVec2(btnWidth, BtnHeight));

        // to give hover-red highlight
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.90f, 0.11f, 0.14f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.70f, 0.09f, 0.11f, 1.0f)); // Darker red on click

        ImGui::SetCursorPos(ImVec2(closeStartX, 0.0f)); 
        ImGui::Button(ICON_REG_DISMISS "##win_close", ImVec2(btnWidth, BtnHeight));
       
        ImGui::PopStyleColor(2);    // for close button 
        ImGui::PopStyleVar(3);  // rounding, bordersize, padding

        ImGui::EndChild();
    }
}

namespace ToolBar{
    void Render(AppContext& ctx){
        f32 height = Height * ctx.dpiScale;
        f32 leftPadding =  LeftPadding * ctx.dpiScale;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(leftPadding, 0.0f));
        if (!ImGui::BeginChild("ToolBar", ImVec2(0, height), ImGuiChildFlags_None, Flags)){
            ImGui::PopStyleVar();
            ImGui::EndChild();
            return;
        }
        ImGui::PopStyleVar();

        NavBar::Render(ctx);
        ImGui::SameLine();

        AddressBar::Render(ctx);

        ImGui::EndChild();
    }   
}

namespace NavBar{
    namespace Style = UI::Style;
    
    void Render(AppContext& ctx){
        f32 verticalPadding = (ToolBar::Height - Height) * 0.5f * ctx.dpiScale; // most likely 0, wonder what the compiler does to this
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, verticalPadding));

        if (!ImGui::BeginChild("NavBar", ImVec2(Width * ctx.dpiScale, Height * ctx.dpiScale), ImGuiChildFlags_None, TopBar::Flags)){
            ImGui::PopStyleVar();
            ImGui::EndChild();
            return;
        }
        ImGui::PopStyleVar();

        f32 btnSize = BtnSize * ctx.dpiScale;
        f32 margin = XPadding * ctx.dpiScale;
        f32 navSepSpace = btnSize + margin;

        // ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, navBtnSize * 0.5f); // 50% rounding
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,  4.0f * ctx.dpiScale); 
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, Style::NoBorder);   // remove any outline borders
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,Style::NoPadding);   // Autocenters text inside button, but we need to center button ourselves


        ImGui::BeginDisabled(!Navigation::CanGoBack(ctx));
        if (ImGui::Button(ICON_REG_ARROW_LEFT "##nav_backward", ImVec2(btnSize, btnSize))) { 
            Navigation::GoBack(ctx);
        }   ImGui::SameLine(0, margin);
        ImGui::EndDisabled();

        ImGui::BeginDisabled(!Navigation::CanGoForward(ctx));
        if (ImGui::Button(ICON_REG_ARROW_RIGHT "##nav_forward", ImVec2(btnSize, btnSize))) {
            Navigation::GoForward(ctx);
        }   ImGui::SameLine(0, margin);
        ImGui::EndDisabled();

        ImGui::BeginDisabled(!Navigation::CanGoParent(ctx));
        if (ImGui::Button(ICON_REG_ARROW_UP "##nav_parent", ImVec2(btnSize, btnSize))) {
            Navigation::GoParent(ctx);
        }   ImGui::SameLine(0, margin);
        ImGui::EndDisabled();


        if (ImGui::Button(ICON_REG_ARROW_CLOCKWISE "##refresh", ImVec2(btnSize, btnSize))) { 
            Backend::EnumerateDirectory(ctx, ctx.currentFolderPidl);
        };

        ImGui::PopStyleVar(3);
        ImGui::EndChild();
    }

} // namespace NavBar

namespace AddressBar{
    namespace ToolBarLayout = UI::Style::ToolBarLayout;

    static bool s_isEditing = false;
    static bool s_justOpened = false;
    static char s_pathInputBuffer[1024] = {0};

    static void RenderPathEditor(AppContext& ctx){
        // Center text input cursor
        f32 inputPaddingY = (Height * ctx.dpiScale - ImGui::GetFontSize()) * 0.5f;
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f * ctx.dpiScale, inputPaddingY));
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

        if (s_justOpened){
            ImGui::SetKeyboardFocusHere();
            s_justOpened = false;
        }

        ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll;
        if (ImGui::InputText("##address_bar_path_input", s_pathInputBuffer, sizeof(s_pathInputBuffer), flags)){
            wchar_t* widePath = Str::Utf8ToWide(s_pathInputBuffer);
            PIDLIST_ABSOLUTE targetPidl = Backend::CreatePidlFromPath(widePath);
            
            if (Navigation::NavigateTo(ctx, targetPidl)){
                History::Append(ctx, ctx.currentFolderPidl);
            }
            Utils::FreePidl(targetPidl);
            free(widePath);

            s_isEditing = false;
        }

        // End editing if user clicks elsewhere
        if (ImGui::IsItemDeactivated() && !ImGui::IsItemActivated()){
            s_isEditing = false;
        }
        ImGui::PopStyleVar();   // FramePadding
    }    

    static void RenderBreadcrumbs(AppContext& ctx){
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f * ctx.dpiScale, 4.0f * ctx.dpiScale));

        char folderBuffer[MAX_PATH] = {0}; 
        char arrowBuffer[MAX_PATH] = {0};
        for (u64 i = 0; i < ctx.currentBreadcrumbs.count; i++){
            BreadcrumbItem& crumb = ctx.currentBreadcrumbs.breadcrumbs[i];

            const char* safeName = crumb.displayName.data ? crumb.displayName.data : "Unknown";
            snprintf(folderBuffer, sizeof(folderBuffer), "%s##bcrumb_%lld", safeName, i);
            if (ImGui::Button(folderBuffer)){
                if (Navigation::NavigateTo(ctx, crumb.pidl)){
                    History::Append(ctx, ctx.currentFolderPidl);
                }
            }

            if (i < ctx.currentBreadcrumbs.count - 1 || (i == ctx.currentBreadcrumbs.count - 1 && ctx.currentBreadcrumbs.hasSubFolders)){
                ImGui::SameLine(0.0f, 8.0f * ctx.dpiScale);

                const char* arrowSign = ImGui::IsPopupOpen(arrowBuffer) ? ICON_REG_CHEVRON_DOWN : ICON_REG_CHEVRON_RIGHT; 
                snprintf(arrowBuffer, sizeof(arrowBuffer), "%s##bcrumb_arrow_%lld", arrowSign, i);   // use a new arrowBuffer or nah

                if (ImGui::Button(arrowBuffer)){
                    ImGui::OpenPopup(arrowBuffer);
                }

                ImVec2 btnRect = ImGui::GetItemRectMax();
                ImGui::SetNextWindowPos(ImVec2(btnRect.x, btnRect.y + 2.0f));

                if (ImGui::BeginPopup(arrowBuffer)){
                    // cache the directory contents so we dont fetch every frame
                    if (!ILIsEqual(ctx.popupCachePidl, crumb.pidl)){
                        Backend::FreeLightShellItemArray(ctx.popupCacheList);
                        Utils::FreePidl(ctx.popupCachePidl);
                        ctx.popupCacheList = Backend::GetDirectoryContents(crumb.pidl);
                        ctx.popupCachePidl = ILClone(crumb.pidl);
                    }

                    for (u64 j = 0; j < ctx.popupCacheList.numEntries; j++){
                        if (ImGui::Selectable(ctx.popupCacheList.entries[j].name.data)){
                            if (Navigation::NavigateTo(ctx, ctx.popupCacheList.entries[j].pidl)){
                                History::Append(ctx, ctx.currentFolderPidl);
                            }
                            ImGui::CloseCurrentPopup();
                            break;
                        }
                    }
                    ImGui::EndPopup();
                }
            }
            ImGui::SameLine(0.0f, 8.0f * ctx.dpiScale);
        }
        ImGui::PopStyleVar();   // FramePadding
    }

    void Render(AppContext& ctx){
        f32 windowWidth = ImGui::GetWindowWidth();
        f32 remainingWidth = windowWidth - ((ToolBarLayout::LeftPadding + NavBar::Width + ToolBarLayout::AddressToSearchGap + ToolBarLayout::RightPadding) * ctx.dpiScale);
        f32 addressWidth = remainingWidth * ToolBarLayout::AddressRatio;
        
        f32 verticalPadding = (Height - AddressBar::Height) * 0.5f * ctx.dpiScale;

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f * ctx.dpiScale);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.16f, 0.16f, 0.16f, 1.0f)); // FrameBg color
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, verticalPadding));
        if (!ImGui::BeginChild("AddressBar", ImVec2(addressWidth, Height * ctx.dpiScale), ImGuiChildFlags_None, TopBar::Flags)){
            ImGui::PopStyleColor();
            ImGui::PopStyleVar(3);
            ImGui::EndChild();
            return;
        }
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(3);

        if (s_isEditing){
            RenderPathEditor(ctx);
        }
        else{
            RenderBreadcrumbs(ctx);
        }

        // Empty Space Click Detection
        ImVec2 min = ImGui::GetWindowPos();
        ImVec2 max = ImVec2(min.x + addressWidth, min.y + Height * ctx.dpiScale);

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsMouseHoveringRect(min, max)){
            if (!ImGui::IsAnyItemActive() && !ImGui::IsAnyItemHovered()){
                s_isEditing = true;

                s_justOpened = true;
                if (ctx.currentBreadcrumbs.fullPath.data){
                    strncpy_s(s_pathInputBuffer, sizeof(s_pathInputBuffer), ctx.currentBreadcrumbs.fullPath.data, _TRUNCATE);
                }
                else{
                    s_pathInputBuffer[0] = '\0';
                }
            }
        }
        ImGui::EndChild();
    }


}

namespace FileView{
    namespace Colors = UI::Colors;
    namespace Style = UI::Style;
    
    void Render(AppContext& ctx){        
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
            Backend::DirectoryArray& dirs = ctx.currentDirArray;

            u16 totalRows = (u16) (dirs.numEntries + columnsCount - 1)/columnsCount; // the + columnsCount - 1/ columnsCount is there to celiling the result
            ImGuiListClipper clipper;
            clipper.Begin(totalRows);
            
            // instead of looping through, entries, we loop through visible rows
            while (clipper.Step()){
                for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++){  // DisplayStart is the first row, DisplayEnd is exclusive
                    u64 startItemIdx = (u64) row * columnsCount;
                    u64 endItemIdex = startItemIdx + columnsCount;
                    endItemIdex = (endItemIdex < dirs.numEntries) ? endItemIdex : dirs.numEntries;  // get the min, important for last row

                    for (u64 i = startItemIdx; i < endItemIdex; i++){
                        ImGui::TableNextColumn();

                        f32 realCellWidth = ImGui::GetContentRegionAvail().x;
                        
                        Backend::ShellItem& currentItem = dirs.entries[i];
                        bool isFolder = currentItem.attributes & SFGAO_FOLDER;
                        bool isSelected = (ctx.currentDirArray.selectedIndex == (i64)i);

                        ImGui::PushID((int)i);

                        ImVec2 startPos = ImGui::GetCursorPos();    // 1. save where we are to draw an invisible bounding box first

                        if (ImGui::Selectable("##file_selectedbox", isSelected, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(cellWidth, iconSize + ImGui::GetTextLineHeightWithSpacing() * 2))){
                            ctx.currentDirArray.selectedIndex = i;
                        }
                                            
                        // 2. - INTERATION HANDLING -
                        // Hovering
                        if (ImGui::IsItemHovered()){
                            ImGui::SetTooltip("Type: %s", isFolder ? "Folder" : "File");
                        }
                        // clicking
                        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)){
                            dirs.selectedIndex = i;                    
                        }
                        // Double click or enter key
                        if ((ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && ImGui::IsItemHovered()) || (isSelected && ImGui::IsKeyPressed(ImGuiKey_Enter ))){
                            if (isFolder){
                                if (Navigation::NavigateTo(ctx, currentItem.pidl)){
                                    History::Append(ctx, ctx.currentFolderPidl);    // currentItem.pidl is freed in NavigateTo'
                                    ImGui::PopID();
                                    clipper.End();
                                    ImGui::EndTable();
                                    ImGui::EndChild();
                                    return;
                                }
                            }
                            else{
                                Backend::ExecuteFile(currentItem.pidl);
                            }
                        }
                        // 3. Move cursor back to start so we can draw visuals on top of the selectable
                        ImGui::SetCursorPos(startPos);
                        ImGui::BeginGroup();

                        f32 columnStartX = ImGui::GetCursorPosX();
                        f32 centeredIconX = columnStartX + (realCellWidth - iconSize) * 0.5f;
                        ImGui::SetCursorPosX(centeredIconX);

                        // Draw a colored box as placeholder using ImDrawList, so it doesnt steal clicks like Button()
                        ImTextureID icon = Icons::GetIconTexture(ctx, currentItem.iconCacheKey);
                        if (icon){
                            ImGui::Image(icon, ImVec2(iconSize, iconSize));
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
                        f32 textWidth = ImGui::CalcTextSize(currentItem.name.data).x; 
                        if (textWidth < realCellWidth){
                            ImGui::SetCursorPosX(columnStartX + (realCellWidth - textWidth) * 0.5f);
                        }
                        else{
                            ImGui::SetCursorPosX(columnStartX);
                        }

                        // Wraps text smoothly if name exceeds column size limit
                        ImGui::PushTextWrapPos(columnStartX + realCellWidth);
                        ImGui::Text("%s", currentItem.name.data);
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
    
}

