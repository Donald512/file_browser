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


#define PrntErr(msg) printf("Error: %s, File: %s, Line: %u\n", msg, __FILE__, __LINE__)
#define PrntErrIsNULL printf("Already NULL, File: %s, Line: %u\n", __FILE__, __LINE__)
#define PrntErrInvalid printf("Invalid move, File: %s, Line: %u\n", __FILE__, __LINE__)


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
    String* visitedPaths;  // Array of strings
    u64 capacity;   // Max size of the array E.g: 64 strings
    u64 count;      // How many paths are currently valid
    i64 currentIndex;  // Where the current directory is in history
};

//

void UpdateCurrentDir();


extern ImVec4 g_clear_color;

extern String g_currentDir;
extern DirectoryList g_currentDirList;
extern PathHistory g_pathHistory;



