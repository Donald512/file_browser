#include "icons.h"
#include <commoncontrols.h>
#include <memory>
#include <functional>

using namespace Icons;

IImageList* IconManager::GetImageList(int shilSize) {
    switch (shilSize) {
        case SHIL_SMALL:      return imgListSmall.Get();
        case SHIL_LARGE:      return imgListLarge.Get();
        case SHIL_EXTRALARGE: return imgListExtraLarge.Get();
        case SHIL_JUMBO:      return imgListJumbo.Get();
        default:              return imgListLarge.Get();
    }
}

bool IconManager::Init(ID3D11Device* device, ID3D11DeviceContext* context){
    d3dDevice = device;
    d3dContext = context;

    // Fetch each system image list variant
    HRESULT hrSmall = SHGetImageList(SHIL_SMALL, IID_PPV_ARGS(&imgListSmall));
    HRESULT hrLarge = SHGetImageList(SHIL_LARGE, IID_PPV_ARGS(&imgListLarge));
    HRESULT hrExtra = SHGetImageList(SHIL_EXTRALARGE, IID_PPV_ARGS(&imgListExtraLarge));
    HRESULT hrJumbo = SHGetImageList(SHIL_JUMBO, IID_PPV_ARGS(&imgListJumbo));

    // Fail if at least small or large failed (Jumbo is safe on Vista+)
    if (FAILED(hrSmall) || FAILED(hrLarge)) {
        return false;
    }

    return true;
}


ImTextureID IconManager::GetTexture(const IconKey& key) {
    // Pack iconIndex (32-bit) and shilSize into a unique 64-bit cache key
    
    // search cache
    for (auto& cachedIcon : cachedIcons){
        if (cachedIcon.key == key){
            cachedIcon.lastUsedFrame = currentFrame;
            return reinterpret_cast<ImTextureID>(cachedIcon.texture.Get());
        }
    }

    IImageList* imgList = GetImageList(key.shilSize);
    if (!imgList) return {};

    // create texture if not found
    HICON hIcon = nullptr;
    HRESULT hr = imgList->GetIcon(static_cast<int>(key.iIcon), ILD_TRANSPARENT, &hIcon);
    if (!hIcon) return 0;

    auto iconScope = std::unique_ptr<HICON__, decltype(&DestroyIcon)>(hIcon, DestroyIcon);


    ImTextureID texture = HIconToTexture(iconScope.get());
    if (!texture) return 0;

    // Cache insertion & LRU eviction management
    if (cachedIcons.size() >= capacity) {
        u64 targetIndex = EvictLeastRecentlyUsed();
        CachedIcon& entry = cachedIcons[targetIndex];
        entry.key = key;
        entry.texture.Attach(reinterpret_cast<ID3D11ShaderResourceView*>(texture));
        entry.lastUsedFrame = currentFrame;
    } else {
        CachedIcon newEntry;
        newEntry.key = key;
        newEntry.texture.Attach(reinterpret_cast<ID3D11ShaderResourceView*>(texture));
        newEntry.lastUsedFrame = currentFrame;
        cachedIcons.push_back(std::move(newEntry));
    }

    return texture;
}



