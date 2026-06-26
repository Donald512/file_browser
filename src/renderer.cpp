// renderer.cpp

#include "file_browser.h"
#include "imgui_boilerplate.h"
#include "history_helper.h"
#include "string_helper.h"
#include "file_backend.h"
#include "renderer.h"


static float sidebar_width = 200.0f;
extern String g_currentDir;
extern DirectoryList g_currentDirList;
extern PathHistory g_pathHistory;

const f32 BASE_BAR_HEIGHT = 42.0f;


void RenderMainSplit(const DirectoryList* dirList){
    f32 availableHeight = ImGui::GetContentRegionAvail().y; // leftover vertical space for the panels
    // LEFT COLUMN (Sidebar) ---
    RenderSideBar(availableHeight);
    ImGui::SameLine();    //  This tells ImGui to put the file grid nect to the sidebar 
    // RIGHT COLUMN (File Table) ---
    RenderFileGrid(dirList, availableHeight);
        
}



void RenderMainInterface(){
    // 1. Setup Fullscreen Window
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | 
                                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    if (ImGui::Begin("Main UI Workspace", nullptr, window_flags)){
        ImGui::PopStyleVar();
        
        // ==========================================
        // TABS & WINDOW CONTROLS (Top Bar)
        // ==========================================
        ImGuiWindowFlags topBarFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
        if (ImGui::BeginChild("TopBar", ImVec2(0, 80.0f * g_DpiScale), ImGuiChildFlags_None, topBarFlags)){
            

            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.08f, 0.08f, 1.00f));  // Push the dark color JUST for the Title Bar area
            if (ImGui::BeginChild("TitleBarArea", ImVec2(0, 32.0f * g_DpiScale), ImGuiChildFlags_None, topBarFlags)){  // 32 is the height of the title bar area
                RenderTitleBar();
            }   ImGui::EndChild();
            ImGui::PopStyleColor();
            
            // 32 is the height of the title bar area
            // 8 is the vertical distance/padding between the title bar and the address bar
            ImGui::SetCursorPos(ImVec2(10 * g_DpiScale, (32.0f + 8.0f ) * g_DpiScale));   
            RenderNavBar();

            ImGui::SetCursorPos(ImVec2(201.0f * g_DpiScale, (32.0f + 8.0f ) * g_DpiScale)); // the nav bar takes 201 width, then the rest splits between address and search bar

            RenderAddressBar(g_currentDir);
        }
        ImGui::EndChild();



        // =================================
        // MAIN SPLIT: SIDEBAR AND FILE GRID
        // =================================
        RenderMainSplit(&g_currentDirList);

    }
    ImGui::End();
}


