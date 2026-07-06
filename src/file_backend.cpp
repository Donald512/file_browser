// file_backend.cpp
#include "file_backend.h"
#include "string_helper.h"

DirectoryList GetDirectoryContents(const String directoryPath){

    // todo: Implement FindFirstFileExW, with FindExInfoBasic and FIND_FIRST_EX_LARGE_FETCH


    printf("%s\n", directoryPath.own_str);
    // Check that the input path plus 3 is not longer than MAX_PATH.
    // Three characters are for the "\*" plus NULL appended below.
    if (directoryPath.length > MAX_PATH - 3){
        printf("Directory path is too long: %s, length: %llu\n", directoryPath.own_str, directoryPath.length);
        assert(0);
    }

    DirectoryList contents;
    u64 pathSuffixLength = 3;  // for \*NULL
    u64 searchPathLength;

    WIN32_FIND_DATAW ffd;
    // LARGE_INTEGER filesize;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    DWORD dwError = 0;

    wchar_t* wideSearchPath = Utf8ToWide(directoryPath.own_str, pathSuffixLength, &searchPathLength);
    StringCchCatW(wideSearchPath, searchPathLength, L"\\*");
    
    
    contents.capacity = 64;
    contents.entries = (FileItem*) malloc(sizeof(FileItem) * contents.capacity);
    if (contents.entries == nullptr){
        PrntErrIsNULL;
        DestroyDirectoryList(&contents);
        return contents;

    }
    
    
    // hFind = FindFirstFileW(wideSearchPath, &ffd);
    hFind = FindFirstFileExW(wideSearchPath, FindExInfoBasic, &ffd, FindExSearchNameMatch, nullptr,  FIND_FIRST_EX_LARGE_FETCH);
    u64 i = 0;
    do{
        if (hFind == INVALID_HANDLE_VALUE) break;
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) continue; // its a fake folder and will break the code
        if (!wcscmp(ffd.cFileName, L".") || !wcscmp(ffd.cFileName, L"..")) continue;    // dont want to show the . and ..

        if (i >= contents.capacity){
            while (i >= contents.capacity){
                contents.capacity *= 2;
            }
            contents.entries = (FileItem*) realloc(contents.entries, sizeof(FileItem) * contents.capacity);
        }
        
        char* utf8Name = WideToUtf8(ffd.cFileName);
        
        contents.entries[i].name = CreateString(utf8Name);
        contents.entries[i].isFolder = (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        
        
        free(utf8Name);
        i++;
    }   while (FindNextFileW(hFind, &ffd) != 0);
    contents.numEntries = i;
    
    free(wideSearchPath);
    
    if (hFind == INVALID_HANDLE_VALUE){ 
        dwError = GetLastError();
        printf("%lu\n", dwError);
        printf("Returning empty directory\n");
        
        DestroyDirectoryList(&contents);
        return contents;
    }   
    
    FindClose(hFind);
    return contents;
}

void DestroyDirectoryList(DirectoryList* directoryContents){
    if (directoryContents == nullptr) {PrntErrIsNULL; return;} 
    for (u64 i = 0; i < directoryContents->numEntries; i++){
        DestroyString(&directoryContents->entries[i].name); 
    }
    free(directoryContents->entries);
    directoryContents->entries = nullptr;
    directoryContents->capacity = 0;
    directoryContents->numEntries = 0;
}

