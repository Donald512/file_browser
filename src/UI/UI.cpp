#include "UI.h"

void UI::Render(AppContext& ctx){
    // Make the root ImGui window fill the entire Windows window.
    const ImGuiViewport* viewport = ImGui::GetMainViewport();   
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, Style::NoPadding);
    if (ImGui::Begin("Main UI Workspace", nullptr, WindowFlags)){
        ImGui::PopStyleVar();
        
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f)); // Transparent
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
    
        TopBar::Render(ctx);
        ToolBar::Render(ctx); // contains NavBar, Address Bar, and Search Bar
        CommandBar::Render(ctx);

        ImGui::PushFont(ctx.ui.smallFont);
        // ================================
        f32 sidebarWidth = Sidebar::Width * ctx.ui.dpiScale;
        Sidebar::Render(ctx, sidebarWidth);
        ImGui::SameLine(0, 0);

        f32 splitterWidth = 4.0f * ctx.ui.dpiScale;
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0)); // Make it invisible
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0.47f, 0.83f, 1.0f)); // Blue when clicked
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.2f, 0.2f, 1.0f)); // Grey when hovered

        // Draw the button filling the height of the window
        ImGui::Button("##vsplitter", ImVec2(splitterWidth, ImGui::GetContentRegionAvail().y));

        // Change cursor to resize arrow when hovering the invisible button
        if (ImGui::IsItemHovered()) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
        }

        // Logic to resize
        if (ImGui::IsItemActive()) {
            // Add mouse delta to sidebar width
            Sidebar::Width += ImGui::GetIO().MouseDelta.x;
            
            // Clamp the width so they can't shrink it to 0 or make it take the whole screen
            if (Sidebar::Width < 150.0f * ctx.ui.dpiScale) Sidebar::Width = 150.0f * ctx.ui.dpiScale;
            f32 maxWidth = ImGui::GetWindowWidth()/2 - (100.0f * ctx.ui.dpiScale);
            if (Sidebar::Width > maxWidth) Sidebar::Width = maxWidth;
        }
        ImGui::PopStyleColor(3);
        ImGui::SameLine(0, 0);

        // ================================

        FileView::Render(ctx);
        ImGui::PopFont();
        
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
        
        ImGui::End();
    }
}

bool UI::Helpers::IconAndTextButton(const char* str_id, const char* icon, const char* label, const ImVec4& icon_color){
    ImGui::PushID(str_id);

    f32 iconWidth = ImGui::CalcTextSize(icon).x;
    f32 textWidth = ImGui::CalcTextSize(label).x;
    f32 innerSpacing = ImGui::GetStyle().ItemInnerSpacing.x;
    f32 btnWidth = iconWidth + textWidth + (innerSpacing * 2.0f);
    f32 btnHeight = ImGui::GetFrameHeight();

    ImVec2 startCursorPos = ImGui::GetCursorPos();

    // clickable bounding box
    bool pressed = ImGui::Button("##bg", ImVec2(btnWidth, btnHeight));

    ImGui::SetCursorPos(ImVec2(startCursorPos.x + innerSpacing, startCursorPos.y + (btnHeight - ImGui::GetTextLineHeight()) * 0.5f));

    ImGui::PushStyleColor(ImGuiCol_Text, icon_color);
    ImGui::TextUnformatted(icon);
    ImGui::PopStyleColor();

    ImGui::SameLine(0.0f, innerSpacing);
    ImGui::TextUnformatted(label);

    ImGui::PopID();
    return pressed;
}


void UI::Helpers::DrawCenteredWrappedText(const char* text, float columnWidth, float maxTextWidth, int maxLines) {
    ImFont* font = ImGui::GetFont();
    float fontSize = ImGui::GetFontSize();
    const char* text_end = text + strlen(text);
    const char* current = text;
    int currentLine = 0;

    // CRITICAL: Capture the absolute left edge of the column ONCE so newlines don't reset it
    float startCursorX = ImGui::GetCursorPosX(); 

    while (current < text_end && currentLine < maxLines) {
        currentLine++;
        bool isLastLine = (currentLine == maxLines);

        const char* wrap_pos = font->CalcWordWrapPositionA(1.0f, current, text_end, maxTextWidth);

        // If a single word is insanely long, force break it so it doesn't bleed out of the column
        if (wrap_pos == current) {
            const char* p = current;
            while (p < text_end) {
                const char* next_p = p + 1;
                if (font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, current, next_p).x > maxTextWidth) {
                    wrap_pos = (p == current) ? next_p : p;
                    break;
                }
                p = next_p;
            }
            if (p == current || p >= text_end) wrap_pos = text_end;
        }

        std::string lineStr(current, wrap_pos);

        // Last line enforcement (Truncate + append "...")
        if (isLastLine && wrap_pos < text_end) {
            const std::string ellipsis = "...";
            while (!lineStr.empty()) {
                std::string testStr = lineStr + ellipsis;
                if (font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, testStr.c_str()).x <= maxTextWidth) {
                    lineStr = testStr;
                    break;
                }
                lineStr.pop_back(); 
            }
            if (lineStr.empty()) lineStr = ellipsis;
            wrap_pos = text_end; 
        }

        // --- Render line perfectly centered ---
        ImVec2 lineSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, lineStr.c_str());
        float offsetX = (columnWidth - lineSize.x) * 0.5f;

        ImGui::SetCursorPosX(startCursorX + (std::max)(0.0f, offsetX));
        ImGui::TextUnformatted(lineStr.c_str());

        // Advance pointer to start of next line
        current = wrap_pos;
        while (current < text_end && (*current == ' ' || *current == '\n')) current++;
    }
}

