#include "file_browser.h"
#include "imgui_boilerplate.h"
// #include <assert.h>

String g_currentDir;
directory_list* g_currentDirList = nullptr;
static float sidebar_width = 200.0f;
PathHistory* g_path_history;




int main(void){
    // 1. Setup phase (Runs ONCE)
    HWND window = CreateMyOSWindow(); 
    if (!window) return 1;

    if (!InitializeGraphicsAPI(window)) return 1;
    
    ::ShowWindow(window, SW_SHOWDEFAULT);
    ::UpdateWindow(window);

    InitializeImGui(window);
  
    createString(g_currentDir, "C:");
    g_currentDirList = ReturnFilesInDir(g_currentDir);
  
    // 2. The Main Loop (Runs continuously at 60+ frames per second)
    bool running = true;
    while (running) {
        
        // A. Handle Windows events (clicks, closes, moves)
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) running = false;
        }

        g_SwapChainOccluded = false;

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        ImGui_Backend_NewFrame();
        ImGui::NewFrame();

        // C. Call YOUR code!
        RenderMainInterface(g_currentDirList);

        // D. Hand the instructions over to your graphics card to paint pixels
        ImGui::Render();
        MyGraphicsAPI_PresentFrame(); 
    }

    // 3. Cleanup phase (Runs ONCE when exiting)
    FreeDirectoryList(g_currentDirList);
    ShutdownImGui(window);
    return 0;
}


directory_list* ReturnFilesInDir(String &dirName){   // todo IM so confused onn what text encoding i currently am

    printf("%s\n", dirName.str);
    directory_list* dir_list = (directory_list*) malloc(sizeof(directory_list));
    if (!dir_list){
        printf("Malloc failed to alloc for directory_list.\n");
    }

    WIN32_FIND_DATAW ffd;
    LARGE_INTEGER filesize;
    u64 pathLength = dirName.length + 3;
    wchar_t* szDir = (wchar_t*) malloc(sizeof(wchar_t) * pathLength); // for /*NULL
    HANDLE hFind = INVALID_HANDLE_VALUE;
    DWORD dwError = 0;
    u64 fileCount = 0;

    // Check that the input path plus 3 is not longer than MAX_PATH.
    // Three characters are for the "\*" plus NULL appended below.
    if (dirName.length > MAX_PATH - 3){
        printf("Directory path is too long: %s, length: %llu\n", dirName.str, dirName.length);
        assert(0);
    }
    wchar_t* wideDirName = Utf8ToWide(dirName.str);
    StringCchCopyW(szDir, pathLength, wideDirName);
    StringCchCatW(szDir, pathLength, L"\\*");
    free(wideDirName);

    hFind = FindFirstFileW(szDir, &ffd);
    if (hFind == INVALID_HANDLE_VALUE){
        dwError = GetLastError();
        printf("%lu\n", dwError);
        assert(0);
    }
    
    do{
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) continue; // its a fake folder and will break the code
        if (!wcscmp(ffd.cFileName, L".") || !wcscmp(ffd.cFileName, L"..")) continue;    // dont want to show the . and ..
        fileCount++;
    }   while(FindNextFileW(hFind, &ffd));
    
    dwError = GetLastError();
    if (dwError != ERROR_NO_MORE_FILES){
        printf("%lu\n", dwError);
        printf("%p\n", hFind);
        assert(0);
    }

    dir_list->start_of_file_items = (FileItem*) malloc(sizeof(FileItem) * fileCount);
    dir_list->num_items = fileCount;

    FindClose(hFind);

    hFind = FindFirstFileW(szDir, &ffd);
    u64 i = 0;
    do{
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) continue; // its a fake folder and will break the code
        if (!wcscmp(ffd.cFileName, L".") || !wcscmp(ffd.cFileName, L"..")) continue;    // dont want to show the . and ..
        FileItem currentItem;
        u64 stringLength;
        StringCchLengthW(ffd.cFileName, MAX_PATH, &stringLength);
        char* fileName = WideToUtf8(ffd.cFileName);
        String* strObj = (String*)malloc(sizeof(String));
        createString(*strObj, fileName);
        currentItem.name = strObj;
        currentItem.isFolder = (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        dir_list->start_of_file_items[i] = currentItem;
        free(fileName);
        i++;
    }   while (FindNextFileW(hFind, &ffd) != 0);
    
    free(szDir);
    FindClose(hFind);

    return dir_list;
}

// void RenderFileExplorer(directory_list* dir_list){
    
//     // 1. Setup a main window panel
//     ImGui::SetNextWindowPos(ImVec2(0, 0));  // Snap to top left
//     ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);   // fill the canvas

//     ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
//     if (ImGui::Begin("File Explorer"), NULL, flags){  // Starts a window with this name, returns true if window is visible and rendered 
        
//         // Top control bar ( could add back buttons, search paths here later)
//         ImGui::Text("Directory Content:");
//         ImGui::Separator(); // Draws a horizontal line 

