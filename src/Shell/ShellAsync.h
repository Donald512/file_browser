#pragma once
#include "AppContext.h"

// every function below runs the actual shell/com call on a worker thread and only touches the corresponding Item/ItemLite field on the main thread through Directory::PatchItem or WShell::PathByPid
// Render's code job is just: if (!item.someField.resolved && !item.someRequestSent), call the matching Request function here, and in the mean time draw whatever placeholder rect. 

namespace WShell::Async{
    // safe to call every frame - they no-op after the first call (via the matching *RequestSent flag on the item) until the field resolves. Render code should still check the flag itself before calling, to avoid the lambda-build overhead of a call that will bail out anywhere

    void RequestIcon(AppContext& ctx, const WShell::Item& item, size_t index);
    void RequestTooltip(AppContext& ctx, const WShell::Item& item, size_t index);
    void RequestMeta(AppContext& ctx, const WShell::Item& item, size_t index);    // Typename + Size - virtual items only
    void RequestTileInfo(AppContext& ctx, const WShell::Item& item, size_t index);
    
    // sidebar's icon + "has sub-folders" arrow use the exact same idea, but ItemLite doesn't live in a Directory, so no generation concept applies, so these take the specific vector the item lives in directly - a sidebar node's persistent children list, or one of ctx.items1/2/3. The vector must outlive the request; both of these cases do
    void RequestLiteIcon(AppContext& ctx, std::vector<WShell::ItemLite>& owner, WShell::ItemLite& item);
    void RequestHasSubFolders(AppContext& ctx, std::vector<WShell::ItemLite>& owner, WShell::ItemLite& item);

    // Populates ctx.items1/2/3 and ctx.NewMenuItems. All four run as independent jobs
    void RequestSidebarItems(AppContext& ctx);

    // 'New' menu's icons: NewMunuItem doesnt carry a pidl the way Item/ItemList do - its icon is looked up by file extnsion, so it cant go through PatchByPId, ctx.NewMenuItems is populated once at startup and not reassigned afterward, so matching result back to its item by extension string, rather than pidl, is safe here.
    void RequestNewMenuIcon(AppContext& ctx, WShell::NewMenuItem& item);

}

