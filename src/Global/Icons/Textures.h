#pragma once
#include <d3d11.h>
#include "imgui.h"
#include <memory>
#include "BasicTypes.h"
#include <vector>
#include <functional>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;


inline ImTextureID HIconToTexture(HICON hIcon, ID3D11Device* d3dDevice) {
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