//         // 2. Create a Scrolling region for the files
//         ImGui::BeginChild("FileViewRegion", ImVec2(0, 0), ImGuiChildFlags_Borders, ImGuiChildFlags_NavFlattened);  
//         // Starts a Child Window with ID "FileViewRegion", 2nd param is size of the child region - ImVec2(0, 0) means fill all available space, ImGuiChildFlags_Borders adds a visible border around the child, 

//         // Define standard item size configs
//         const f32 iconSize = 48.0f;
//         const f32 cellWidth = iconSize + 32.0f; // Horizontal padding 

//         f32 availWidth = ImGui::GetContentRegionAvail().x;    // Calculate how much width is available currently

//         // Determine how many columns can fit based on width. Minimum 1 column
//         u16 columnsCount = availWidth / cellWidth;
//         columnsCount = columnsCount ? columnsCount: 1;  // if columnsCount is 0, make it 1

//         // 3. Create a layout grid using Tables
//         // ImGuiTableFlags_NoSavedSettings stops ImGui from remembering column settings between runs
//         if (ImGui::BeginTable("ExplorerGrid", columnsCount, ImGuiTableFlags_NoSavedSettings)){
//             for (size_t i = 0; i < dir_list->num_items; i++){
//                 ImGui::TableNextColumn();
//                 FileItem currentItem = dir_list->start_of_file_items[i];

//                 // Group makes sure the item acts as a single cohesize block for interaction
//                 ImGui::BeginGroup();
//                 // - DRAW THE GRAPHICS ICON -
//                 // todo Replace the text icons with actual textures (via ImTextureID) 
//                 ImVec2 startCursorPos = ImGui::GetCursorScreenPos();

//                 if (currentItem.isFolder){
//                     // Folder Graphic placeholder: Tinted Yellow button 
//                     ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.65f, 0.2f, 1.0f));    // Background color that RGBA color 
//                     ImGui::Button(("[F]##" + std::to_string(i)).c_str(), ImVec2(iconSize, iconSize)); 
//                     // CLickable square button on the screen with visible text [F], everuthing after ## is hidden from the user but used by ImGui as a unique id, .c_str() converts from std::string to string literal, imvec2(iconsize, iconsize) makes it a perfect square
//                     ImGui::PopStyleColor(); // makes the current button colorer the default style color, so other buttons dont inherit the same color
//                 }
//                 else{
//                     // File Graphic placeholder: Tinted Blue/Grey Button
//                     ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.7f, 1.0f));
//                     ImGui::Button(("[#]##" + std::to_string(i)).c_str(), ImVec2(iconSize, iconSize));
//                     ImGui::PopStyleColor();
//                 }

//                 // - DRAW TEXT BELOW ICON -
//                 // Center alignment math for text strings within the defined column grid block
//                 f32 textWidth = ImGui::CalcTextSize(currentItem.name->str).x;  // todo convert wide to utf8
//                 if (textWidth < cellWidth){
//                     ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (iconSize - textWidth) * 0.5f);
//                 }

//                 // Wraps text smoothly if name exceeds column size limit
//                 ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + cellWidth);
//                 ImGui::Text("%s", currentItem.name->str);
//                 ImGui::PopTextWrapPos();

//                 ImGui::EndGroup();

//                 // - INTERATION HANDLING -
//                 if (ImGui::IsItemHovered()){
//                     ImGui::SetTooltip("Type: %s", currentItem.isFolder? "Folder" : "File");
//                 }
//                 if (ImGui::IsItemClicked(ImGuiMouseButton_Left)){

//                 }
//                 if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && (ImGui::IsItemHovered())){
//                     if (currentItem.isFolder){
//                         joinDirs(g_currentDir, *currentItem.name);
//                         FreeDirectoryList(g_currentDirList);
//                         g_currentDirList = ReturnFilesInDir(g_currentDir);
//                         break; // ! very very important
                        
//                     }
//                 }
//             }
//             ImGui::EndTable();
//         }
//         ImGui::EndChild();
        
//         ImGui::End();
//     }
// }

void RenderFileGrid(directory_list* dir_list){
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
            for (size_t i = 0; i < dir_list->num_items; i++){
                ImGui::TableNextColumn();
                FileItem currentItem = dir_list->start_of_file_items[i];
                
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
                f32 textWidth = ImGui::CalcTextSize(currentItem.name->str).x;  // todo convert wide to utf8
                if (textWidth < cellWidth){
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (iconSize - textWidth) * 0.5f);
                }
                
                // Wraps text smoothly if name exceeds column size limit
                ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + cellWidth);
                ImGui::Text("%s", currentItem.name->str);
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
                        joinDirs(g_currentDir, *currentItem.name);
                        FreeDirectoryList(g_currentDirList);
                        g_currentDirList = ReturnFilesInDir(g_currentDir);
                        break; // ! very very important
                        
                    }
                }
            }
            ImGui::EndTable();
        }
        ImGui::EndChild();
    }
}


