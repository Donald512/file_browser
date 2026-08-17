#include "BasicTypes.h"
#include "Item.h"
#include "Enum.h"
#include "Icons.h"

// class Directory{
//     public:
//     bool updatedChildren = false;
//     DirParent parent;
//     std::vector<DirChild> children;

//     void UpdateParent(PCIDLIST_ABSOLUTE parentPidl){
//         parent = GetDirParent(parentPidl);
//     }

//     void UpdateChildren(){
//         if (updatedChildren) return;
//         children = GetDirChildren(parent.shellFolder.Get(), parent.pidl.get());
//         updatedChildren = true;
//     }

// };

class Directory{
    public:
    bool updatedChildren = false;
    DirParent parent;
    DirChildren children;

    void UpdateParent(PCIDLIST_ABSOLUTE parentPidl){
        parent = GetDirParent(parentPidl);
    }

    void UpdateChildren(){
        if (updatedChildren) return;
        children = GetDirChildren2(parent.shellFolder.Get(), parent.pidl.get());
        updatedChildren = true;
    }

};
