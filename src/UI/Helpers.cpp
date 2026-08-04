#include "UI.h"
#include "imgui.h"

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

// Draws text. If it exceeds the column width, it adds "..." and provides a tooltip!
void UI::Helpers::DrawTableTextWithTooltip(const char* text, bool isRowHovered) {
    f32 availWidth = ImGui::GetContentRegionAvail().x;
    f32 textWidth = ImGui::CalcTextSize(text).x;

    DrawSingleLineTruncatedText(text, availWidth);
    
    // If the text is too long, the mouse is on this row, AND the mouse is in this specific column:
    if (textWidth > availWidth && isRowHovered && ImGui::TableGetHoveredColumn() == ImGui::TableGetColumnIndex()) {
        ImGui::SetTooltip("%s", text);
    }
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

bool UI::Helpers::MenuRow(const char* strId, const char* label, f32 dpi, bool selected, MenuRowStyle style, f32 width){
    f32 outerMargin = style.outerMargin * dpi;
    f32 innerPadX    = style.innerPad.x * dpi;
    f32 innerPadY    = style.innerPad.y * dpi;
    f32 rounding    = style.rounding * dpi;
    f32 rowWidth    = (width > 0.0f) ? width : ImGui::GetContentRegionAvail().x;
    f32 rowHeight   = ImGui::GetFrameHeight() + innerPadY;

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + outerMargin);

    ImGui::PushID(strId);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive,  ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_Header,        ImVec4(0,0,0,0));
    bool clicked = ImGui::Selectable("##row", selected, 0, ImVec2(rowWidth - outerMargin * 2.0f, rowHeight));
    bool hovered = ImGui::IsItemHovered();
    ImGui::PopStyleColor(3);
    ImGui::PopID();

    ImVec2 boxMin = ImGui::GetItemRectMin();
    ImVec2 boxMax = ImGui::GetItemRectMax();

    if (selected || hovered){
        ImU32 col = ImGui::GetColorU32(selected ? ImGuiCol_HeaderActive : ImGuiCol_HeaderHovered);
        ImGui::GetWindowDrawList()->AddRectFilled(boxMin, boxMax, col, rounding);
    }
    ImGui::GetWindowDrawList()->AddText(
        ImVec2(boxMin.x + innerPadX, boxMin.y + (rowHeight - ImGui::GetTextLineHeight()) * 0.5f),
        ImGui::GetColorU32(ImGuiCol_Text), label);

    ImGui::Dummy(ImVec2(0, style.itemSpacing.y * dpi));
    return clicked;
}

// creating a new one because i need one that doesnt center but shifts left by some amount
bool UI::Helpers::MenuRow(const char* strId, const char* label, ImTextureID icon, f32 dpi, MenuRowStyle style, f32 leftPush, f32 spaceBetweenIconAndText, f32 width, bool selected){
    f32 outerMargin = style.outerMargin * dpi;
    f32 innerPadX   = style.innerPad.x * dpi;
    f32 innerPadY   = style.innerPad.y * dpi;
    f32 rounding    = style.rounding * dpi;
    f32 rowWidth    = (width > 0.0f) ? width : ImGui::GetContentRegionAvail().x;
    f32 rowHeight   = ImGui::GetFrameHeight() + innerPadY;

    // Include left push so that selectable spans almost full width and indent is more noticeable
    f32 availableWidth = rowWidth - outerMargin * 2.0f;

    ImU32 hoverBgCol = ImGui::GetColorU32(ImGuiCol_HeaderHovered); 
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + outerMargin);

    ImGui::PushID(strId);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive,  ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_Header,        ImVec4(0,0,0,0));

    bool clicked = ImGui::Selectable("##row", selected, 0, ImVec2(availableWidth , rowHeight));
    bool hovered = ImGui::IsItemHovered();
    ImGui::PopStyleColor(3);
    ImGui::PopID();

    ImVec2 boxMin = ImGui::GetItemRectMin();
    ImVec2 boxMax = ImGui::GetItemRectMax();

    if (hovered){
        ImGui::GetWindowDrawList()->AddRectFilled(boxMin, boxMax, hoverBgCol, rounding);
    }

    // FIX 2: Draw the icon and offset the text so they don't overlap
    f32 textOffsetX = innerPadX;
    // push here so that indent is more percievable
    f32 iconSize = 16.0f * dpi; 
    if (icon) {
        f32 iconY = boxMin.y + (rowHeight - iconSize) * 0.5f;
        
        ImGui::GetWindowDrawList()->AddImage(icon, 
            ImVec2(boxMin.x + innerPadX + leftPush, iconY), 
            ImVec2(boxMin.x + innerPadX + leftPush + iconSize, iconY + iconSize));
            
    }
    // move the text forward regardless of icon or not 
    textOffsetX = innerPadX + iconSize + (spaceBetweenIconAndText * dpi); // Space between icon and text

    f32 textStartX = boxMin.x + textOffsetX + leftPush;
    f32 textWidth = ImGui::CalcTextSize(label).x;

    f32 maxTextWidth = boxMax.x - textStartX - innerPadX; // right padding
    std::string displayText = label;

    // Text Clipping
    if (textWidth > maxTextWidth){    
        displayText = "";
        f32 ellipsisWidth = ImGui::CalcTextSize("...").x;
        for (int i = 0; label[i] != '\0'; i++){
            displayText += label[i];
            if (ImGui::CalcTextSize(displayText.c_str()). x > maxTextWidth - ellipsisWidth){
                displayText.pop_back();
                displayText += "...";
                break;
            }
        }
    }   
    ImGui::GetWindowDrawList()->AddText(
        ImVec2(textStartX, boxMin.y + (rowHeight - ImGui::GetTextLineHeight()) * 0.5f),
        ImGui::GetColorU32(ImGuiCol_Text), displayText.c_str());

    // Restore the Cursor X position before moving down, otherwise the next row will be shifted even further
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, style.itemSpacing.y * dpi));
    ImGui::Spacing();
    ImGui::PopStyleVar();
    return clicked;
}


