#include "Core.h"
#include "iconRegular.h"


void BuildFonts(AppContext& ctx, ImFontAtlas* atlas){
    if (atlas == nullptr) return;
    atlas->Clear();

    static const ImWchar icon_ranges[] = { (ImWchar)ICON_MIN_REG, (ImWchar)ICON_MAX_REG, 0 };
    static const ImWchar32 emoji_ranges[] = {
        0x2000, 0x206F, 0x2600, 0x26FF, 0x2700, 0x27BF,
        0x1F300, 0x1F64F, 0x1F680, 0x1F6FF, 0x1F900, 0x1F9FF, 0
    };

    ImFontConfig icon_config;
    icon_config.MergeMode = true;
    icon_config.GlyphOffset.y = 2.0f * ctx.ui.dpiScale;
    icon_config.PixelSnapH = true;
    icon_config.GlyphMinAdvanceX = 16.0f * ctx.ui.dpiScale;

    ImFontConfig emoji_config;
    emoji_config.MergeMode = true;
    emoji_config.FontDataOwnedByAtlas = false;

    f32 dpi = ctx.ui.dpiScale;
    // mainFont + its merged icons/emoji ---
    ctx.ui.mainFont = atlas->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 16.0f * dpi);
    atlas->AddFontFromFileTTF("thirdparty\\fontstuff\\FluentSystemIcons-Regular.ttf", 14.0f * dpi, &icon_config, icon_ranges);
    atlas->AddFontFromFileTTF("C:\\Windows\\Fonts\\seguiemj.ttf", 16.0f * dpi, &emoji_config, emoji_ranges);

    ctx.ui.smallFont = atlas->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 15.0f * dpi);
    atlas->AddFontFromFileTTF("thirdparty\\fontstuff\\FluentSystemIcons-Regular.ttf", 12.0f * dpi, &icon_config, icon_ranges);
    atlas->AddFontFromFileTTF("C:\\Windows\\Fonts\\seguiemj.ttf", 12.0f * dpi, &emoji_config, emoji_ranges);

    ImGuiStyle& style = ImGui::GetStyle();
    // style.ScaleAllSizes(ctx.ui.dpiScale);
    style.FontScaleDpi = 1.0f;
}

void QueryClipBoardCutItems(AppContext& ctx){
    std::unordered_set<u64> result{};

    if (!OpenClipboard(ctx.gfx.hwnd)) return;
    // check if the clipBoard could open before clearing the previous, some apps could be holding it hostage

    // Check if drop effect first
    HANDLE hDropEffect = GetClipboardData(ctx.cfDropEffect);
    if (!hDropEffect){
        CloseClipboard();
        return;
    }
    void* pEffect = GlobalLock(hDropEffect);
    if (!pEffect){
        CloseClipboard();
        return;
    }
    DWORD effect = *reinterpret_cast<DWORD*>(pEffect);
    GlobalUnlock(hDropEffect);

    if ((effect & DROPEFFECT_MOVE) == 0){
        CloseClipboard();
        return;
    } 

    HANDLE hShellIDList = GetClipboardData(ctx.cfShellIDList);

    if (hShellIDList) {
        if (LPIDA pIDA = static_cast<LPIDA>(GlobalLock(hShellIDList))) {
            // Index 0 in cida.aoffset is the parent folder PIDL
            LPCITEMIDLIST parentPidl = reinterpret_cast<LPCITEMIDLIST>(
                reinterpret_cast<BYTE*>(pIDA) + pIDA->aoffset[0]
            );

            // Indices 1..cidl are the relative PIDLs of each selected item
            for (UINT i = 0; i < pIDA->cidl; ++i) {
                LPCITEMIDLIST childPidl = reinterpret_cast<LPCITEMIDLIST>(
                    reinterpret_cast<BYTE*>(pIDA) + pIDA->aoffset[i + 1]
                );

                // Combine parent + relative child into a full absolute PIDL
                PIDLIST_ABSOLUTE fullPidl = ILCombine(parentPidl, childPidl);
                if (fullPidl) {
                    result.insert(WShell::HashPidl(fullPidl));
                    ILFree(fullPidl);
                }
            }
            GlobalUnlock(hShellIDList);
        }
    }

    CloseClipboard();
    ctx.clipBoardCutItems = result;
    return;
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