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
class Directory{
    public:
    bool updatedChildren = false;
    DirParent parent;
    std::shared_ptr<const DirChildren> children;

    void UpdateParent(PCIDLIST_ABSOLUTE parentPidl){
        parent = GetDirParent(parentPidl);
    }

    void UpdateChildren(DirectoryManager& directory, TypenameStore& typeStore, SortMode sm, SortDirection sd, bool showHidden);

    void RebuildNonHiddenIndices();

    const std::vector<u32>& VisibleIndices(bool showHidden) const {
        return showHidden ? sortedIndices : nonHiddenIndices;
    }

    void Sort(TypenameStore& typeStore, SortMode mode, SortDirection direction);

    private:
        std::vector<u32> sortedIndices;
        std::vector<u32> nonHiddenIndices;

};

inline void Directory::UpdateChildren(DirectoryManager& directory, TypenameStore& typeStore, SortMode sm, SortDirection sd, bool showHidden){
    if (updatedChildren) return;

    UpdateParentShellFolder(parent);
    children = directory.GetOrRequest(parent.shellFolder.Get(), typeStore, parent.pidl.get(), parent.hash);

    size_t count = children->ItemCount();
    sortedIndices.resize(count);

    for(u32 i = 0; i < count; i++){
        sortedIndices[i] = i;
    }

    Sort(typeStore, sm, sd);

    if (!showHidden){
        RebuildNonHiddenIndices();
    }
    
    updatedChildren = true;
}

inline void Directory::RebuildNonHiddenIndices(){
    nonHiddenIndices.clear();
    nonHiddenIndices.reserve(sortedIndices.size());

    for (u32 index : sortedIndices) {
        if (!(children->attributes[index] & SFGAO_HIDDEN))
            nonHiddenIndices.push_back(index);
        // std::cout << children->GetChildName(index) << " attributes: " << children->attributes[index] << std::endl;
    }
}

inline void Directory::Sort(TypenameStore& typeStore, SortMode mode, SortDirection direction) {
    if (!children || sortedIndices.empty()) return;

    std::sort(sortedIndices.begin(), sortedIndices.end(), 
        [this, mode, direction, &typeStore](u32 idxA, u32 idxB) {
            // Grab lightweight views for both items being compared
            auto a = children->GetItem(idxA, typeStore);
            auto b = children->GetItem(idxB, typeStore);

            // Folders always stay at the top
            if (a.IsFolder() != b.IsFolder()) {
                return a.IsFolder(); 
            }

            // Primary sort criteria
            int cmp = 0;
            switch (mode) {
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
            if (direction == SortDirection::Descending) {
                return cmp > 0;
            }
            return cmp < 0;
        }
    );
}

