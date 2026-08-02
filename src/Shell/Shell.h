#pragma once
#include "Types.h"
#include <utility>
#include <ShlObj.h>
#include "icons.h"
#include "Str.h"

#include "ShellPidl.h"
#include "ShellItems.h"

#pragma comment(lib, "Shell32.lib") 
#pragma comment(lib, "Shlwapi.lib") 
#pragma comment(lib, "Advapi32.lib")


namespace WShell{

    // A stable identity for a pidl based on its content, not its address. Pure in-memory hashing - ILGetSize() walks the linked SHITEMID structure summing  sizes, no shell/COM/IO calls involved - so this is cheap enough to call every frame and safe to call on any thread, Used as a key in std::unordered_map 
    u64 HashPidl(PCIDLIST_ABSOLUTE pidl);

    // Searches 'items' for the one whose pidl matches 'pidl' by content and if found, calls apply(item)- otherwise, does nothing. 
    // td check if looping through items is neccessary
    // NOTE: This exists because of a side effect of asynchronous programming, if fileview pushes a request to get the iconKey, but someone clicks sort and the ordering changes, it gives the wrong item a wrong Pidl
    // but using ILIsEqual to compare 5000 items everytime we need the iconIndex is a slow API call, so compare by a hash they each hold 
    template <typename TCollection, typename  TApply>
    bool PatchByHash(TCollection& items, PCIDLIST_ABSOLUTE pidl, u64 hash, size_t hintIndex, TApply&& apply){
        // O(1) try
        // if hintIndex is 0, doesnt matter, just 1 check, could mean a valid index or just random 
        if (hintIndex < items.size() && items[hintIndex].hash == hash){
            apply(items[hintIndex]);
            return true;
        }
        // Fallback to O(N) 
        for (auto& item: items){
            if (item.hash == hash){
                // for the quintillionth chance of a hash collision
                if (ILIsEqual(item.pidl.get(), pidl)){
                    apply(item);
                    return true;
                }
            }
        }
        return false;
    }

    class Directory{
        public:
            bool Load(PCIDLIST_ABSOLUTE folder);

            const std::vector<Item>& Items() const { return items; }
            FolderAccess Access() const { return access; }
            void SelectIndex(i64 i) {
                if (i < (i64)items.size()) selectedIndex = i;
            };
            u64 Selected() const {return (u64) selectedIndex; }

            void SetSort(SortMode mode, SortDirection dir){
                if (sortMode != mode || dir != sortDirection){    
                    sortMode = mode;
                    sortDirection = dir;
                    ResortItems();  
                }
            }

            SortMode GetSort(){
                return sortMode;
            }
            SortDirection GetSortDir(){
                return sortDirection;
            }

            bool GetShowHidden() const { return showHidden; }

            void SetShowHidden(bool show){
                showHidden = show;
            }

            // tags this directory with the navigation generation it was loaded for, set when the background load's result is applied. Lets PatchItem() below reject a per-item asysnc result.
            void SetLoadGeneration(u64 g) {loadGeneration = g; }
            u64 Generation() const {return loadGeneration;}

            // forGeneration is whatever Generation() returned at the moment the request was fired, if the directory has been replaced by  a newer navigation, the generations wont match, and result is dropped without scanning items for the matching pidl 
            // never call this anywhere except a RunAsync onDone callback, it mutates item directly and assumes its on the main thread

            template <typename TApply>
            bool PatchItem(PCIDLIST_ABSOLUTE targetPidl, u64 targetHash, u64 hintIndex, u64 forGeneration, TApply&& apply){
                if (forGeneration != loadGeneration) return false;
                return PatchByHash(items, targetPidl, targetHash, hintIndex, std::forward<TApply>(apply));
            }
        private:
            i64 selectedIndex = -1;

            std::vector<Item> items;

            FolderAccess access = FolderAccess::NoCreate;

            SortMode sortMode = SortMode::Name;
            SortDirection sortDirection = SortDirection::Ascending;
            void ResortItems();

            bool showHidden = false;
            u64 loadGeneration = 0;
    };
    
    // NOTE: Typeable means it includes the names of virtual folders
    std::vector<Item> EnumFolder(PCIDLIST_ABSOLUTE folder);
    std::vector<ItemLite> GetLiteItems(PCIDLIST_ABSOLUTE folder);
    bool ExecuteFile(PCIDLIST_ABSOLUTE file);
    Pidl TypeablePathToPidl(const wchar_t* widePath);
    std::string PidlToTypeablePath(PCIDLIST_ABSOLUTE pidl);
    FolderAccess GetFolderAccess(PCIDLIST_ABSOLUTE folder);
    std::vector<NewMenuItem> EnumerateNewMenu();
    std::vector<ItemLite> GetOneDriveAccounts();
    std::vector<ItemLite> GetSidebarItems(int category);
    void FileTime(const FILETIME& ft, char* outBuf, int outBufSize);
    void Size(u64 sizeInBytes, char* outBuf, int outBufSize);


    // Resolves a well-known folder (This PC, Desktop, Recycle Bin, ...) to a Pidl.
    Pidl GetKnownFolderPidl(REFKNOWNFOLDERID folderID);
    Pidl GetKnownFolderPidl(const wchar_t* shellParsingGuid);

}

// Sidebar is sectioned in 3 parts, 
// 1 - SHCONTF_FOLDERS | SHCONTF_NAVIGATION_ENUM
// 2 - Pinned, Enumerate Home Quick access shell:::{679F85CB-0220-4080-B29B-5540CC05AAB6} 
//       EnumObjects(SHCONTF_FOLDERS)
// 3 - Will make it enumerate This PC, then Add Recycle Bin, and Control Panel

