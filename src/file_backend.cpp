// file_backend.cpp
#include "file_backend.h"
#include "string_helper.h"

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
    u64 searchPathLength;


    WIN32_FIND_DATAW ffd;
    // LARGE_INTEGER filesize;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    DWORD dwError = 0;

    wchar_t* wideSearchPath = Utf8ToWide(directoryPath.own_str, pathSuffixLength, &searchPathLength);
    StringCchCatW(wideSearchPath, searchPathLength, L"\\*");
    
    
    hFind = FindFirstFileW(wideSearchPath, &ffd);
    if (hFind == INVALID_HANDLE_VALUE){ 
        dwError = GetLastError();
        printf("%lu\n", dwError);
        printf("Returning empty directory\n");
        free(wideSearchPath);

        DirectoryList emptyContents = {0};
        emptyContents.entries = nullptr;
        return emptyContents;
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
    
    // malloc(0) returns a valid pointer so empty folders dont trigger the nullptr check
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
    directoryContents->entries = nullptr;
    directoryContents->capacity = 0;
    directoryContents->numEntries = 0;
}

