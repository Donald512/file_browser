#pragma once
#include "BasicTypes.h"
#include "macros.h"

struct Arena{
    u64 offset = 0;     // current offset in arena
    u64 committed = 0;       // current commited bytes
    u64 capacity = 0;   // total reserved bytes
    u64 pageSize = 0;
    char* buffer = nullptr;
};

bool ArenaInit(Arena* arena, u64 _cap);
void* ArenaPush(Arena* arena, u64 numElem, u64 elemSize, u64 alignSize);

inline void ArenaReset(Arena* arena){
    arena->offset = 0;     // does not decommit
}

struct TempArena{
    Arena* arena;
    u64 savedOffset;
};

internal inline TempArena TempBegin(Arena* arena){
    return TempArena{.arena = arena, .savedOffset = arena->offset};
}

internal inline void TempEnd(TempArena temp){
    temp.arena->offset = temp.savedOffset;
} 