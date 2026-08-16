
#pragma once

#include "imgui.h"
#include "BasicTypes.h"
#include <string>



// Only 1 current Font available at anytiem

struct FontPaths {
    std::string Primary = "C:\\Windows\\Fonts\\segoeui.ttf"; // Consider making this dynamic/configurable
    std::string Icons   = "thirdparty/fontstuff/FluentSystemIcons-Regular.ttf";
    std::string Emoji   = "C:\\Windows\\Fonts\\seguiemj.ttf";
};

void BuildFonts(float dpi, const FontPaths& paths = FontPaths());

inline void ResetImGuiStyleScale(f32 dpi = 1.0f){
    ImGuiStyle& style = ImGui::GetStyle();
    style.FontScaleDpi = dpi;
}

// Since only one font is used at a time
// will use the rasterize as you go for fonts
// when we encounter a glyph we dont have, we add it at the beginning of the frame
// if theres no space in the atlas during the middle of the frame, we expand the atlas memory and LRU after
// if the user changes font or font size, we flush the VRAM buffer and 

/*
// 1. Rebuild the atlas on CPU
BuildFonts(io.Fonts, currentDpi, newPaths);

// 2. Build texture pixels & update DX11 GPU texture
io.Fonts->Build();
UpdateDX11FontTexture(); // Your DirectX UpdateSubresource / Re-create ID3D11Texture2D
*/