ImTextureID IconManager::HIconToTexture(HICON hIcon) {
    if (!hIcon || !d3dDevice) return 0;

    ICONINFO iconInfo = {};
    if (!GetIconInfo(hIcon, &iconInfo)) return 0;

    // RAII Wrappers for GDI Bitmaps ---
    // These automatically call DeleteObject when they go out of scope
    auto colorScope = std::unique_ptr<void, decltype(&DeleteObject)>(iconInfo.hbmColor, DeleteObject);
    auto maskScope = std::unique_ptr<void, decltype(&DeleteObject)>(iconInfo.hbmMask, DeleteObject);

    // get bitmap info
    BITMAP bmp = {};
    GetObjectW(iconInfo.hbmColor, sizeof(bmp), &bmp);

    // if no color bitmap, use mask
    if (!iconInfo.hbmColor) {
        GetObjectW(iconInfo.hbmMask, sizeof(bmp), &bmp);
    }

    u32 width = bmp.bmWidth;
    i32 height = bmp.bmHeight;

    // Extract pixel data from bitmap
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // Top-down DIB
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    std::vector<u8> pixels(width * height * 4);

    //  RAII Wrappers for Device Contexts ---
    HDC hdcRaw = GetDC(nullptr);
    auto hdcScope = std::unique_ptr<HDC__, std::function<void(HDC)>>(hdcRaw, [](HDC h) { 
        ReleaseDC(nullptr, h); 
    });

    HDC hdcMemRaw = CreateCompatibleDC(hdcRaw);
    auto hdcMemScope = std::unique_ptr<HDC__, std::function<void(HDC)>>(hdcMemRaw, [](HDC h) { 
        DeleteDC(h); 
    });

    HBITMAP hActiveBmp = iconInfo.hbmColor ? iconInfo.hbmColor : iconInfo.hbmMask;
    HBITMAP hOldBmp = (HBITMAP)SelectObject(hdcMemRaw, hActiveBmp);
    
    // Automatically restore the old bitmap when leaving scope
    auto restoreBmpScope = std::unique_ptr<HDC__, std::function<void(HDC)>>(hdcMemRaw, [hOldBmp](HDC h) { 
        SelectObject(h, hOldBmp); 
    });

    GetDIBits(hdcMemRaw, hActiveBmp, 0, height, pixels.data(), &bmi, DIB_RGB_COLORS);

    // Windows GDI gives us BGRA natively. Instead of manual swapping on the CPU with a loop, we configure the DXGI format below to read B8G8R8A8 directly

    // Create D3D11 texture using modern ComPtr tools
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    // Changed to reflect native GDI pixel formatting
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; 
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = pixels.data();
    initData.SysMemPitch = width * 4;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> pTexture;
    HRESULT hr = d3dDevice->CreateTexture2D(&desc, &initData, pTexture.GetAddressOf());
    if (FAILED(hr)) {
        return 0;
    }

    // Create shader resource view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> pSRV;
    hr = d3dDevice->CreateShaderResourceView(pTexture.Get(), &srvDesc, pSRV.GetAddressOf());
    if (FAILED(hr)) {
        return 0;
    }

    // Detach the raw address pointer directly to pass ownership back to ImGui interface
    return (ImTextureID)pSRV.Detach();
}


u64 IconManager::EvictLeastRecentlyUsed(){
    u64 indexOfLRU = 0;
    u32 oldestFrame = UINT32_MAX;   
    // the smaller it is, the older it is, born in 2002 is older than born in 2026, and it represents the LRU cos thats when it was last used, outdated

    // scan and find index of lowest LRU
    u64 i = 0;
    for (auto& cachedIcon : cachedIcons ){
        if (cachedIcon.lastUsedFrame < oldestFrame){
            oldestFrame = cachedIcon.lastUsedFrame;
            indexOfLRU = i;
        }
        i++;
    }

    if (cachedIcons[indexOfLRU].texture){
        cachedIcons[indexOfLRU].texture.Reset();
    }
    return indexOfLRU;
}


u32 Icons::GetIconIndex(PCIDLIST_ABSOLUTE pidl, const wchar_t* pszPath, DWORD dwFileAttributes, UINT uFlags) {
    SHFILEINFOW sfi = {};

    // Ensure SHGFI_SYSICONINDEX is always set
    UINT flags = uFlags | SHGFI_SYSICONINDEX;

    if (pidl) {
        SHGetFileInfoW(reinterpret_cast<LPCWSTR>(pidl), dwFileAttributes, &sfi, sizeof(sfi), flags);
    } else if (pszPath) {
        SHGetFileInfoW(pszPath, dwFileAttributes, &sfi, sizeof(sfi), flags);
    }

    return sfi.iIcon;
}

/* // todo
    GetDC(nullptr) + CreateCompatibleDC + DeleteDC + ReleaseDC on every single cal - Cache memory DC in iconcache
    many files in a folder have the same extension, so only get the key once per Plain folders, documents that arent .ico, .exe, .lnk etc



*/