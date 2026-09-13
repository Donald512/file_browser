#pragma once

#include <d3d11.h>
#include "imgui.h"
#include "BasicTypes.h"
#include <vector>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;


ImTextureID HIconToTexture(HICON hIcon, ID3D11Device* d3dDevice);

std::vector<u8> BitmapToPixels(HBITMAP hbmp, int& outWidth, int& outHeight);

ComPtr<ID3D11ShaderResourceView> CreateTextureFromRGBA(ID3D11Device* pDevice, const std::vector<u8>& pixels, int width, int height);