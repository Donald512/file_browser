#include "Types.h"
#include "imgui.h"
#include <d3d11.h>
#include <CommCtrl.h>
#include "WinFramework.h"
#include <wrl/client.h>
#include <ShlObj.h>
#pragma comment(lib, "comctl32.lib")

using Microsoft::WRL::ComPtr;


namespace Icons{
    struct CachedIcon{
        u64 key = 0;
        ComPtr<ID3D11ShaderResourceView> texture;   // owned — this cache created it, so it releases it
        u32 lastUsedFrame = 0; // for LRU eviction (Least recently used)
    };

    class IconManager{
        public:
            // d3dDevice/d3dContext are borrowed from AppContext, which owns the real device —
            // that's why they stay raw pointers here, not ComPtr 
            bool Init(ID3D11Device* device, ID3D11DeviceContext* context);
            ImTextureID GetTexture(u64 key);
            void NextFrame() { currentFrame++; }

        private:
            ImTextureID HIconToTexture(HICON hIcon);
            u64 EvictLeastRecentlyUsed();

            std::vector<CachedIcon> cachedIcons;
            u64 capacity = 256;
            u32 currentFrame = 0;

            HIMAGELIST hSystemImageList = nullptr;   // system-owned, never released by us
            ID3D11Device* d3dDevice = nullptr;        // borrowed, non-owning
            ID3D11DeviceContext* d3dContext = nullptr;// borrowed, non-owning
    };

    u64 GetIconIndex(PCIDLIST_ABSOLUTE pidl, const wchar_t* pszPath, DWORD dwFileAttributes, UINT uFlags);
}