void RenderFileGrid(const DirectoryList* dirList, f32 availableHeight){
    // RIGHT COLUMN (MAIN CONTENT) ---
    // Width 0 tells ImGui to fill the remaining horizontal screen space
    // 2. Create a Scrolling region for the files
    if (ImGui::BeginChild("FileViewRegion", ImVec2(0, availableHeight), ImGuiChildFlags_Borders, ImGuiChildFlags_NavFlattened)){

        // Starts a Child Window with ID "FileViewRegion", 2nd param is size of the child region - ImVec2(0, 0) means fill all available space, ImGuiChildFlags_Borders adds a visible border around the child, 
        
        // Define standard item size configs
        const f32 iconSize = 48.0f;
        const f32 cellWidth = iconSize + 32.0f; // Horizontal padding 
        
        f32 availWidth = ImGui::GetContentRegionAvail().x;    // Calculate how much width is available currently
        
        // Determine how many columns can fit based on width. Minimum 1 column
        u16 columnsCount = (u16) (availWidth / cellWidth);
        columnsCount = columnsCount ? columnsCount: 1;  // if columnsCount is 0, make it 1

        // 3. Create a layout grid using Tables
        // ImGuiTableFlags_NoSavedSettings stops ImGui from remembering column settings between runs
        if (ImGui::BeginTable("ExplorerGrid", columnsCount, ImGuiTableFlags_NoSavedSettings)){
            for (size_t i = 0; i < dirList->numEntries; i++){
                ImGui::TableNextColumn();
                FileItem currentItem = dirList->entries[i];
                
                // Group makes sure the item acts as a single cohesize block for interaction
                ImGui::BeginGroup();
                // - DRAW THE GRAPHICS ICON -
                // todo Replace the text icons with actual textures (via ImTextureID) 
                ImVec2 startCursorPos = ImGui::GetCursorScreenPos();
                
                if (currentItem.isFolder){
                    // Folder Graphic placeholder: Tinted Yellow button 
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.65f, 0.2f, 1.0f));    // Background color that RGBA color 

                    char folderId[32];
                    snprintf(folderId, sizeof(folderId), "[F]##%zu", i);
                    ImGui::Button(folderId, ImVec2(iconSize, iconSize)); 
                    // CLickable square button on the screen with visible text [F], everuthing after ## is hidden from the user but used by ImGui as a unique id, .c_str() converts from std::string to string literal, imvec2(iconsize, iconsize) makes it a perfect square
                    ImGui::PopStyleColor(); // makes the current button colorer the default style color, so other buttons dont inherit the same color
                }
                else{
                    // File Graphic placeholder: Tinted Blue/Grey Button
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.7f, 1.0f));

                    char fileId[32];
                    snprintf(fileId, sizeof(fileId), "[#]##%zu", i);
                    ImGui::Button(fileId, ImVec2(iconSize, iconSize));
                    ImGui::PopStyleColor();
                }
                
                // - DRAW TEXT BELOW ICON -
                // Center alignment math for text strings within the defined column grid block
                f32 textWidth = ImGui::CalcTextSize(currentItem.name.own_str).x; 
                if (textWidth < cellWidth){
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (iconSize - textWidth) * 0.5f);
                }
                
                // Wraps text smoothly if name exceeds column size limit
                ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + cellWidth);
                ImGui::Text("%s", currentItem.name.own_str);
                ImGui::PopTextWrapPos();
                
                ImGui::EndGroup();
                
                // - INTERATION HANDLING -
                if (ImGui::IsItemHovered()){
                    ImGui::SetTooltip("Type: %s", currentItem.isFolder? "Folder" : "File");
                }
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)){
                    
                }
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && (ImGui::IsItemHovered())){
                    if (currentItem.isFolder){
                        String newPath = CloneString(g_currentDir);
                        AppendSubDirectory(&newPath, &currentItem.name);
                        DestroyDirectoryList(&g_currentDirList);
                        NewBranch(newPath);     // adds a copy
                        DestroyString(&newPath);    
                        g_currentDirList = GetDirectoryContents(g_currentDir);
                        break; // ! very very important
                        
                    }
                }
            }
        }
        ImGui::EndTable();
    }
    ImGui::EndChild();
}


