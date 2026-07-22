#pragma once
#include "Types.h"
#include <utility>
#include <ShlObj.h>

#pragma comment(lib, "Shell32.lib") 
#pragma comment(lib, "Shlwapi.lib") 
#pragma comment(lib, "Advapi32.lib") 

namespace WShell{

    class Pidl{
        public:
            Pidl() = default;
            explicit Pidl(PIDLIST_ABSOLUTE owned) : ptr(owned) {}

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
        u64 iconIndex = 0;
        NewItemAction action = NewItemAction::EmptyFile;
    };

    enum class FolderAccess {
        NoCreate,     // Hide "New" menu completely
        Restricted,   // Only show New Folder
        FullAccess    // Show full menu (cached ShellNew items)
    };

    struct Item{
        std::string name; // 24
       WShell::Pidl pidl;    // 8

        u64 fileSize = 0;   // 8
        FILETIME lastWriteTime{}; // 8
        SFGAOF attributes = 0;  // 4
        u64 iconCacheKey = 0;
    };

    struct ItemLite{   // just a stripped down version of ShellItem
        std::string name; // 24
       WShell::Pidl pidl;    // 8
    };

    class Directory{
        public:
            bool Load(PCIDLIST_ABSOLUTE folder);

            const std::vector<Item>& Items() const { return items; }
            FolderAccess Access() const { return access; }
            void SelectIndex(u64 i) {
                if (i < items.size()){
                    selectedIndex = (i64) i;
                }
            };
            u64 Selected() const {return (u64) selectedIndex; }
            private:
            i64 selectedIndex = -1;
            std::vector<Item> items;
            FolderAccess access = FolderAccess::NoCreate;
    };
    
    // NOTE: Typeable means it cincludes the names of virtual folders
    std::vector<ItemLite> GetLiteItems(PCIDLIST_ABSOLUTE folder);
    bool ExecuteFile(PCIDLIST_ABSOLUTE file);
    Pidl TypeablePathToPidl(const wchar_t* widePath);
    std::string PidlToTypeablePath(PCIDLIST_ABSOLUTE pidl);
    bool PidlHasSubFolders(PCIDLIST_ABSOLUTE folder);
    FolderAccess GetFolderAccess(PCIDLIST_ABSOLUTE folder);
    std::vector<NewMenuItem> EnumerateNewMenu();

}