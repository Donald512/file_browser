#pragma once

#include <d3d11.h>
#include "imgui.h"
#include <memory>
#include "BasicTypes.h"
#include <vector>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

struct GdiObjectDeleter { void operator()(HGDIOBJ h) const { if (h) DeleteObject(h); } };
struct DcReleaser      { void operator()(HDC h)     const { if (h) ReleaseDC(nullptr, h); } };
struct DcDeleter       { void operator()(HDC h)     const { if (h) DeleteDC(h); } };

template <typename F>
class ScopeGuard {
    F func;
public:
    explicit ScopeGuard(F&& f) : func(std::move(f)) {}
    ~ScopeGuard() { func(); }
};

ImTextureID HIconToTexture(HICON hIcon, ID3D11Device* d3dDevice);

std::vector<u8> BitmapToPixels(HBITMAP hbmp, int& outWidth, int& outHeight);

ComPtr<ID3D11ShaderResourceView> CreateTextureFromRGBA(ID3D11Device* pDevice, const std::vector<u8>& pixels, int width, int height);