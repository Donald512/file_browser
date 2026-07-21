#include <core.h>
namespace Icons
{

    void InitIconCache(AppContext& ctx){
        ctx.iconCache.d3dDevice = ctx.d3dDevice;
        ctx.iconCache.d3dContext = ctx.d3dContext;
        ctx.iconCache.capacity = 256;
        ctx.iconCache.entries = (CachedIcon*)calloc(ctx.iconCache.capacity, sizeof(CachedIcon));

        HRESULT hr = SHGetImageList(SHIL_LARGE, IID_IImageList, (void**)&ctx.iconCache.hSystemImageList);
        if (FAILED(hr)) {
            PrntErr("Failed to acquire System Image List");
        }
    }

    void DestroyIconCache(IconCache& cache){
        if (!cache.entries) return;

        for (u64 i = 0; i < cache.count; i++) {
            if (cache.entries[i].texture) {
                ((ID3D11ShaderResourceView*)cache.entries[i].texture)->Release();
            }
        }
        free(cache.entries);
        cache.entries = nullptr;
        cache.count = 0;
        cache.capacity = 0;
        
    }



    ImTextureID HIconToTexture(IconCache& cache, HICON hIcon){
        if (!hIcon  || !cache.d3dDevice) return 0;

        ICONINFO iconInfo = {};
        if (!GetIconInfo(hIcon, &iconInfo)) return 0;

        // get bitmap info
        BITMAP bmp = {};
        GetObjectW(iconInfo.hbmColor, sizeof(bmp), &bmp);

        // if no color bitmap, use mask
        if (!iconInfo.hbmColor){
            GetObjectW(iconInfo.hbmMask, sizeof(bmp), &bmp);
        }

        u32 width = bmp.bmWidth;
        i32 height = bmp.bmHeight;

        // Extract pixel data from bitmap
        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = width;
        bmi.bmiHeader.biHeight = -height;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        std::vector<u8> pixels(width * height * 4);

        HDC hdc = GetDC(nullptr);
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hOldBmp = (HBITMAP)SelectObject(hdcMem, iconInfo.hbmColor ? iconInfo.hbmColor : iconInfo.hbmMask);
        GetDIBits(hdcMem, iconInfo.hbmColor ? iconInfo.hbmColor : iconInfo.hbmMask, 0, height, pixels.data(), &bmi, DIB_RGB_COLORS);

        SelectObject(hdcMem, hOldBmp);
        DeleteDC(hdcMem);
        ReleaseDC(nullptr, hdc);

        
        // Windows GDI gives us BGRA, but D3D11 texture expects RGBA
        // So we swap Blue (0) with Red (2) bytes
        // Leave alpha (3) byte, ImGui needs straight alpha
        for (u32 i = 0; i < width * height; i++) {
            u8 b = pixels[i * 4 + 0];
            // pixels[i * 4 + 1] is Green, it stays exactly where it is.
            u8 r = pixels[i * 4 + 2];
            pixels[i * 4 + 0] = r; // Put Red where Blue was
            pixels[i * 4 + 2] = b; // Put Blue where Red was

        }

        // Create D3D11 texture
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = pixels.data();
        initData.SysMemPitch = width * 4;

        ID3D11Texture2D* pTexture = nullptr;
        HRESULT hr = cache.d3dDevice->CreateTexture2D(&desc, &initData, &pTexture);
        
        if (FAILED(hr)) {
            DeleteObject(iconInfo.hbmColor);
            DeleteObject(iconInfo.hbmMask);
            return 0;
        }

        // Create shader resource view
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = desc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;

        ID3D11ShaderResourceView* pSRV = nullptr;
        hr = cache.d3dDevice->CreateShaderResourceView(pTexture, &srvDesc, &pSRV);
        pTexture->Release();

        DeleteObject(iconInfo.hbmColor);
        DeleteObject(iconInfo.hbmMask);

        return (ImTextureID)pSRV;
    }

    u64 GetIconIndex(PIDLIST_ABSOLUTE pidl){
        if (!pidl) return 0;

        // Each icon has a unique index, so there's no need to hash it
        SHFILEINFOW sfi = {};
        SHGetFileInfoW((LPCWSTR) pidl, 0, &sfi, sizeof(sfi), SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_LARGEICON);
        // sfi.iIcon now contains the unique icon index
        return (u64) sfi.iIcon; // even if it fails, it returns 0
    }

