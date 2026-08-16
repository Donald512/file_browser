#include "imgui_fonts.h"


#include "imgui.h"
#include "BasicTypes.h"
#include "iconRegular.h"
// #include "imgui_freetype.h"


#include <string>

static const ImWchar icon_ranges[] = { (ImWchar)ICON_MIN_REG, (ImWchar)ICON_MAX_REG, 0 };
static const ImWchar32 emoji_ranges[] = {
    0x1F600, 0x1F64F, // Classic emoji faces
    0x1F900, 0x1F9AF, // Newer faces (🤔 🤣 🥹 🥶 🥸 etc.)
    0
};



void LoadFontWithGlyphs(ImFontAtlas* atlas, const FontPaths& paths, f32 sizePx, f32 dpi){
    ImFont* font = atlas->AddFontFromFileTTF(paths.Primary.c_str(), sizePx * dpi);
    if (!font) return;

    // Merge Icons
    ImFontConfig icon_config;
    icon_config.MergeMode = true;
    icon_config.GlyphOffset.y = 2.0f * dpi;
    icon_config.PixelSnapH = true;
    icon_config.GlyphMinAdvanceX = sizePx * dpi;
    atlas->AddFontFromFileTTF(paths.Icons.c_str(), (sizePx - 2.0f) * dpi, &icon_config, icon_ranges);

    // Merge Emojis
    ImFontConfig emoji_config;
    emoji_config.MergeMode = true;
    // emoji_config.FontLoaderFlags = ImGuiFreeTypeLoaderFlags_LoadColor;
    atlas->AddFontFromFileTTF(paths.Emoji.c_str(), (sizePx - 2.0f) * dpi, &emoji_config, emoji_ranges);

    // atlas->FontLoaderFlags = ImGuiFreeTypeLoaderFlags_LoadColor;
}

void BuildFonts(float dpi, const FontPaths& paths) {
    ImGuiIO& io = ImGui::GetIO();
    ImFontAtlas* atlas = io.Fonts;
    if (!atlas) return;
    atlas->Clear();
    LoadFontWithGlyphs(atlas, paths, 16.0f, dpi);

}

