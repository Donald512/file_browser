#include "ShellAsync.h"
#include "Str.h"

using namespace WShell;

void WShell::Async::RequestIcon(AppContext& ctx, const Item& item, size_t index){
    item.iconRequestSent = true;
    u64 gen = ctx.navigation.Contents().Generation();
    u64 targetHash = item.hash;

    ctx.tasks.RunAsync(
        // worker thread: only toouch its clone of the pidl
        [pidl = item.pidl.Clone()]() mutable {
            // check what add_overlays does
            // the SHGFI_SMALLICON here, does not matter, it is just there for iconIndex, only matters when you retrieve the IconTexture
            u32 iIcon = Icons::GetIconIndex(pidl.get(), nullptr, 0, SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
            return std::make_pair(std::move(pidl), iIcon);
        },
        // main thread
        [&ctx, gen, targetHash, index](std::pair<Pidl, u32> result) mutable {
            ctx.navigation.Contents().PatchItem(result.first.get(), targetHash, index, gen, [&](Item& it){
                it.iconKey.value = result.second;
                it.iconKey.resolved = true;
            });
        }
    );
}


void WShell::Async::RequestTooltip(AppContext& ctx, const Item& item, size_t index){
    item.tooltipRequestSent = true;
    u64 gen = ctx.navigation.Contents().Generation();
    u64 targetHash = item.hash;

    ctx.tasks.RunAsync(
        [pidl = item.pidl.Clone()]() mutable {
            std::string tip = FetchWindowsTooltip(pidl.get());
            return std::make_pair(std::move(pidl), std::move(tip));
        },
        [&ctx, gen, targetHash, index](std::pair<Pidl, std::string> result) mutable {
            ctx.navigation.Contents().PatchItem(result.first.get(), targetHash, index, gen, [&](Item& it){
                it.tooltipInfo.value = std::move(result.second);
                it.tooltipInfo.resolved = true;
            });
        }
    );
}


void WShell::Async::RequestMeta(AppContext& ctx, const Item& item, size_t index){
    item.metaRequestSent = true;
    u64 gen = ctx.navigation.Contents().Generation();
    bool isFolder = (item.attributes & SFGAO_FOLDER) != 0;  // matching Item::Size()'s existing short circuit
    u64 targetHash = item.hash;


    ctx.tasks.RunAsync(
        [pidl = item.pidl.Clone(), isFolder]() mutable {
            struct Result { Pidl pidl; std::string type; u64 size; };
            Result r;
            r.type = GetPidlTypeName(pidl.get());
            r.size = isFolder ? 0ULL : GetPidlFileSize(pidl.get());
            r.pidl = std::move(pidl);
            return r;
        },
        [&ctx, gen, targetHash, index](auto result) mutable {
            ctx.navigation.Contents().PatchItem(result.pidl.get(), targetHash, index, gen, [&](Item& it){
                it.typeName.value = std::move(result.type);
                it.typeName.resolved = true;
                it.size.value = result.size;
                it.size.resolved = true;
            });
        }
    );
}

void WShell::Async::RequestTileInfo(AppContext& ctx, const Item& item, size_t index){
    item.tileInfoRequestSent = true;
    u64 gen = ctx.navigation.Contents().Generation();
    u64 targetHash = item.hash;

    ctx.tasks.RunAsync(
        [pidl = item.pidl.Clone()]() mutable {
            std::string info = FetchTileViewLines(pidl.get());
            return std::make_pair(std::move(pidl), std::move(info));
        },
        [&ctx, gen, targetHash, index](std::pair<Pidl, std::string> result) mutable {
            ctx.navigation.Contents().PatchItem(result.first.get(), targetHash, index, gen, [&](Item& it){
                it.tileViewInfo.value = std::move(result.second);
                it.tileViewInfo.resolved = true;
            });
        }
    );
}
 


// Sidebar's ItemLite requests. No Directory/generation concept applies here - staleness is handled by the fact that 'owner' (a sidebar node's children list, or one of ctx.items1/2/3) is a stable, never-erased vector for the life of the app; PatchByPidl's own "not found -> no-op" behavious covers the case where the specific item is gone (deleted on disk, or the node ws rebuilt from scratch)
// intentionally ignoring hintIndex for LiteIcons, they are small enough to be O(N) and break in recursive
void WShell::Async::RequestLiteIcon(AppContext& ctx, std::vector<ItemLite>& owner, ItemLite& item){
    item.iconRequestSent = true;
    u64 targetHash = item.hash;

    ctx.tasks.RunAsync(
        [pidl = item.pidl.Clone()]() mutable {
            u32 iIcon = Icons::GetIconIndex(pidl.get(), nullptr, 0, SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
            return std::make_pair(std::move(pidl), iIcon);
        },
        [&owner, targetHash](std::pair<Pidl, u32> result) mutable {
            PatchByHash(owner, result.first.get(), targetHash, 0, [&](ItemLite& it){
                it.iconKey.value = result.second;
                it.iconKey.resolved = true;
            });
        }
    );
}


void WShell::Async::RequestHasSubFolders(AppContext& ctx, std::vector<ItemLite>& owner, ItemLite& item){
    item.hasSubFoldersRequestSent = true;
    u64 targetHash = item.hash;
 
    ctx.tasks.RunAsync(
        [pidl = item.pidl.Clone()]() mutable {
            bool has = PidlHasSubFolders(pidl.get());
            return std::make_pair(std::move(pidl), has);
        },
        [&owner, targetHash](std::pair<Pidl, bool> result) mutable {
            PatchByHash(owner, result.first.get(), targetHash, 0, [&](ItemLite& it){
                it.hasSubFolders.value = result.second;
                it.hasSubFolders.resolved = true;
            });
        }
    );
}

void WShell::Async::RequestSidebarItems(AppContext& ctx){
    ctx.tasks.RunAsync([]{ return GetSidebarItems(1);}, [&ctx](std::vector<ItemLite> r) {ctx.items1 = std::move(r); });
    ctx.tasks.RunAsync([]{ return GetSidebarItems(2);}, [&ctx](std::vector<ItemLite> r) {ctx.items2 = std::move(r); });
    ctx.tasks.RunAsync([]{ return GetSidebarItems(3);}, [&ctx](std::vector<ItemLite> r) {ctx.items3 = std::move(r); });
    ctx.tasks.RunAsync([]{ return EnumerateNewMenu();}, [&ctx](std::vector<NewMenuItem> r) {ctx.newMenuItems = std::move(r); });
}


void WShell::Async::RequestNewMenuIcon(AppContext& ctx, NewMenuItem& item){
    item.iconRequestSent = true;
    std::string ext = item.extension;   // copied — matched back by value, not by address
 
    ctx.tasks.RunAsync(
        [ext]() {
            wchar_t* wide = Str::Utf8ToWide(ext.c_str());
            u32 idx = (u32) Icons::GetIconIndex(nullptr, wide, FILE_ATTRIBUTE_NORMAL, SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
            free(wide);
            return idx;
        },
        [&ctx, ext](u32 iIcon) mutable {
            for (auto& it : ctx.newMenuItems){
                if (it.extension == ext){
                    it.iconKey.value = iIcon;
                    it.iconKey.resolved = true;
                }
            }
        }
    );
}