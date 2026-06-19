#include "file_browser.h"
#include "imgui_boilerplate.h"

String g_currentDir;
DirectoryList g_currentDirList;
PathHistory g_pathHistory;

int main(void){
    // 1. Setup phase (Runs ONCE)
    HWND window = CreateMyOSWindow(); 
    if (!window) return 1;

    if (!InitializeGraphicsAPI(window)) return 1;
    
    ::ShowWindow(window, SW_SHOWDEFAULT);
    ::UpdateWindow(window);

    InitializeImGui(window);
  
    g_currentDir = CreateString("C:");
    g_currentDirList = GetDirectoryContents(g_currentDir);
  

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

        RenderMainInterface(&g_currentDirList);

        ImGui::Render();
        MyGraphicsAPI_PresentFrame(); 
    }

    // 3. Cleanup phase (Runs ONCE when exiting)
    DestroyDirectoryList(&g_currentDirList);
    ShutdownImGui(window);
    return 0;
}


DirectoryList GetDirectoryContents(const String directoryPath){

    printf("%s\n", directoryPath.own_str);
    // Check that the input path plus 3 is not longer than MAX_PATH.
    // Three characters are for the "\*" plus NULL appended below.
    if (directoryPath.length > MAX_PATH - 3){
        printf("Directory path is too long: %s, length: %llu\n", directoryPath.own_str, directoryPath.length);
        assert(0);
    }

    DirectoryList contents;
    u64 fileCount = 0;
    u64 pathSuffixLength = 3;  // for \*NULL
    u64 searchPathLength = directoryPath.length + pathSuffixLength;


    WIN32_FIND_DATAW ffd;
    // LARGE_INTEGER filesize;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    DWORD dwError = 0;

    wchar_t* wideSearchPath = Utf8ToWide(directoryPath.own_str, pathSuffixLength);
    StringCchCatW(wideSearchPath, searchPathLength, L"\\*");
    
    hFind = FindFirstFileW(wideSearchPath, &ffd);
    if (hFind == INVALID_HANDLE_VALUE){ 
        dwError = GetLastError();
        printf("%lu\n", dwError);
        free(wideSearchPath);
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
    
    contents.entries = (FileItem*) malloc(sizeof(FileItem) * fileCount);
    contents.numEntries = fileCount;
    
    FindClose(hFind);
    
    hFind = FindFirstFileW(wideSearchPath, &ffd);
    u64 i = 0;
    do{
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) continue; // its a fake folder and will break the code
        if (!wcscmp(ffd.cFileName, L".") || !wcscmp(ffd.cFileName, L"..")) continue;    // dont want to show the . and ..

        
        char* utf8Name = WideToUtf8(ffd.cFileName);
        
        FileItem currentItem;
        currentItem.name = CreateString(utf8Name);
        currentItem.isFolder = (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        
        contents.entries[i] = currentItem;    // copied by value
        
        free(utf8Name);
        i++;
    }   while (FindNextFileW(hFind, &ffd) != 0);
    
    free(wideSearchPath);
    FindClose(hFind);
    
    return contents;
}

void DestroyDirectoryList(DirectoryList* directoryContents){
    if (directoryContents == nullptr) {PrntErrIsNULL; return;} 
    for (u64 i = 0; i < directoryContents->numEntries; i++){
        DestroyString(&directoryContents->entries[i].name); 
    }
    free(directoryContents->entries);
    directoryContents->capacity = 0;
    directoryContents->numEntries = 0;
}

wchar_t* Utf8ToWide(char* utf8, u64 extraCharCount){
    u64 len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);

    if (!len){
        printf("Error in MultiByteToWideChar. Error: %lu. String: %s\n", GetLastError(), utf8);
        return nullptr;
    }

    u64 totalLen = len + extraCharCount; 
    wchar_t* new_string = (wchar_t*) malloc(sizeof(wchar_t) * totalLen);
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, new_string, len); //! must be len, not totalLen

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

String CreateString(const char* strLiteral){
    String out_str;

    u64 len = strlen(strLiteral);
    out_str.capacity = 64;
    while (len >= out_str.capacity){
        out_str.capacity *= 2;
    }

    // this is going on the heap, not ro. data
    // So it technically is an array, technically..., not a string literal
    out_str.own_str = (char*)malloc(sizeof(char) * out_str.capacity);
    if (out_str.own_str == nullptr){
        printf("malloc failed for %s. capacity: %llu, length: %llu", strLiteral, out_str.capacity, len);
        free(out_str.own_str);
        return String{};
    }
    strcpy_s(out_str.own_str, out_str.capacity, strLiteral);
    out_str.length = len;
    return out_str;
}

void DestroyString(String* target){
    if (target == nullptr){
        PrntErrIsNULL;
        return;
    }
    if( target->own_str == nullptr){
        PrntErrIsNULL;
        return;
    }
    free(target->own_str);
    target->own_str = nullptr;
    target->capacity = 0;
    target->length = 0;
    target = nullptr;
}


void AppendSubDirectory(String* path, const String* subDir){
    u64 requiredLen = path->length + 1 + subDir->length ; // Total length needs to include backslash

    if (requiredLen >= path->capacity){
        while (requiredLen >= path->capacity){
            path->capacity *= 2;
        }

        char* buffer = (char*)realloc(path->own_str, sizeof(char) * path->capacity); 
        if (buffer == nullptr){ PrntErrIsNULL; return;}

        path->own_str = buffer;
    }
    strcat_s(path->own_str, path->capacity, "\\");
    strcat_s(path->own_str, path->capacity, subDir->own_str);

    path->length = requiredLen;

}

void PopPath(String* currentDir){
    u64 length = currentDir->length;

    if (currentDir->own_str[length-1] == ':' && currentDir->length <=3){
        printf("At root, so returning\n");
        return;  // at a root directory
    }

    // i represents position of last backslash
    i64 i;
    for (i = (i64) currentDir->length /*starts at null, hopefully...*/; i >= 0 && currentDir->own_str[i] != '\\'; i--);
    ZeroMemory(currentDir->own_str + i, currentDir->length - i);
    
    currentDir->length = i;
}

PathHistory initHistory(){
    PathHistory output;

    output.capacity = 64;
    
    output.count = 0;
    output.currentIndex = -1;
    
    output.visitedPaths = (String*) malloc(sizeof(String) * output.capacity);

    return output;

}

void AppendPathToHistory(const String subDir){
    u64 newCount = g_pathHistory.count + 1;

    if (newCount >= g_pathHistory.capacity){
        while (newCount >= g_pathHistory.capacity){
            g_pathHistory.capacity *= 2;
        }

        String* buffer = (String*) realloc(g_pathHistory.visitedPaths, sizeof(String) * g_pathHistory.capacity);
        if (buffer == nullptr){ PrntErrIsNULL; return;}

        g_pathHistory.visitedPaths = buffer;
    }
    

    g_pathHistory.currentIndex++;

    g_pathHistory.visitedPaths[g_pathHistory.currentIndex + 1] = subDir; 
    g_pathHistory.count = g_pathHistory.currentIndex + 1;

}


// todo free strings in pop history from path



