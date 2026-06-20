#include "history_helper.h"
#include "string_helper.h"



PathHistory InitHistory(){
    PathHistory output;

    output.capacity = 64;
    
    output.count = 0;
    output.currentIndex = -1;
    
    output.visitedPaths = (String*) malloc(sizeof(String) * output.capacity);
    if (output.visitedPaths == nullptr){
        PrntErrIsNULL;
        return PathHistory{};
    }

    return output;

}

bool AppendPathToHistory(const String subDir){
    u64 newCount = g_pathHistory.count + 1;

    if (!g_pathHistory.capacity) {PrntErrInvalid; return false;}
    if (newCount >= g_pathHistory.capacity){
        while (newCount >= g_pathHistory.capacity){
            g_pathHistory.capacity *= 2;
        }

        String* buffer = (String*) realloc(g_pathHistory.visitedPaths, sizeof(String) * g_pathHistory.capacity);
        if (buffer == nullptr){ PrntErrIsNULL; return false;}

        g_pathHistory.visitedPaths = buffer;
    }
    

    g_pathHistory.currentIndex++;
    // Deep copy is mandatory here. A plain assignment copies the raw char* pointer
    g_pathHistory.visitedPaths[g_pathHistory.currentIndex] = CloneString(subDir); 
    g_pathHistory.count++;

    // Sync global active directory
    UpdateCurrentDir();
    return true;
}


bool PopPathHistoryFromIndex(u64 cursorIndex){  // inclusive
    // Only has to destroy the strings, not Zero them, or else they would leak
    if (cursorIndex > g_pathHistory.count){ PrntErrInvalid; return false;   }    // todo check if > or >= 

    for (u64 i = cursorIndex; i < g_pathHistory.count ; i++){
        DestroyString(&g_pathHistory.visitedPaths[i]);
    }
    g_pathHistory.currentIndex = (i64) cursorIndex - 1;
    g_pathHistory.count = cursorIndex;
    UpdateCurrentDir();
    return true;
}


bool NavigateBackward(){
    if (g_pathHistory.currentIndex > 0){
        g_pathHistory.currentIndex--;
    }
    else{
        PrntErrInvalid;
        return false;
    }
    UpdateCurrentDir();
    return true;
}

bool NavigateForward(){
    if (g_pathHistory.currentIndex + 1 < (i64) g_pathHistory.count){
        g_pathHistory.currentIndex++;
    }
    else{
        PrntErrInvalid;
        return false;
    }
    UpdateCurrentDir();
    return true;
}

bool NewBranch(const String subDir){
    if (PopPathHistoryFromIndex(g_pathHistory.currentIndex + 1) && AppendPathToHistory(subDir)){
        UpdateCurrentDir();
        return true;
    }
    return false;
}



void UpdateCurrentDir(){
    DestroyString(&g_currentDir);
    g_currentDir = CloneString (g_pathHistory.visitedPaths[g_pathHistory.currentIndex]);

}