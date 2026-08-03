#pragma once
#include "Types.h"
#include "icons.h"
#include <ShlObj.h>
#include <string>
#include <vector>
#include "ShellPidl.h"
#include "Str.h"

namespace WShell{
    bool PidlHasSubFolders(PCIDLIST_ABSOLUTE folder, bool accurate = false);
    std::string GetPidlTypeName(PCIDLIST_ABSOLUTE pidl);
    u64 GetPidlFileSize(PCIDLIST_ABSOLUTE pidl);
    std::string FetchWindowsTooltip(PCIDLIST_ABSOLUTE pidl);
    std::string FetchTileViewLines(PCIDLIST_ABSOLUTE pidl);
    std::string FetchContentViewLines(PCIDLIST_ABSOLUTE pidl);
    
    enum class NewItemAction{
        Folder, 
        Shortcut, 
        EmptyFile, 
        FromTemplate
    };


    enum class FolderAccess {
        NoCreate,     // Hide "New" menu completely
        Restricted,   // Only show New Folder
        FullAccess    // Show full menu (cached ShellNew items)
    };

    enum class TriState{Unknown, True, False};

    enum class SortMode { Name, DateModified, Type, Size};
    enum class SortDirection {Ascending, Descending };

    struct ContextMenuItem {
        std::string text;
        std::string verb;
        UINT id;            // The offset ID to invoke the command later
        bool isSeparator = false;
        bool enabled = true;
        bool checked = false;

        ComPtr<ID3D11ShaderResourceView> srv; 
        ImTextureID hIconTex{};      

        std::vector<ContextMenuItem> subItems;  // for nested menus
    };


    struct Item{
        std::string name; // 24
        Pidl pidl;    // 8
        u64 hash = 0;   // store Hash -  ID used to match item, shared_ptr/unique_ptr is slow (cache misses) and using ptr is risky because things can be ordered 
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

        // mutable Lazy<std::string> contentViewInfo;
        // std::string ContentViewInfo() const {
        //     return contentViewInfo.Get([&]{
        //         return FetchContentViewLines(pidl.get());
        //     });

        // }
        // these track if an async request for this field is already in flight, checked by the render code before firing another one. Without these, a visible row would re-enque a fresh job for the same action, every frame until the result comes back 
        mutable bool iconRequestSent = false;
        mutable bool tooltipRequestSent = false;
        mutable bool metaRequestSent = false;   // Covers typename + size together (virtual items only)
        mutable bool tileInfoRequestSent = false;
    };

    struct ItemLite{   // just a stripped down version of ShellItem
        std::string name; // 24
        Pidl pidl;    // 8
        u64 hash = 0;

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
        mutable bool iconRequestSent = false;
        mutable bool hasSubFoldersRequestSent = false;
    };

    struct NewMenuItem{
        std::string displayName;    // E.g: Text document
        std::string extension;      // .txt
        Pidl templatePath;
        NewItemAction action = NewItemAction::EmptyFile;

        Lazy<u32> iconKey;
        u32 IconKey() const {
            return iconKey.Get([&]{
                wchar_t* wide = Str::Utf8ToWide(extension.c_str());
                u32 idx = Icons::GetIconIndex(nullptr, wide, FILE_ATTRIBUTE_NORMAL, SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
                free(wide);
                return idx;
            });
        }
        mutable bool iconRequestSent = false;
    };


}