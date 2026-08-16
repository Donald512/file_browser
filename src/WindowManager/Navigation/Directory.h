#include "BasicTypes.h"
#include "Item.h"
#include "Enum.h"
#include "Icons.h"



class Directory{
    public:
    bool updatedChildren = false;
    DirParent parent;
    std::vector<DirChild> children;

    void UpdateParent(PCIDLIST_ABSOLUTE parentPidl){
        parent = GetDirParent(parentPidl);
    }

    void UpdateChildren(){
        if (updatedChildren) return;
        children = GetDirChildren(parent.shellFolder.Get());
        updatedChildren = true;
    }

    u64 ComputeChildHash(DirChild child){
        return child.Hash(parent.pidl.get());
    }
    
};

