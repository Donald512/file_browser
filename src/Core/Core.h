#pragma once

#include "AppContext.h" 
#include <d3d11.h> 
#include <vector>
#include "Types.h"
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

void SetBackgroundColor(AppContext& ctx, float r, float g, float b, float a);
void QueryClipBoardCutItems(AppContext& ctx);
void BuildFonts(AppContext& ctx, ImFontAtlas* atlas);
ComPtr<ID3D11ShaderResourceView> CreateTextureFromRGBA(ID3D11Device* pDevice, const std::vector<u8>& pixels, int width, int height);
std::vector<u8> BitmapToPixels(HBITMAP hbmp, int& outWidth, int& outHeight);