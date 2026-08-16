#pragma once

#include <ShlObj.h>
#include "BasicTypes.h"
#include "Shell.h"
#include <vector>
#include "Lazy.h"
#include <string>
#include "IconManager.h"


struct DirItem{
    std::string name; // 24
    WShell::Pidl pidl;    // 8
    u64 hash = 0;   // store Hash -  ID used to match item, shared_ptr/unique_ptr is slow (cache misses) and using ptr is risky because things can be ordered 
    SFGAOF attributes = 0;  // 4
    FILETIME lastWriteTime{}; // 8
    u64 size = 0;
    std::string typeName{};  // Gonna change to a u64, since many of them have similar typenames
};

/*
// Convert the std::vector to Arena
struct EverythingBackend{
    std::vector<u64> FRN;   // Aight no FRN, since it requires opening and closing each file to get, and reading the MFT requires Admin, which will scare users
    std::vector<u32> parentIndex;
    std::vector<u32> nameOffset;
    std::vector<u32> nameLen;
    std::vector<u64> size;

    std::vector<u32> Created;   // for 4:01 PM 01/1/2921
    std::vector<u32> Modified;   // this a very heavy struct

    std::vector<u16> typeNameIds;
    std::vector<Flags> attributes;

    std::vector<char> NameBuffer;
    std::vector<bool> NeedsRefresh;
}
*/

/*
New Tab on This PC:
Try FindFirstFile:
    it fails
Use IShellFolder::EnumObjects()
    See C:
        Try FindFirstFile on C:
            It works
                Add C: To EverythingBackend
                Fill Created, Modified, Etc
                
    See PowerPoint Network Document:
        Try FindFirstFile on PowerPoint Network Document:
            It works
                Add To EverythingBackend
                Fill Created, Modified, Etc

    User Chills around, background indexer with low priority starts indexing
    
    Every folder it enumerates, it stores a bool or index, or number, indicating where it stopped
  
    what happens if the user has only one thread, anyways
    User has 7 folders in view, background indexer kicks off for those 7

    indexer closes one of the work it was doing 9 levels deep, but the user has scrolled
    save stop and start indexing for the folders currently in view

    icl im just gonna enjoy the rest of my summer, fuck this project 3 months wasted

    User Clicks C:
    Do we check our backend, or use FindFirstFile, bevause rn, theres no way to maintain an accurate snapshot of our everything backend, 2 choices, build very stable Everything backend, and every file enumeration reads everything backend, or just use FindFirstFile, and hope for the best, but it gets slow and even fails for recycle bin, and at that point i am basicaly implementing File Pilot will more bugs
    
        Enumerate with FindFirstFile and cross reference with C:'s list of children. 
        See Autodesk        
        See Everything else                
        See Hasleo                
        See inetpub                
        See msys64                
        See OneDriveTemp                
        See page                
        See PerfLogs                
        See Windows    

        Some fields are already filled
            Show what 

User clicks Parent at C:
    Since thats physical root, try getting C:'s Pidl, and do ILParent
    See This PC, user clicks parent again, go to desktop
    and in enumerating everything, we see nothing is physical, so we dont get any information from our backend or enumerate
            
        



    
*/
