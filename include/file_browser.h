#pragma once

#include <Windows.h>
#include <string>
#include <strsafe.h>
#include <tchar.h>
#include "imgui.h"
// #include "imgui_boilerplate.h"

// todo prefix variables used by Windows Wide function by tmp_
#pragma comment(lib, "user32.lib")

using f32 = float;
using u16 = uint16_t;
using u64 = uint64_t;
using i64 = int64_t;
using u8 = uint8_t;

#define DO_NOTHING 0
#define MOVE_BACKWARD 1
#define MOVE_FORWARD 2
#define NEW_FOLDER 3

#define STRING_DEFAULT_CAPACITY 64

#define PrntErr(msg) printf("Error: %s, File: %s, Line: %u\n", msg, __FILE__, __LINE__)
#define PrntErrIsNULL printf("Already NULL, File: %s, Line: %u\n", __FILE__, __LINE__)


struct String{
    char* own_str;
    u64 length;
    u64 capacity;
};


struct FileItem{
    String name;
    bool isFolder;
};

struct DirectoryList{
    FileItem* entries;  // array of FileItems
    u64 numEntries;
    u64 capacity;
};

struct PathHistory {
    String* visitedPaths;  // Array of strings // todo am i storing Strings or pointer to Strings, which is better, if a string relocates, i need to store the new pointer 
    u64 capacity;   // Max size of the array E.g: 64 strings
    u64 count;      // How many paths are currently valid
    i64 currentIndex;  // Where the current directory is in history
};

//

DirectoryList GetDirectoryContents(const String directoryPath);
void DestroyDirectoryList(DirectoryList* directoryContents);

wchar_t* Utf8ToWide(char* utf8, u64 extraCharCount = 0);
char* WideToUtf8(wchar_t* wide);

String CreateString(const char* view_str_literal);
void DestroyString(String* target);

void AppendSubDirectory(String* path, const String* subDir);
void PopPath(String* currentDir);

void RenderMainInterface(DirectoryList* dir_list);
void RenderFileGrid(DirectoryList* dir_list);

HWND CreateMyOSWindow();

extern ImVec4 g_clear_color;

String g_currentDir;
DirectoryList g_currentDirList;
PathHistory g_pathHistory;


