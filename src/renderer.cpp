#include "file_browser.h"
#include "imgui_boilerplate.h"
#include "history_helper.h"
#include "string_helper.h"
#include "file_backend.h"


static float sidebar_width = 200.0f;
extern String g_currentDir;
extern DirectoryList g_currentDirList;
extern PathHistory g_pathHistory;


void RenderFileGrid(const DirectoryList* dirList){
    // 2. Create a Scrolling region for the files
    if (ImGui::BeginChild("FileViewRegion", ImVec2(0, 0), ImGuiChildFlags_Borders, ImGuiChildFlags_NavFlattened)){

        // Starts a Child Window with ID "FileViewRegion", 2nd param is size of the child region - ImVec2(0, 0) means fill all available space, ImGuiChildFlags_Borders adds a visible border around the child, 
        
        // Define standard item size configs
        const f32 iconSize = 48.0f;
        const f32 cellWidth = iconSize + 32.0f; // Horizontal padding 
        
        f32 availWidth = ImGui::GetContentRegionAvail().x;    // Calculate how much width is available currently
        
        // Determine how many columns can fit based on width. Minimum 1 column
        u16 columnsCount = availWidth / cellWidth;
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
                    ImGui::Button(("[F]##" + std::to_string(i)).c_str(), ImVec2(iconSize, iconSize)); 
                    // CLickable square button on the screen with visible text [F], everuthing after ## is hidden from the user but used by ImGui as a unique id, .c_str() converts from std::string to string literal, imvec2(iconsize, iconsize) makes it a perfect square
                    ImGui::PopStyleColor(); // makes the current button colorer the default style color, so other buttons dont inherit the same color
                }
                else{
                    // File Graphic placeholder: Tinted Blue/Grey Button
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.7f, 1.0f));
                    ImGui::Button(("[#]##" + std::to_string(i)).c_str(), ImVec2(iconSize, iconSize));
                    ImGui::PopStyleColor();
                }
                
                // - DRAW TEXT BELOW ICON -
                // Center alignment math for text strings within the defined column grid block
                f32 textWidth = ImGui::CalcTextSize(currentItem.name.own_str).x;  // todo convert wide to utf8
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
            ImGui::EndTable();
        }
        ImGui::EndChild();
    }
}


void RenderMainInterface(const DirectoryList* dirList){
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
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));
        // if (ImGui::BeginChild("TopBar", ImVec2(0, 40), false)){
        //     ImGui::SetCursorPos(ImVec2(10, 10)); // Indent a bit

        //     // Mockup Tabs
        //     ImGui::Button("Local Disk (C:)"); ImGui::SameLine();
        //     ImGui::Button(" X ");             ImGui::SameLine(); // Close tab 
        //     ImGui::Button(" + "); // New tab

        //     // Window Controls (Right Aligned)
        //     float right_edge = ImGui::GetWindowWidth();
        //     ImGui::SameLine(right_edge - 100); 
        //     ImGui::Button("_");     ImGui::SameLine();
        //     ImGui::Button("[ ]");   ImGui::SameLine();
        //     ImGui::Button("X");
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();

        // ==========================================
        // NAVIGATION BAR (Back, Forward, Path)
        // ==========================================
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
        if (ImGui::BeginChild("NavBar", ImVec2(0, 50), true)){
            ImGui::SetCursorPos(ImVec2(10, 10));
            
            // Navigation Buttons (Replace with FontAwesome icons later)
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
            
            if (ImGui::Button("C")) { /* Reload logic */ }      ImGui::SameLine();
            
            // Address Bar Placeholder
            ImGui::SetNextItemWidth(400);
            ImGui::InputText("##PathBar:", g_currentDir.own_str, g_currentDir.capacity, ImGuiInputTextFlags_ReadOnly);
            
            // Space for your "monitor" and extra tools on the right
            ImGui::SameLine();
            ImGui::Text(" |  [ Placeholder for other tools ]");
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();
    }

    ImGuiTableFlags table_flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV;
    if (ImGui::BeginTable("MainSplitter", 2, table_flags)) {
        ImGui::TableSetupColumn("Sidebar", ImGuiTableColumnFlags_WidthFixed, sidebar_width);
        ImGui::TableSetupColumn("Content", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextRow();

        // --- LEFT COLUMN ---
        ImGui::TableSetColumnIndex(0);
        // ... [Your Sidebar code] ...

        // --- RIGHT COLUMN ---
        ImGui::TableSetColumnIndex(1);
        if (ImGui::BeginChild("ContentArea", ImVec2(0, 0), false)) {
            
            // THE COLOR PICKER: This actually modifies the global variable!
            ImGui::Text("UI Settings:");
            ImGui::ColorEdit3("Background", (float*)&g_clear_color, ImGuiColorEditFlags_NoInputs);
            ImGui::Separator();

            // NOW, call the file loop inside here
            // Note: You need to pass dirList to this block
            RenderFileGrid(dirList); 
        }
        ImGui::EndChild();
        ImGui::EndTable();
    }
    ImGui::End();
}


void RenderAddressBar(const String& path){
    
    //  Buffer for enough to hold the text for each string's button
    char* subDirName = (char*) calloc(path.length, sizeof(char));
    u64 subDirIndex = 0;
    
    char* currentPath = (char*) calloc(path.length, sizeof(char));
    u64 currentPathIndex = 0;

    int uniqueId = 0;   // for imgui buttons

    // Push Invisble button backgrounds
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));


    for (u64 i = 0; i <= path.length; i++){ // <= to capture the last \0
        char currentChar = path.own_str[i];

    
        if (currentChar == '\\' || currentChar == '\0'){
            if (subDirIndex > 0){   // so we dont draw an empty string
                subDirName[subDirIndex] = '\0';
                currentPath[currentPathIndex] = '\0';

                // Draw folder button
                char btnLabel[256];
                snprintf(btnLabel, sizeof(btnLabel), "%s##dir_%d", subDirName, uniqueId);
                if (ImGui::Button(btnLabel)){
                    String targetPath = CreateString(currentPath);
                    NewBranch(targetPath);
                    DestroyString(&targetPath);

                    g_currentDirList = GetDirectoryContents(g_currentDir);
                    
                    break;
                }

                ImGui::SameLine();

                // Draw > sign 
                if (currentChar == '\\'){
                    std::string pathSeparator = " > ##" + std::to_string(uniqueId++);
                    if (ImGui::Button(pathSeparator.c_str())){
                        // Dropdown logic
                    }   
                    ImGui::SameLine();

                    // Backslash for next dir
                    currentPath[currentPathIndex] = '\\';
                    currentPathIndex++;

                    ZeroMemory(subDirName, path.length + 1);
                    subDirIndex = 0;
                }
                else {
                    subDirName[subDirIndex++] = currentChar;
                    currentPath[currentPathIndex++] = currentChar; 
                }

            }
            ImGui::PopStyleColor(); 
            free(subDirName);
            free(currentPath);
        }
    }
}