    u64 GetIconIndex2(UINT flags){  // so i dont have GetIconIndexSmall
        SHFILEINFOW sfi = {};
        SHGetFileInfoW((LPCWSTR) L"DUMMY", 0, &sfi, sizeof(sfi), flags);  
        return (u64) sfi.iIcon; 
    }

    u64 GetIconIndexForExt(const wchar_t* extension){
        wchar_t dummyPath[MAX_PATH];
        swprintf_s(dummyPath, MAX_PATH, L"dummy%s", extension); // Creates dummy.txt if extension is .txt

        SHFILEINFOW sfi = {};
        UINT flags = SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX | SHGFI_SMALLICON;

        if (SHGetFileInfoW(dummyPath, FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(sfi), flags)){
            return (u64) sfi.iIcon;
        }
        return 0;
    }


    u64 GetIconIndexForAddressBar(PIDLIST_ABSOLUTE pidl){ // No more in use
        if (!pidl) return 0;

        // Each icon has a unique index, so there's no need to hash it
        SHFILEINFOW sfi = {};
        SHGetFileInfoW((LPCWSTR) pidl, 0, &sfi, sizeof(sfi), SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
        // sfi.iIcon now contains the unique icon index
        return (u64) sfi.iIcon; // even if it fails, it returns 0
    }




    ImTextureID GetIconTexture(AppContext& ctx, u64 key){
        IconCache& cache = ctx.iconCache;

        // search cache
        for (u64 i = 0; i < cache.count; i++){
            if (cache.entries[i].key == key){
                cache.entries[i].lastUsedFrame = cache.currentFrame;
                return cache.entries[i].texture;
            }
        }

        // Create texture if not found
        HICON hIcon = ImageList_GetIcon(cache.hSystemImageList, (int) key, ILD_TRANSPARENT);
        
        ImTextureID texture = HIconToTexture(cache, hIcon);
        DestroyIcon(hIcon);

        if (!texture) return 0;

        u64 targetIndex = 0;

        // Cache insetion & LRU eviction
        if (cache.count >= cache.capacity){
            targetIndex = EvictLeastRecentlyUsed(cache);
        }
        else{
            targetIndex = cache.count++;
        }

        // Add to cache
        CachedIcon& entry = cache.entries[targetIndex];
        entry.key = key;
        entry.texture = texture;
        entry.lastUsedFrame = cache.currentFrame;

        return texture;
    }

    u64 EvictLeastRecentlyUsed(IconCache& cache){
        u64 indexOfLRU = 0;
        u32 oldestFrame = UINT32_MAX;   
        // the smaller it is, the older it is, born in 2002 is older than born in 2026, and it represents the LRU cos thats when it was last used, outdated

        // scan and find index of lowest LRU
        for (u64 i = 0; i < cache.count; i++){
            if (cache.entries[i].lastUsedFrame < oldestFrame){
                oldestFrame = cache.entries[i].lastUsedFrame;
                indexOfLRU = i;
            }
        }

        if (cache.entries[indexOfLRU].texture){
            ID3D11ShaderResourceView* srv = (ID3D11ShaderResourceView*)cache.entries[indexOfLRU].texture;
            srv->Release();
            cache.entries[indexOfLRU].texture = 0;
        }

        return indexOfLRU;
    }

} // namespace Icons


/*

Initial plan was to store HICON in every ShellItem. Was abandoned because, some icons are used by many files, and storing each icon in each file is wasteful, 

Option 1: Store HICON in ShellItem
Each file carries its own icon around
And every file loads the same icon multiple times, even tho its the same icon

*/


/* // todo
    make d3dDevice spawn the texture with DXGI_FORMAT_B8G8R8A8_UNORM format, instead of manually swapping 
    GetDC(nullptr) + CreateCompatibleDC + DeleteDC + ReleaseDC on every single cal - Cache memory DC in iconcache
    many files in a folder have the same extension, so only get the key once per Plain folders, documents that arent .ico, .exe, .lnk etc



*/