void RenderAddressBar(const String& path){
    static bool editMode = false;   
    static char pathBuffer[260] = {0};  // todo change to a larger number like 1024, or 32767 here and in GetDirectoryContents

    f32 barHeight = 32.0f * g_DpiScale;
    f32 barWidth = ImGui::GetContentRegionAvail().x * 0.62f;    // take up 62.5% of the leftover width

    // ImVec2 startPos = ImVec2(201.0f * g_DpiScale, (32.0f + 8.0f) * g_DpiScale) ;
    // startPos is the local coordinate inside the window, used for placing ImGui Buttons
    // startPos is the coordinate on the monitor, used for manually drawing shapes
    ImVec2 startPos = ImGui::GetCursorPos() ;
    ImVec2 absolutePos = ImGui::GetCursorScreenPos();    // this returns the actual, absolute monitor pixel, not local to the window it currently is

    // use DrawList to paint a grey rectanfle behind
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 maxBounds = ImVec2(absolutePos.x + barWidth, absolutePos.y + barHeight);
    
    drawList->AddRectFilled(absolutePos, maxBounds, ImGui::GetColorU32(ImGuiCol_FrameBg), ImGui::GetStyle().FrameRounding);
    drawList->AddRect(absolutePos, maxBounds, ImGui::GetColorU32(ImGuiCol_Border));

    // ===================================================================
    // universal formula for centering an item in a container is (ContainerHeight - ItemHeight)/2
    if (editMode){
        // mathematically center the text input cursor
        f32 inputPaddingY = (barHeight - ImGui::GetFontSize()) * 0.5f;
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, inputPaddingY));
        ImGui::SetNextItemWidth(barWidth);

        // Instantly focus the text box so that the user can start typing immediately
        if (!ImGui::IsAnyItemActive()){
            ImGui::SetKeyboardFocusHere();  // todo switch to justOpened bool flag, check what happens when i try to click in the box after clicking before
        }

        
        if (ImGui::InputText("##path_input", pathBuffer, sizeof(pathBuffer), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)){
            String targetPath = CreateString(pathBuffer);
            DirectoryList dirList = GetDirectoryContents(targetPath);

            // if the folder exists, navigate to it
            if (dirList.entries != nullptr){
                NewBranch(targetPath);
                DestroyDirectoryList(&g_currentDirList);

                g_currentDirList = dirList; // transfer ownership to global directory list
                
                dirList.entries = nullptr;
                dirList.numEntries = 0;
            }

            DestroyString(&targetPath);
            DestroyDirectoryList(&dirList); // this is safe because if navigation succeeded, it dirList.entries = nullptr, preventing a dangling pointer
            editMode = false;   // turn back into breadcrumbs
        }

        // If user clicks anywhere outside the input field
        if (ImGui::IsItemDeactivated() && !ImGui::IsItemActivated()){
            editMode = false;
        }

        ImGui::PopStyleVar();
    }
    else {

        
        char subDirName[260]; 
        u64 subDirIndex = 0;
        
        char currentPath[260];
        u64 currentPathIndex = 0;

        static char lastPopupPath[260] = {0};
        static DirectoryList cachedSubDirList = {0};
        
        u16 uniqueId = 0;
        
        // Center the internal buttons/folder/breadcrumbs vertically inside the container
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f * g_DpiScale);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f * g_DpiScale, 2.0f * g_DpiScale));

        f32 buttonHeight = ImGui::GetFrameHeight();
        f32 offsetY = (barHeight - buttonHeight) * 0.5f; // (container - item) / 2
        ImGui::SetCursorPos(ImVec2(startPos.x + 6.0f * g_DpiScale, startPos.y + offsetY));    // where tf do all these numbbers spawn from
        
        // group everything so ImGui Treats the breadcrumb bar as one 
        ImGui::BeginGroup();

        for (u64 i = 0; i <= path.length; i++){
            char c = path.own_str[i];
                
            // when a slash or the end of the string is reached, a folder is complete 
            if (c == '\\' || c == '\0'){
                subDirName[subDirIndex] = '\0';
                currentPath[currentPathIndex] = '\0';

                char btnlabel[256] = {0};
                snprintf(btnlabel, sizeof(btnlabel), "%s##dir_%d", subDirName, uniqueId);
                
                if (ImGui::Button(btnlabel)){
                    
                    String targetPath = CreateString(currentPath);
                    NewBranch(targetPath);
                    DestroyString(&targetPath);
                    
                    g_currentDirList = GetDirectoryContents(g_currentDir);
                    uniqueId++;
                    break;  // so it updates the new directory
                    
                }
                ImGui::SameLine();  // keeps the next item on same row 
                
                if (c == '\\'){
                    char dropDownId[64] = {0};
                    snprintf(dropDownId, sizeof(dropDownId), "dir_dropdown_%d", uniqueId);
                    const char* arrowSign = ImGui::IsPopupOpen(dropDownId) ? " v " : " > ";
                    
                    // buffer for the > and v sign 
                    char pathSep[32] = {0};
                    snprintf(pathSep, sizeof(pathSep), "%s##dir_%d", arrowSign, uniqueId);
                    
                    if (ImGui::Button(pathSep)){
                        ImGui::OpenPopup(dropDownId);                    
                    }
                    
                    // get bottom right coordinates of button that was just drawn
                    ImVec2 buttonBottomRight = ImGui::GetItemRectMax();
                    ImVec2 customPopupPos = ImVec2(buttonBottomRight.x, buttonBottomRight.y + 2.0f);
                    ImGui::SetNextWindowPos(customPopupPos);    // this just makes the popup, not appear where the cursor is, brings it below, so the user can see the > change to v
                    
                    ImGui::SameLine();
                    
                    if (ImGui::BeginPopup(dropDownId)){

                        if (strcmp(currentPath, lastPopupPath) != 0){
                            DestroyDirectoryList(&cachedSubDirList);

                            String folderPath = CreateString(currentPath); 
                            cachedSubDirList = GetDirectoryContents(folderPath);
                            DestroyString(&folderPath);

                            strncpy_s(lastPopupPath, currentPath, sizeof(lastPopupPath) - 1);
                            lastPopupPath[sizeof(lastPopupPath) - 1] = '\0';
                        }

                        for (u64 j = 0; j < cachedSubDirList.numEntries; j++){
                            if (cachedSubDirList.entries[j].isFolder){
                                const char* folderName = cachedSubDirList.entries[j].name.own_str;
                                
                                if (ImGui::Selectable(folderName)){  // Draw each folder entry as a clickable item in the dropdown list
                                    String targetPath = CreateString(currentPath);
                                    AppendSubDirectory(&targetPath, &cachedSubDirList.entries[j].name /* this is just the name, none of the parent stuff, so it has to be joined, to create the full path*/);
                                    NewBranch(targetPath);
                                    DestroyString(&targetPath);
                                    
                                    g_currentDirList = GetDirectoryContents(g_currentDir);                                
                                    ImGui::CloseCurrentPopup(); // close popup cos an item was chosen 
                                }
                            }
                        }
                        ImGui::EndPopup();
                    }
                    
                    uniqueId++;
                    currentPath[currentPathIndex] = '\\';
                    currentPathIndex++;
                }
                subDirIndex = 0;
            }
            else{
                subDirName[subDirIndex] = c;
                currentPath[currentPathIndex] = c;
                currentPathIndex++;
                subDirIndex++;
            }
        }
        
        ImGui::EndGroup();

        ImGui::PopStyleVar(); // clear rounding
        ImGui::PopStyleVar(); // clear padding

        // click detection for empty space background
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)){
            ImVec2 mousePos = ImGui::GetMousePos();
            if (ImGui::IsMouseHoveringRect(absolutePos, maxBounds)){
                // if they clicked the container but missed a breadcrumb button
                if (!ImGui::IsAnyItemActive() && !ImGui::IsAnyItemHovered()){
                    editMode = true;
                    snprintf(pathBuffer, sizeof(pathBuffer), "%s", path.own_str);
                }
            } 
        }
        // ImGui::SetCursorPos(ImVec2(startPos.x, startPos.y +barHeight));
    }
}


