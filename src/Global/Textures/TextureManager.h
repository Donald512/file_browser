#pragma once

#include "BasicTypes.h"
#include <ShlObj.h>
#include <commoncontrols.h>
#include "Textures.h"

// inline constexpr size_t MAX_TEXTURE_CACHE_MEMORY = 32 * 1024 * 1024;
// Td switch to std::list + hash map for memory based evicting

struct IconKey {
    u32 iIcon = 0;
    int shilSize = 0;

    bool operator==(const IconKey& other) const {
        return iIcon == other.iIcon && shilSize == other.shilSize;
    }
};

struct CachedTexture{
    IconKey key;
    ComPtr<ID3D11ShaderResourceView> texture;   // owned — this cache created it, so it releases it
    u32 lastUsedFrame = 0; // for LRU eviction (Least recently used)
};

// d3dDevice/d3dContext are borrowed from AppContext, which owns the real device —
// that's why they stay raw pointers here, not ComPtr 

class TextureManager{
    public:
        void NextFrame() { currentFrame++; }
        bool Init(ID3D11Device* device, ID3D11DeviceContext* context);
        ImTextureID GetTexture(const IconKey& key) const ;
        IImageList* GetImageList(int shilSize) const ;
        

    private:
        // std::vector<CachedTexture> cachedTextures;
        // size_t textureCacheSize = 0;
        
        mutable std::vector<CachedTexture> cachedTextures;
        u64 capacity = 256;

        u32 currentFrame = 0;
        u64 EvictLeastRecentlyUsed() const ;

        std::vector<u64> freeSpots;

        ID3D11Device* d3dDevice = nullptr;        // borrowed, non-owning
        ID3D11DeviceContext* d3dContext = nullptr;// borrowed, non-owning

        Microsoft::WRL::ComPtr<IImageList> imgListSmall;      // 16x16 (SHIL_SMALL)
        Microsoft::WRL::ComPtr<IImageList> imgListLarge;      // 32x32 (SHIL_LARGE)
        Microsoft::WRL::ComPtr<IImageList> imgListExtraLarge; // 48x48 (SHIL_EXTRALARGE)
        Microsoft::WRL::ComPtr<IImageList> imgListJumbo;      // 256x256 (SHIL_JUMBO)

};