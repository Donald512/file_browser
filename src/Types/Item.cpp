#include "Item.h"
#include "TypenameManager.h"

ItemView DirChildren::GetItem(size_t index, TypenameStore& typeStore) const {
    return ItemView{
        GetChildName(index),
        typeStore.GetTypename(typenameIndex[index]),
        GetChildPidl(index),
        hashes[index],
        attributes[index],
        lastWriteTimes[index],
        sizes[index]
    };
}