void UI::Helpers::DrawSingleLineTruncatedText(const char* text, float maxWidth) {
    if (!text || *text == '\0') return;

    ImFont* font = ImGui::GetFont();
    float fontSize = ImGui::GetFontSize();

    // 1. Check if the full text fits
    ImVec2 fullSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, text);
    if (fullSize.x <= maxWidth) {
        ImGui::TextUnformatted(text);
        return;
    }

    const char* ellipsis = "...";
    float ellipsisWidth = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, ellipsis).x;

    // If even the ellipsis cannot fit, render as much as possible or just nothing/ellipsis
    if (ellipsisWidth > maxWidth) {
        // Option A: Render just ellipsis truncated by clipping, or return early
        ImGui::TextUnformatted(ellipsis);
        return;
    }

    // Available width left for the text prefix before appending "..."
    float targetWidth = maxWidth - ellipsisWidth;

    // 2. Fast step: Find the cut-off point using ImGui's built-in line breaker
    // CalcTextSizeA natively handles UTF-8 characters properly.
    const char* remaining = nullptr;
    font->CalcTextSizeA(fontSize, targetWidth, 0.0f, text, nullptr, &remaining);

    // 'remaining' points to the start of the first character that did NOT fit within 'targetWidth'
    if (remaining > text) {
        ImGui::TextUnformatted(text, remaining);
        ImGui::SameLine(0, 0);
        ImGui::TextUnformatted(ellipsis);
    } else {
        // Fallback if not even 1 character fits with the ellipsis
        ImGui::TextUnformatted(ellipsis);
    }
}

int UI::Helpers::GetWrappedLineCount(const char* text, float maxTextWidth, int maxLines) {
    ImFont* font = ImGui::GetFont();
    float fontSize = ImGui::GetFontSize();
    const char* text_end = text + strlen(text);
    const char* current = text;
    int currentLine = 0;

    while (current < text_end && currentLine < maxLines) {
        currentLine++;
        const char* wrap_pos = font->CalcWordWrapPositionA(1.0f, current, text_end, maxTextWidth);

        // Our custom forced-break logic for long single words
        if (wrap_pos == current) {
            const char* p = current;
            while (p < text_end) {
                const char* next_p = p + 1;
                if (font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, current, next_p).x > maxTextWidth) {
                    wrap_pos = (p == current) ? next_p : p;
                    break;
                }
                p = next_p;
            }
            if (p == current || p >= text_end) wrap_pos = text_end;
        }

        current = wrap_pos;
        while (current < text_end && (*current == ' ' || *current == '\n')) current++;
    }
    return currentLine;
}

void UI::Helpers::AlignCursorVertically(f32 containerHeightPx, f32 itemHeightPx){
    f32 centerY = (containerHeightPx - itemHeightPx) * 0.5f;
    // Prevent negative offsets if the item is somehow bigger than the container
    if (centerY > 0.0f) {
        ImGui::SetCursorPosY(centerY);
    }
}

void UI::Helpers::TextCentered(const char* text) {
    f32 windowWidth = ImGui::GetContentRegionAvail().x;
    f32 textWidth = ImGui::CalcTextSize(text).x;

    ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
    ImGui::TextUnformatted(text);
}


bool UI::Helpers::IconButton(const char* iconLabel, f32 sizePx, bool disabled) {
    if (disabled) ImGui::BeginDisabled();

    ImGui::PushID((void*) iconLabel);
    bool clicked = ImGui::Button(iconLabel, ImVec2(sizePx, sizePx));
    ImGui::PopID();
    
    if (disabled) ImGui::EndDisabled();
    
    return clicked;
}