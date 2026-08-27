

#include <d3d11.h>
#include "imgui.h"
#include <memory>
#include "BasicTypes.h"
#include <vector>
#include <wrl/client.h>
#include "Textures.h"

using Microsoft::WRL::ComPtr;


ImTextureID HIconToTexture(HICON hIcon, ID3D11Device* d3dDevice) {
    if (!hIcon || !d3dDevice) return 0;

    ICONINFO iconInfo = {};
    if (!GetIconInfo(hIcon, &iconInfo)) return 0;

    // RAII for Bitmaps: Zero heap allocations, zero pointer overhead
    std::unique_ptr<std::remove_pointer_t<HBITMAP>, GdiObjectDeleter> colorScope(iconInfo.hbmColor);
    std::unique_ptr<std::remove_pointer_t<HBITMAP>, GdiObjectDeleter> maskScope(iconInfo.hbmMask);

    BITMAP bmp = {};
    HBITMAP hActiveBmp = iconInfo.hbmColor ? iconInfo.hbmColor : iconInfo.hbmMask;
    GetObjectW(hActiveBmp, sizeof(bmp), &bmp);

    u32 width = bmp.bmWidth;
    i32 height = bmp.bmHeight;

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // Top-down DIB
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    std::vector<u8> pixels(width * height * 4);

    // RAII for Device Contexts
    std::unique_ptr<std::remove_pointer_t<HDC>, DcReleaser> hdcScope(GetDC(nullptr));
    if (!hdcScope) return 0;

    std::unique_ptr<std::remove_pointer_t<HDC>, DcDeleter> hdcMemScope(CreateCompatibleDC(hdcScope.get()));
    if (!hdcMemScope) return 0;

    // Select new bitmap and automatically restore old bitmap on scope exit
    HBITMAP hOldBmp = (HBITMAP)SelectObject(hdcMemScope.get(), hActiveBmp);
    ScopeGuard restoreOldBmp([&]() { SelectObject(hdcMemScope.get(), hOldBmp); });

    GetDIBits(hdcMemScope.get(), hActiveBmp, 0, height, pixels.data(), &bmi, DIB_RGB_COLORS);

    // Create D3D11 Texture
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; 
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = pixels.data();
    initData.SysMemPitch = width * 4;

    ComPtr<ID3D11Texture2D> pTexture;
    if (FAILED(d3dDevice->CreateTexture2D(&desc, &initData, pTexture.GetAddressOf()))) {
        return 0;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    ComPtr<ID3D11ShaderResourceView> pSRV;
    if (FAILED(d3dDevice->CreateShaderResourceView(pTexture.Get(), &srvDesc, pSRV.GetAddressOf()))) {
        return 0;
    }

    return (ImTextureID)pSRV.Detach();
}


std::vector<u8> BitmapToPixels(HBITMAP hbmp, int& outWidth, int& outHeight) {
    std::vector<u8> pixels;
    if (!hbmp) return pixels;

    BITMAP bm;
    if (!GetObject(hbmp, sizeof(bm), &bm)) return pixels;

    outWidth = bm.bmWidth;
    outHeight = bm.bmHeight;

    if (outWidth <= 0 || outHeight <= 0) return pixels;

    HDC hdcScreen = GetDC(nullptr);

    // Setup standard 32-bit ARGB header
    BITMAPINFOHEADER bmi = {0};
    bmi.biSize = sizeof(BITMAPINFOHEADER);
    bmi.biWidth = outWidth;
    bmi.biHeight = -outHeight; // Negative for top-down row order
    bmi.biPlanes = 1;
    bmi.biBitCount = 32;
    bmi.biCompression = BI_RGB;

    pixels.resize(static_cast<size_t>(outWidth) * outHeight * 4); // 4 bytes per pixel (RGBA)

    // Pass hdcScreen instead of hdcMem for accurate color extraction
    if (!GetDIBits(hdcScreen, hbmp, 0, outHeight, pixels.data(), (BITMAPINFO*)&bmi, DIB_RGB_COLORS)) {
        ReleaseDC(nullptr, hdcScreen);
        pixels.clear();
        return pixels;
    }

    ReleaseDC(nullptr, hdcScreen);

    // Convert Windows BGRA to standard RGBA & fix broken alpha
    for (size_t i = 0; i < pixels.size(); i += 4) {
        u8 b = pixels[i];
        u8 g = pixels[i + 1];
        u8 r = pixels[i + 2];
        u8 a = pixels[i + 3];

        // Fix missing alpha on 32-bit Win32 menu bitmaps
        if (a == 0 && (r != 0 || g != 0 || b != 0)) {
            a = 255;
        }

        pixels[i]     = r;
        pixels[i + 1] = g;
        pixels[i + 2] = b;
        pixels[i + 3] = a;
    }

    return pixels;
}

ComPtr<ID3D11ShaderResourceView> CreateTextureFromRGBA(ID3D11Device* pDevice, const std::vector<u8>& pixels, int width, int height) {
    if (!pDevice || pixels.empty() || width <= 0 || height <= 0) {
        return nullptr;
    }

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
    if (FAILED(pDevice->CreateTexture2D(&desc, &initData, &pTexture))) return nullptr;

    ComPtr<ID3D11ShaderResourceView> pSRV;
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    pDevice->CreateShaderResourceView(pTexture, &srvDesc, &pSRV);
    pTexture->Release(); // Release temp texture

    return pSRV;
}