void ApplyWindows11DarkTheme(){
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // - - - Windown and Panel Layout Properties 
    style.WindowRounding    = 7.0f;  // Windows 11 signature slightly rounded corners
    style.FrameRounding     = 4.0f;
    style.PopupRounding     = 6.0f;
    style.ChildRounding     = 4.0f;
    style.WindowBorderSize  = 1.0f;  // Thin borders separating panels
    style.ChildBorderSize   = 1.0f;
    style.FrameBorderSize   = 1.0f;
    style.ItemSpacing       = ImVec2(8.0f, 6.0f);

    // --- 2. The Exact Windows 11 Color Tokens ---
    // Top-level app background (Mica / Acrylic dark slate)
    colors[ImGuiCol_WindowBg]             = ImVec4(0.12f, 0.12f, 0.12f, 1.00f); // #1E1E1E
    
    // Left Sidebar / Navigation Pane (Slightly darker for depth)
    colors[ImGuiCol_ChildBg]              = ImVec4(0.10f, 0.10f, 0.10f, 1.00f); // #1A1A1A
    
    // Popups, Dropdowns, and Dialogs
    colors[ImGuiCol_PopupBg]              = ImVec4(0.14f, 0.14f, 0.14f, 0.98f); // #242424

    // Subtle divider lines and panel borders
    colors[ImGuiCol_Border]               = ImVec4(0.18f, 0.18f, 0.18f, 1.00f); // #2E2E2E
    colors[ImGuiCol_BorderShadow]         = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    // Text (Off-white / Light grey)
    colors[ImGuiCol_Text]                 = ImVec4(0.90f, 0.90f, 0.90f, 1.00f); // #E6E6E6
    colors[ImGuiCol_TextDisabled]         = ImVec4(0.50f, 0.50f, 0.50f, 1.00f); // #808080

    // --- 3. Interactive States (Buttons, Selectables, Hovers) ---
    // The classic Windows Accent Blue (#0078D4 or updated Win11 #005A9E variant)
    ImVec4 winAccentBlue = ImVec4(0.00f, 0.45f, 0.83f, 1.00f); 
    
    // Normal button / background elements
    colors[ImGuiCol_FrameBg]              = ImVec4(0.16f, 0.16f, 0.16f, 1.00f); // #292929
    colors[ImGuiCol_FrameBgHovered]       = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBgActive]        = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);

    // Folder selectables and Header rows (Light highlight on hover)
    colors[ImGuiCol_Header]               = ImVec4(0.18f, 0.18f, 0.18f, 1.00f); // #2E2E2E
    colors[ImGuiCol_HeaderHovered]        = ImVec4(0.22f, 0.22f, 0.22f, 1.00f); // #383838
    colors[ImGuiCol_HeaderActive]         = winAccentBlue;

    // Address Bar Buttons & standard clickables
    colors[ImGuiCol_Button]               = ImVec4(0.14f, 0.14f, 0.14f, 0.00f); // Invisible by default like Win11 toolbars
    colors[ImGuiCol_ButtonHovered]        = ImVec4(0.22f, 0.22f, 0.22f, 1.00f); // Light grey on hover
    colors[ImGuiCol_ButtonActive]         = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);

    // Scrollbars
    colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.10f, 0.10f, 0.10f, 0.00f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
}


