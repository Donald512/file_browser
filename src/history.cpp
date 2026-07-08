// history_helper.cpp
#include "core.h"


namespace History{
    void Init(AppContext& ctx){
        ctx.history.capacity =  32;
        ctx.history.count = 0;
        ctx.history.currentIndex  = -1;
        ctx.history.visitedPidls = (PIDLIST_ABSOLUTE*) malloc(sizeof(PIDLIST_ABSOLUTE) * ctx.history.capacity);
    }
    
    bool Append(AppContext& ctx, PIDLIST_ABSOLUTE targetPidl){

        if (!ctx.history.capacity) return false;

        if (ctx.history.count + 1 >= ctx.history.capacity){
            ctx.history.capacity *= 2;
            ctx.history.visitedPidls = (PIDLIST_ABSOLUTE*) realloc( ctx.history.visitedPidls, sizeof(PIDLIST_ABSOLUTE) * ctx.history.capacity);
        }

        ctx.history.currentIndex++;
        ctx.history.visitedPidls[ctx.history.currentIndex] = ILClone(targetPidl);
        ctx.history.count++;

        return true;
    }



    void Destroy(AppContext& ctx){
        for (u64 i = 0; i < ctx.history.count; i++){
            Utils::FreePidl(ctx.history.visitedPidls[i]);
        }
        ctx.history.visitedPidls = nullptr;
        ctx.history.capacity = 0; 
        ctx.history.count = 0; 
    }
}

