#include "TextureManager.h"

IImageList* TextureManager::GetImageList(int shilSize) const {
    switch (shilSize) {
        case SHIL_SMALL:      return imgListSmall.Get();
        case SHIL_LARGE:      return imgListLarge.Get();
        case SHIL_EXTRALARGE: return imgListExtraLarge.Get();
        case SHIL_JUMBO:      return imgListJumbo.Get();
        default:              return imgListLarge.Get();
    }
}

size_t CalculateTextureSize(int shilSize) {
        switch (shilSize) {
            case SHIL_SMALL:      return 16 * 16 * 4;
            case SHIL_EXTRALARGE: return 48 * 48 * 4;
            case SHIL_JUMBO:      return 256 * 256 * 4;
            case SHIL_LARGE:
            default:              return 32 * 32 * 4;
        }
}

bool TextureManager::Init(ID3D11Device* device, ID3D11DeviceContext* context){
    d3dDevice = device;
    d3dContext = context;

    // Fetch each system image list variant
    HRESULT hrSmall = SHGetImageList(SHIL_SMALL, IID_PPV_ARGS(&imgListSmall));
    HRESULT hrLarge = SHGetImageList(SHIL_LARGE, IID_PPV_ARGS(&imgListLarge));
    HRESULT hrExtra = SHGetImageList(SHIL_EXTRALARGE, IID_PPV_ARGS(&imgListExtraLarge));
    HRESULT hrJumbo = SHGetImageList(SHIL_JUMBO, IID_PPV_ARGS(&imgListJumbo));
    (void)hrJumbo; (void)hrSmall;

    // Fail if at least small or large failed (Jumbo is safe on Vista+)
    if (FAILED(hrExtra) || FAILED(hrLarge)) {
        return false;
    }

    return true;
}

ImTextureID TextureManager::GetTexture(const IconKey& key) const {
    // Pack iconIndex (32-bit) and shilSize into a unique 64-bit cache key
    
    // search cache
    for (auto& cachedTexture : cachedTextures){
        if (cachedTexture.key == key){
            cachedTexture.lastUsedFrame = currentFrame;
            return reinterpret_cast<ImTextureID>(cachedTexture.texture.Get());
        }
    }
    
    IImageList* imgList = GetImageList(key.shilSize);
    if (!imgList) return {};
    
    // create texture if not found
    HICON hIcon = nullptr;
    imgList->GetIcon(static_cast<int>(key.iIcon), ILD_TRANSPARENT, &hIcon);
    if (!hIcon) return 0;
    
    auto iconScope = std::unique_ptr<HICON__, decltype(&DestroyIcon)>(hIcon, DestroyIcon);
    
    
    ImTextureID texture = HIconToTexture(iconScope.get(), d3dDevice);
    if (!texture) return 0;
    
    // Cache insertion & LRU eviction management
    if (cachedTextures.size() >= capacity) {
        u64 targetIndex = EvictLeastRecentlyUsed();
        CachedTexture& entry = cachedTextures[targetIndex];
        entry.key = key;
        entry.texture.Attach(reinterpret_cast<ID3D11ShaderResourceView*>(texture));
        entry.lastUsedFrame = currentFrame;
    } else {
        CachedTexture newEntry;
        newEntry.key = key;
        newEntry.texture.Attach(reinterpret_cast<ID3D11ShaderResourceView*>(texture));
        newEntry.lastUsedFrame = currentFrame;
        cachedTextures.push_back(std::move(newEntry));
    }
    
    return texture;
}


u64 TextureManager::EvictLeastRecentlyUsed() const{
    u64 indexOfLRU = 0;
    u32 oldestFrame = UINT32_MAX;   
    // the smaller it is, the older it is, born in 2002 is older than born in 2026, and it represents the LRU cos thats when it was last used, outdated

    // scan and find index of lowest LRU
    u64 i = 0;
    for (auto& cachedTexture : cachedTextures ){
        if (cachedTexture.lastUsedFrame < oldestFrame){
            oldestFrame = cachedTexture.lastUsedFrame;
            indexOfLRU = i;
        }
        i++;
    }

    if (cachedTextures[indexOfLRU].texture){
        cachedTextures[indexOfLRU].texture.Reset();
    }
    return indexOfLRU;
}