// navigation.cpp

#include "core.h"

namespace Navigation{

    bool NavigateTo(AppContext& ctx, PIDLIST_ABSOLUTE newPidl){
        if (!newPidl) return false;
        // change currentPidl, currentDirList, append new path to history
        if (ILIsEqual(newPidl, ctx.currentFolderPidl)){
            return false;
        }
        if (ctx.currentFolderPidl){
            Utils::FreePidl(ctx.currentFolderPidl);
        }
        ctx.currentFolderPidl = ILClone(newPidl);

        Backend::EnumerateDirectory(ctx, ctx.currentFolderPidl);
        Backend::GenerateBreadcrumbs(ctx, ctx.currentFolderPidl);
        return true;
    }
    bool CanGoBack(AppContext& ctx){
        return ctx.history.currentIndex > 0;
    }
    
    bool CanGoForward(AppContext& ctx){
        return ctx.history.currentIndex + 1 < (i64) ctx.history.count;
    }

    bool CanGoParent(AppContext& ctx){
        if (!ctx.currentFolderPidl || ILIsEmpty(ctx.currentFolderPidl)){
            return false;
        }
        return true;
    }

    bool GoBack(AppContext& ctx){
        if (CanGoBack(ctx)){
            ctx.history.currentIndex--;
            Navigation::NavigateTo(ctx, ctx.history.visitedPidls[ctx.history.currentIndex]);
            return true;
        }
        return false;
    }

    bool GoForward(AppContext& ctx){
        if (CanGoForward(ctx)){
            ctx.history.currentIndex++;
            Navigation::NavigateTo(ctx, ctx.history.visitedPidls[ctx.history.currentIndex]);
            return true;
        }
        return false;
    }

    bool GoParent(AppContext& ctx){
        if (!CanGoParent(ctx)){
            return false;
        }
        PIDLIST_ABSOLUTE parentPidl = ILClone(ctx.currentFolderPidl);
        if (!parentPidl) return false;
        ILRemoveLastID(parentPidl);

        if (NavigateTo(ctx, parentPidl)){
            History::Append(ctx, parentPidl);
        }

        Utils::FreePidl(parentPidl);
        return true;
    }


} // namespace Navigation