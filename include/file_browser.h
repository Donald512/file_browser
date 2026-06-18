#pragma once

#include <Windows.h>
#include <string>
#include <strsafe.h>
#include <tchar.h>
#include "imgui.h"
// #include "imgui_boilerplate.h"


#pragma comment(lib, "user32.lib")

using f32 = float;
using u16 = uint16_t;
using u64 = uint64_t;
using i64 = int64_t;
using u8 = uint8_t;

struct String{
    char* str;
    u64 length;
    u64 capacity;
};

struct FileItem{
    String* name;
    bool isFolder;
};

struct directory_list{
    FileItem* start_of_file_items;
    u64 num_items;
};

struct PathHistory {
    String* paths;  // Array of strings
    u64 capacity;   // Max size of the array E.g: 64 strings
    u64 count;      // How many paths are  currently valid
    i64 current_index;  // Where the current directory is in history
};

//

directory_list* ReturnFilesInDir(String &dirName);
// void RenderFileExplorer(directory_list* dir_list);
void RenderMainInterface(directory_list* dir_list);
void RenderFileGrid(directory_list* dir_list);
HWND CreateMyOSWindow();
void FreeDirectoryList(directory_list* dir_list);
wchar_t* Utf8ToWide(char* utf8);
char* WideToUtf8(wchar_t* wide);
void createString(String &string, const char* str_literal);
void joinDirs(String &dest, String &src);
void goToParentDir(String &currentDir);

extern ImVec4 g_clear_color;