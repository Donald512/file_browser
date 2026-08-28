#pragma once

#include "BasicTypes.h"
#include "Item.h"
#include "Enum.h"
#include "Icons.h"
#include "DirectoryManager.h"
#include "TypenameManager.h"

#include <algorithm>
#include <cstring>

enum class SortMode { Name, DateModified, Type, Size};
enum class SortDirection {Ascending, Descending };
enum class ViewMode { Icons, Small, List, Details, Tiles}; // feel like this belongs to  UI


struct FileViewState {
    // Change to user's last choice, or a setttings
    ViewMode viewMode = ViewMode::Details;
    f32 iconSize = 104.0f;
    SortMode sortMode = SortMode::Name;
    SortDirection sortDir = SortDirection::Ascending;
    bool showHidden = false;

    
    // UI Directives (The "Magic" variables)
    std::optional<u64> scrollToItemId = std::nullopt;
    std::optional<u64> renamingItemId = std::nullopt;
    float scrollY = 0.0f;
    
    f32 gridIconSize = 64.0f; 
};

class Directory{
    public:
    bool updatedChildren = false;
    DirParent parent;
    // std::shared_ptr<const DirChildren> children;    // delete
    CachedDirHandle HChildren;  

    void UpdateParent(PCIDLIST_ABSOLUTE parentPidl){
        parent = GetDirParent(parentPidl);
    }

    void UpdateChildren(DirectoryManager& directory, FileViewState vs);
    void ClearForNav();
    
    const std::vector<u32>& VisibleIndices(bool showHidden) const {
        return showHidden ? sortedIndices : nonHiddenIndices;
    }
    void RebuildNonHiddenIndices(const DirChildren& children);

    void Sort(const DirChildren& children, TypenameStore& typeStore, FileViewState vs);

    private:
        std::vector<u32> sortedIndices;
        std::vector<u32> nonHiddenIndices;

};

inline void Directory::UpdateChildren(DirectoryManager& dirManager, FileViewState vs){
    if (updatedChildren) return;

    UpdateParentShellFolder(parent);
    HChildren = dirManager.GetOrRequest(parent.pidl.get(), parent.hash);

    const DirChildren* PChildren = dirManager.Get(HChildren);
    if (!PChildren) return;

    size_t count = PChildren->ItemCount();
    sortedIndices.resize(count);

    for(u32 i = 0; i < count; i++){
        sortedIndices[i] = i;
    }

    Sort(*PChildren, dirManager.GetTypeStore(), vs);

    if (!vs.showHidden) RebuildNonHiddenIndices(*PChildren);
    
    updatedChildren = true;
}

inline void Directory::ClearForNav(){
    // children = nullptr;  // not needed, i think
    sortedIndices.clear();
    nonHiddenIndices.clear();
    updatedChildren = false;
}

inline void Directory::RebuildNonHiddenIndices(const DirChildren& children){
    nonHiddenIndices.clear();
    nonHiddenIndices.reserve(sortedIndices.size());

    for (u32 index : sortedIndices) {
        if (!(children.attributes[index] & SFGAO_HIDDEN)) nonHiddenIndices.push_back(index);
        // std::cout << children->GetChildName(index) << " attributes: " << children->attributes[index] << std::endl;
    }
}

inline void Directory::Sort(const DirChildren& children, TypenameStore& typeStore, FileViewState vs) {
    if (sortedIndices.empty()) return;
    std::sort(sortedIndices.begin(), sortedIndices.end(), 
        [this, children, vs, &typeStore](u32 idxA, u32 idxB) {
            // Grab lightweight views for both items being compared
            auto a = children.GetItem(idxA, typeStore);
            auto b = children.GetItem(idxB, typeStore);

            // Folders always stay at the top
            if (a.IsFolder() != b.IsFolder()) {
                return a.IsFolder(); 
            }

            // Primary sort criteria
            int cmp = 0;
            switch (vs.sortMode) {
                case SortMode::Name:
                    cmp = _stricmp(a.name, b.name);
                    break;

                case SortMode::DateModified:
                    cmp = ::CompareFileTime(&a.lastWriteTime, &b.lastWriteTime);
                    break;

                case SortMode::Type:
                    cmp = _stricmp(a.typeName, b.typeName); 
                    break;

                case SortMode::Size:
                    if (a.size < b.size) cmp = -1;
                    else if (a.size > b.size) cmp = 1;
                    break;
            }

            // 3. Tie-breaker
            if (cmp == 0) {
                cmp = _stricmp(a.name, b.name);
            }

            // 4. Direction
            if (vs.sortDir == SortDirection::Descending) {
                return cmp > 0;
            }
            return cmp < 0;
        }
    );
}