void RenderNavBar(){

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f); // sharp squares
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);   // Hard-remove any outline borders
    if (ImGui::Button("<")) {
        printf("n'azu\n");
        if (NavigateBackward()){
            g_currentDirList = GetDirectoryContents(g_currentDir);
        }
    }        ImGui::SameLine();
    
    if (ImGui::Button(">")) {
        printf("n'iru\n");
        if (NavigateForward()){
            g_currentDirList = GetDirectoryContents(g_currentDir);
        } 
    }     ImGui::SameLine();
    
    if (ImGui::Button("^")) {
        printf("nne na nna\n");

        String parentPath = CloneString(g_currentDir);
        PopPath(&parentPath);
        NewBranch(parentPath);
        DestroyString(&parentPath);

        g_currentDirList = GetDirectoryContents(g_currentDir);
    }          ImGui::SameLine();
    
    if (ImGui::Button("C")) { /* Reload logic */ };

    ImGui::PopStyleVar(2);
}

void RenderSideBar(f32 availableHeight){

    // Push the slightly darker color specifically for the sidebar panel
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyle().Colors[ImGuiCol_ChildBg]);

    if (ImGui::BeginChild("Sidebar", ImVec2(sidebar_width, availableHeight), ImGuiChildFlags_Borders)){

        // Authentic Windows 11 Sidebar content spacing
        ImGui::Spacing();
        ImGui::TextDisabled(" Quick Access");
        ImGui::Separator();

        // Selectable items
        if (ImGui::Selectable("  ⭐ Favourites")) { /* Navigation logic */ }
        if (ImGui::Selectable("  📥 Downloads"))  { /* Navigation logic */ }
        if (ImGui::Selectable("  📄 Documents"))  { /* Navigation logic */ }

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::TextDisabled("  This PC");
        ImGui::Separator();

        if (ImGui::Selectable("  🖥️ Desktop")) { /* Navigation logic */ }
        if (ImGui::Selectable("  💽 Local Disk (C:)")) {
            // Navigate to root C:
            String cDrive = CreateString("C:");
            NewBranch(cDrive);
            DestroyString(&cDrive);
            g_currentDirList = GetDirectoryContents(g_currentDir);
        }
    } 
    ImGui::PopStyleColor();
    ImGui::EndChild();

}

void RenderTitleBar(){
    f32 windowWidth = ImGui::GetWindowWidth();

    f32 controlsWidth = 145 * g_DpiScale;
    f32 controlsStartX = windowWidth - controlsWidth;
    
    f32 controlButtonWidth = 45.0f * g_DpiScale;
    f32 controlButtonHeight = 32.0f * g_DpiScale;
    f32 closeStartX = windowWidth - controlButtonWidth;
    f32 maximizeStartX = windowWidth - 2 * controlButtonWidth;
    f32 minimizeStartX = windowWidth - 3 * controlButtonWidth;


    ImGui::SetCursorPos(ImVec2(controlsStartX, 16.0f));
    ImGui::TextDisabled("|");
        
    
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f); // sharp squares
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);   // Hard-remove any outline borders
    
    ImGui::SetCursorPos(ImVec2(minimizeStartX, 0.0f));    // Should prolly be in the middle of the button
    if(ImGui::Button(" - ##win_min", ImVec2(controlButtonWidth, controlButtonHeight))){
        // minimize window
        ::ShowWindow(g_hwnd, SW_MINIMIZE);
    }

    ImGui::SetCursorPos(ImVec2(maximizeStartX, 0.0f));    // Should prolly be in the middle of the button
    if(ImGui::Button(" [ ] ##win_max", ImVec2(controlButtonWidth, controlButtonHeight))){
        // restore window
        if (::IsZoomed(g_hwnd)) ::ShowWindow(g_hwnd, SW_RESTORE);
        else ::ShowWindow(g_hwnd, SW_MAXIMIZE);
    }

    // to give hover-red highlight
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.90f, 0.11f, 0.14f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.70f, 0.09f, 0.11f, 1.0f)); // Darker red on click

    ImGui::SetCursorPos(ImVec2(closeStartX, 0.0f));    // Should prolly be in the middle of the button
    if(ImGui::Button("X ##win_close", ImVec2(controlButtonWidth, controlButtonHeight))){
        ::PostQuitMessage(0);
    }
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}