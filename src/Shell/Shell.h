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
    std::string GetPidlTypeName(PCIDLIST_ABSOLUTE pidl);
    u64 GetPidlFileSize(PCIDLIST_ABSOLUTE pidl);
    std::string FetchWindowsTooltip(PCIDLIST_ABSOLUTE pidl);
    std::string FetchTileViewLines(PCIDLIST_ABSOLUTE pidl);
    std::string FetchContentViewLines(PCIDLIST_ABSOLUTE pidl);
    
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
        NewItemAction action = NewItemAction::EmptyFile;

        Lazy<u32> iconKey;
        u32 IconKey() const {
            return iconKey.Get([&]{
                return (u32) Icons::GetIconIndex(nullptr, Str::Utf8ToWide(extension.c_str()), FILE_ATTRIBUTE_NORMAL, SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
            });
        }
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
        SFGAOF attributes = 0;  // 4
        FILETIME lastWriteTime{}; // 8
        
        mutable Lazy<u32> iconKey;
        u32 IconKey() const {
            return iconKey.Get([&]{
                return Icons::GetIconIndex(pidl.get(), nullptr, 0, SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
            });
        }
        
        mutable Lazy<u64> size;
        u64 Size() const {
            // handle cases like virtual items, or folders recursively later
            if (attributes & SFGAO_FOLDER) return 0ULL;
            return size.Get([&]{
                return GetPidlFileSize(pidl.get());
            });
        }
    
        mutable Lazy<std::string> typeName;
        std::string TypeName() const {
            // FALLBACK for Virtual items (e.g., "This PC", "Control Panel", "Recycle Bin")
            return typeName.Get([&]{
                return GetPidlTypeName(pidl.get());
            });


        }

        mutable Lazy<std::string> tooltipInfo;
        std::string TooltipInfo() const {
            return tooltipInfo.Get([&]{
                return FetchWindowsTooltip(pidl.get());
            });

        }

        mutable Lazy<std::string> tileViewInfo;
        std::string TileViewInfo() const {
            return tileViewInfo.Get([&]{
                return FetchTileViewLines(pidl.get());
            });

        }

        mutable Lazy<std::string> contentViewInfo;
        std::string ContentViewInfo() const {
            return contentViewInfo.Get([&]{
                return FetchContentViewLines(pidl.get());
            });

        }
    };

    struct ItemLite{   // just a stripped down version of ShellItem
        std::string name; // 24
        Pidl pidl;    // 8

        Lazy<u32> iconKey;
        u32 IconKey() const {
            return iconKey.Get([&]{
                return (u32) Icons::GetIconIndex(pidl.get(), nullptr, 0, SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
            });
        }

        Lazy<bool> hasSubFolders;
        bool HasSubFolders() const {
            return hasSubFolders.Get([&]{ return PidlHasSubFolders(pidl.get()); });
        }
    };

    enum class SortMode { Name, DateModified, Type, Size};
    enum class SortDirection {Ascending, Descending };

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

