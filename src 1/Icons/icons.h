#pragma once
#include "Types.h"
#include "imgui.h"
#include <d3d11.h>
#include <CommCtrl.h>
#include <commoncontrols.h>
#include "WinFramework.h"
#include <wrl/client.h>
#include <ShlObj.h>
#pragma comment(lib, "comctl32.lib")

using Microsoft::WRL::ComPtr;


namespace Icons{

    struct IconKey {
        u32 iIcon = 0;
        int shilSize = 0;

        bool operator==(const IconKey& other) const {
            return iIcon == other.iIcon && shilSize == other.shilSize;
        }
    };

    struct CachedIcon{
        IconKey key;
        ComPtr<ID3D11ShaderResourceView> texture;   // owned — this cache created it, so it releases it
        u32 lastUsedFrame = 0; // for LRU eviction (Least recently used)
    };

    class IconManager{
        public:
            // d3dDevice/d3dContext are borrowed from AppContext, which owns the real device —
            // that's why they stay raw pointers here, not ComPtr 
            bool Init(ID3D11Device* device, ID3D11DeviceContext* context);
            ImTextureID IconManager::GetTexture(const IconKey& key);
            void NextFrame() { currentFrame++; }
            IImageList* GetImageList(int shilSize);

        private:
            ImTextureID HIconToTexture(HICON hIcon);
            u64 EvictLeastRecentlyUsed();

            std::vector<CachedIcon> cachedIcons;
            u64 capacity = 256;
            // td change to memory based capacity, 32MB where each imageList gets 8MB
            u32 currentFrame = 0;

            // HIMAGELIST hSystemImageList = nullptr;   // system-owned, never released by us
            ID3D11Device* d3dDevice = nullptr;        // borrowed, non-owning
            ID3D11DeviceContext* d3dContext = nullptr;// borrowed, non-owning

            Microsoft::WRL::ComPtr<IImageList> imgListSmall;      // 16x16 (SHIL_SMALL)
            Microsoft::WRL::ComPtr<IImageList> imgListLarge;      // 32x32 (SHIL_LARGE)
            Microsoft::WRL::ComPtr<IImageList> imgListExtraLarge; // 48x48 (SHIL_EXTRALARGE)
            Microsoft::WRL::ComPtr<IImageList> imgListJumbo;      // 256x256 (SHIL_JUMBO)
    };

    u32 GetIconIndex(PCIDLIST_ABSOLUTE pidl, const wchar_t* pszPath, DWORD dwFileAttributes, UINT uFlags);
}

/*
class IconManager {
    private:
        // Budget in bytes, split so no single tier can dominate GPU memory
        static constexpr u64 kMemoryBudgetBytes = 32ull * 1024 * 1024; // 32 MB total, reasonable on integrated GPUs too

        // Per-tier caches, each with its own count cap derived from the shared budget
        struct TierCache {
            std::vector<CachedIcon> entries;
            u64 capacity; // computed from kMemoryBudgetBytes / bytesPerIcon for this tier
        };
        TierCache smallCache;      // 1 KB/icon  → budget/4 ≈ 8000 entries cap (never realistically hit)
        TierCache largeCache;      // 4 KB/icon  → budget/4 ≈ 2000 entries cap
        TierCache extraLargeCache;// 9 KB/icon  → budget/4 ≈ ~900 entries cap
        TierCache jumboCache;     // 256 KB/icon → budget/4 ≈ 32 entries cap
*/