bool UI::Helpers::RenderSectionHeader(const char* id, const char* label, f32 dpi, bool* isOpen, MenuRowStyle style, f32 width){
    f32 outerMargin = style.outerMargin * dpi;
    f32 innerPadX   = style.innerPad.x * dpi;
    f32 innerPadY   = style.innerPad.y * dpi;
    f32 rounding    = style.rounding * dpi;
    f32 rowWidth    = (width > 0.0f) ? width : ImGui::GetContentRegionAvail().x;
    f32 rowHeight   = ImGui::GetFrameHeight() + innerPadY;
    
    ImU32 hoverBgCol = ImGui::GetColorU32(ImGuiCol_HeaderHovered); 
    // (Or use a specific color: ImGui::GetColorU32(ImVec4(1,1,1,0.1f)); )

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + outerMargin);
    
    ImGui::PushID(id);
    
    // Override Selectable colors to transparent so the default box doesn't show
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive,  ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_Header,        ImVec4(0,0,0,0));

    bool clicked = ImGui::Selectable("##row", false, 0, ImVec2(rowWidth - outerMargin * 2.0f, rowHeight));
    bool hovered = ImGui::IsItemHovered();
    
    ImGui::PopStyleColor(3);
    ImGui::PopID();

    ImVec2 boxMin = ImGui::GetItemRectMin();
    ImVec2 boxMax = ImGui::GetItemRectMax();

    // Draw the custom background using the color we saved earlier
    if (hovered){
        ImGui::GetWindowDrawList()->AddRectFilled(boxMin, boxMax, hoverBgCol, rounding);
    }

    f32 textEnd = boxMin.x + innerPadX + ImGui::CalcTextSize(label).x;
    // Draw Label via DrawList
    ImGui::GetWindowDrawList()->AddText(
        ImVec2(boxMin.x + innerPadX, boxMin.y + (rowHeight - ImGui::GetTextLineHeight()) * 0.5f),
        ImGui::GetColorU32(ImGuiCol_Text), label);

    // Draw Chevron right aligned
    const char* chev = *isOpen ? ICON_REG_CHEVRON_DOWN : ICON_REG_CHEVRON_RIGHT;
    f32 chevW = ImGui::CalcTextSize(chev).x;
    f32 chevStart = boxMax.x - innerPadX - chevW;

    // make sure, we dont draw on top of the main text
    if (chevStart > textEnd){
        ImGui::GetWindowDrawList()->AddText(
            ImVec2(boxMax.x - innerPadX - chevW, boxMin.y + (rowHeight - ImGui::GetTextLineHeight()) * 0.5f),
            ImGui::GetColorU32(ImGuiCol_Text), chev);
    }

    if (clicked) *isOpen = !*isOpen;
    
    // Advance cursor for the next item
    ImGui::Dummy({0.0f, style.itemSpacing.y * dpi});
    return clicked;
}

bool UI::Helpers::IsItemHoveredWithDelay(f32 delaySeconds) {
    if (!ImGui::IsItemHovered()) {
        return false;
    }

    ImGuiID currentItemId = ImGui::GetItemID();
    static ImGuiID s_lastHoveredId = 0;
    static double s_hoverStartTime = 0.0;

    if (s_lastHoveredId != currentItemId) {
        s_lastHoveredId = currentItemId;
        s_hoverStartTime = ImGui::GetTime();
    }

    return (ImGui::GetTime() - s_hoverStartTime) >= delaySeconds;
}