void RenderMainInterface(directory_list* dir_list){
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
        if (ImGui::BeginChild("TopBar", ImVec2(0, 40), false)){
            ImGui::SetCursorPos(ImVec2(10, 10)); // Indent a bit

            // Mockup Tabs
            ImGui::Button("Local Disk (C:)"); ImGui::SameLine();
            ImGui::Button(" X ");             ImGui::SameLine(); // Close tab 
            ImGui::Button(" + "); // New tab

            // Window Controls (Right Aligned)
            float right_edge = ImGui::GetWindowWidth();
            ImGui::SameLine(right_edge - 100); 
            ImGui::Button("_");     ImGui::SameLine();
            ImGui::Button("[ ]");   ImGui::SameLine();
            ImGui::Button("X");
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
            if (ImGui::Button("<")) { /* Back logic */ }        ImGui::SameLine();
            if (ImGui::Button(">")) { /* Forward logic */ }     ImGui::SameLine();
            if (ImGui::Button("^")) {
                goToParentDir(g_currentDir);
                g_currentDirList = ReturnFilesInDir(g_currentDir);
            }          ImGui::SameLine();
            if (ImGui::Button("C")) { /* Reload logic */ }      ImGui::SameLine();
            
            // Address Bar Placeholder
            ImGui::SetNextItemWidth(400);
            char path_buffer[256] = "This PC > Local Disk (C:)";
            ImGui::InputText("##PathBar", path_buffer, sizeof(path_buffer));
            
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
            // Note: You need to pass dir_list to this block
            RenderFileGrid(dir_list); 
        }
        ImGui::EndChild();
        ImGui::EndTable();
    }
    ImGui::End();
}


wchar_t* Utf8ToWide(char* utf8){
    u64 len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);

    if (!len){
        printf("Error in MultiByteToWideChar. Error: %lu. String: %s\n", GetLastError(), utf8);
        return nullptr;
    }
    // on heap 
    wchar_t* new_string = (wchar_t*) malloc(sizeof(wchar_t) * len);
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, new_string, len);

    return new_string;
}

char* WideToUtf8(wchar_t* wide){
    u64 len = WideCharToMultiByte(CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);

    if (!len){
        printf("Error in WideCharToMultiByte. Error: %lu. String: %ls\n", GetLastError(), wide);
        return nullptr;
    }
    // on heap 
    char* new_string = (char*) malloc(sizeof(char) * len);
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, new_string, len, 0, 0);

    return new_string;
}

void createString(String &string, const char* str_literal){
    u64 len = strlen(str_literal);
    string.capacity = 64;
    while (len >= string.capacity){
        string.capacity *= 2;
    }
    // this is going on the heap, not ro. data
    // So it technically is an array, technically...
    string.str = (char*)malloc(sizeof(char) * string.capacity);
    if (string.str == nullptr){
        printf("malloc failed for %s. capacity: %llu, length: %llu", str_literal, string.capacity, string.length);
        free(string.str);
        return;
    }
    strcpy_s(string.str, string.capacity, str_literal);
    string.length = len;
}

void joinDirs(String &dest, String &src){
    u64 requiredLen = dest.length + 1 + src.length ; // Total length needs to include backslash
    bool change = requiredLen > dest.capacity;
    if (requiredLen >= dest.capacity){
        while (requiredLen >= dest.capacity){
            dest.capacity *= 2;
        }
        char* buffer = (char*)malloc(sizeof(char) * dest.capacity); 
        assert((buffer != nullptr) && "malloc failed in joinDirs");
        strcpy_s(buffer, dest.capacity, dest.str);
        free(dest.str);
        dest.str = buffer;
    }
    strcat_s(dest.str, dest.capacity, "\\");
    strcat_s(dest.str, dest.capacity, src.str);

    dest.length = requiredLen;


}

void goToParentDir(String &currentDir){
    u64 length = currentDir.length;
    // printf("%c, %c\n", currentDir.str[length-1], currentDir.str[length-2]);
    if (currentDir.str[length-1] == ':' && currentDir.length <=3){
        printf("At root, so returning\n");
        return;  // at a root directory
    }
    // i represents position of last backslash
    i64 i;
    for (i = (i64) currentDir.length; i >= 0 && currentDir.str[i] != '\\'; i--);
    ZeroMemory(currentDir.str + i, currentDir.length - i);
    currentDir.length = i;
}

// void joinDirs(String &dest, char* src){
//    
// }

void FreeDirectoryList(directory_list* dir_list){
    if (!dir_list) return;
    // Free every string allocated
    for (u64 i = 0; i < dir_list->num_items; i++){
        free(dir_list->start_of_file_items[i].name->str);
        free(dir_list->start_of_file_items[i].name);
    }
    free(dir_list->start_of_file_items);
    free(dir_list);
}


void initHistory(){
    
}