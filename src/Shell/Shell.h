#pragma once
#include "Types.h"
#include <utility>
#include <ShlObj.h>
#include "icons.h"
#include "Str.h"

#pragma comment(lib, "Shell32.lib") 
#pragma comment(lib, "Shlwapi.lib") 
#pragma comment(lib, "Advapi32.lib") 

namespace WShell{

    bool PidlHasSubFolders(PCIDLIST_ABSOLUTE folder, bool accurate = false);
    class Pidl{
        public:
            Pidl() = default;
            Pidl(std::nullptr_t) : ptr(nullptr) {}
            explicit Pidl(PIDLIST_ABSOLUTE owned) : ptr(owned) {}
            explicit Pidl(PCIDLIST_ABSOLUTE unowned) : ptr(unowned ? ILClone(unowned) : nullptr) {}

            ~Pidl(){ if (ptr) ILFree(ptr); }

            // no copying — a pidl has one owner. Use Clone() if you need a duplicate.
            Pidl(const Pidl&) = delete;
            Pidl& operator=(const Pidl&) = delete;

            Pidl(Pidl&& other) noexcept : ptr(other.ptr) { other.ptr = nullptr; }
            Pidl& operator=(Pidl&& other) noexcept{
                if (this != &other){
                    if (ptr) ILFree(ptr);
                    ptr = other.ptr;
                    other.ptr = nullptr;
                }
                return *this;
            }

            explicit operator bool() const { return ptr != nullptr; }

            PCIDLIST_ABSOLUTE get() const { return ptr; }
            operator PCIDLIST_ABSOLUTE() const { return ptr; } // lets it be passed anywhere a raw pidl is expected
    
            PIDLIST_ABSOLUTE* GetAddressOf(){
                if (ptr){
                    ILFree((LPITEMIDLIST)ptr);
                    ptr = nullptr;
                }
                return &ptr;
            }

            Pidl Clone() const { return Pidl(ptr ? ILClone(ptr) : nullptr); }

        private:
            PIDLIST_ABSOLUTE ptr = nullptr;
    };

    enum class NewItemAction{
        Folder, 
        Shortcut, 
        EmptyFile, 
        FromTemplate
    };

    struct NewMenuItem{
        std::string displayName;    // E.g: Text document
        std::string extension;      // .txt
        Pidl templatePath;

        mutable u32 iconKeyCache = 0; 
        mutable bool iconKeyResolved = false;

        NewItemAction action = NewItemAction::EmptyFile;
        u32 IconKey() const {
            if (!iconKeyResolved){
                iconKeyCache = Icons::GetIconIndex(nullptr, Str::Utf8ToWide(extension.c_str()), FILE_ATTRIBUTE_NORMAL, SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
                iconKeyResolved = true;
            }
            return iconKeyCache;
        };
    };

    enum class FolderAccess {
        NoCreate,     // Hide "New" menu completely
        Restricted,   // Only show New Folder
        FullAccess    // Show full menu (cached ShellNew items)
    };

    enum class TriState{Unknown, True, False};

    struct Item{
        std::string name; // 24
        Pidl pidl;    // 8

        u64 fileSize = 0;   // 8
        FILETIME lastWriteTime{}; // 8
        SFGAOF attributes = 0;  // 4
        
        mutable u32 iconKeyCache = 0; 
        mutable bool iconKeyResolved = false;
        u32 IconKey() const {
            if (!iconKeyResolved){
                iconKeyCache = (u32) Icons::GetIconIndex(pidl.get(), nullptr, 0, SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
                iconKeyResolved = true;
            }
            return iconKeyCache;
        };
    };

    struct ItemLite{   // just a stripped down version of ShellItem
        std::string name; // 24
        Pidl pidl;    // 8

        mutable TriState subFolderState = TriState::Unknown;
        
        mutable u32 iconKeyCache = 0; 
        mutable bool iconKeyResolved = false;
        
        bool HasSubFolders() const { 
            if (subFolderState == TriState::Unknown){
                if (PidlHasSubFolders(pidl.get())){
                    subFolderState = TriState::True;
                }   else{
                    subFolderState = TriState::False;   
                }
            }
            return subFolderState == TriState::True;
        }

        u32 IconKey() const {
            if (!iconKeyResolved){
                iconKeyCache = (u32) Icons::GetIconIndex(pidl.get(), nullptr, 0, SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
                iconKeyResolved = true;
            }
            return iconKeyCache;
        };
    };

    enum class SortMode { Name, DateModified, Type, Size};
    enum class SortDirection {Ascending, Descending };

    class Directory{
        public:
            bool Load(PCIDLIST_ABSOLUTE folder);

            const std::vector<Item>& Items() const { return items; }
            FolderAccess Access() const { return access; }
            void SelectIndex(u64 i) {
                if (i < items.size()) selectedIndex = (i64) i;
            };
            u64 Selected() const {return (u64) selectedIndex; }

            void SetSort(SortMode mode, SortDirection dir){
                sortMode = mode;
                sortDirection = dir;
                ResortItems();  
            }

            void setShowHidden(bool show){
                showHidden = show;
            }

        private:
            i64 selectedIndex = -1;

            std::vector<Item> items;

            FolderAccess access = FolderAccess::NoCreate;

            SortMode sortMode = SortMode::Name;
            SortDirection sortDirection = SortDirection::Ascending;
            void ResortItems();

            bool showHidden = false;
    };
    
    // NOTE: Typeable means it cincludes the names of virtual folders
    std::vector<Item> EnumFolder(PCIDLIST_ABSOLUTE folder);
    std::vector<ItemLite> GetLiteItems(PCIDLIST_ABSOLUTE folder);
    bool ExecuteFile(PCIDLIST_ABSOLUTE file);
    Pidl TypeablePathToPidl(const wchar_t* widePath);
    std::string PidlToTypeablePath(PCIDLIST_ABSOLUTE pidl);
    FolderAccess GetFolderAccess(PCIDLIST_ABSOLUTE folder);
    std::vector<NewMenuItem> EnumerateNewMenu();
    std::vector<ItemLite> GetOneDriveAccounts();
    std::vector<ItemLite> GetSidebarItems(int category);


    // Resolves a well-known folder (This PC, Desktop, Recycle Bin, ...) to a Pidl.
    Pidl GetKnownFolderPidl(REFKNOWNFOLDERID folderID);
    Pidl GetKnownFolderPidl(const wchar_t* shellParsingGuid);

}

// Sidebar is sectioned in 3 parts, 
// 1 - SHCONTF_FOLDERS | SHCONTF_NAVIGATION_ENUM
// 2 - Pinned, Enumerate Home Quick access shell:::{679F85CB-0220-4080-B29B-5540CC05AAB6} 
//       EnumObjects(SHCONTF_FOLDERS)
// 3 - Will make it enumerate This PC, then Add Recycle Bin, and Control Panel

