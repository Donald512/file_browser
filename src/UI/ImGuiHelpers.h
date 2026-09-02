#pragma once

#include "BasicTypes.h"
#include "imgui_internal.h"
#include "imgui.h"
#include "theme.h"

#include <cfloat>
#include <cmath>
#include <vector>


struct HitTestRegistry {
    // Stores screen-space bounding boxes of UI elements that accept clicks
    std::vector<ImRect> clientElements;

    void Clear() { 
        clientElements.clear(); 
    }    
    
    void RegisterElement(const ImRect& rect) { 
        clientElements.push_back(rect); 
    }    

    bool IsOverClientElement(const ImVec2& mousePos) const {
        for (const auto& rect : clientElements) {
            if (rect.Contains(mousePos)) return true;
        }    
        return false;
    }    
};    


inline void AnimateFloat(f32& current, f32 target, f32 deltaTime, f32 speed){
    if (deltaTime <= 0.0f){
        current = target;
        return;
    }    
    const f32 alpha = 1.0f - std::exp(-deltaTime * speed);
    current += (target - current) * alpha;
    if (std::abs(target - current) < 0.25f) current = target;
}   

struct AnimatedFloat {
    f32 value = 0.0f;
    f32 speed = 16.0f;

    explicit AnimatedFloat(f32 startValue = 0.0f, f32 animSpeed = 16.0f)
        : value(startValue), target(startValue), speed(animSpeed), initialized(true) {}

    // Normal animation path
    void SetTarget(f32 newTarget) {
        target = newTarget;
        if (!initialized) {
            value = target;
            initialized = true;
        }
    }

    // Immediate path (for dragging resize handles)
    void SetValue(f32 newValue) {
        value = newValue;
        target = newValue; // Keep target in sync so it doesn't animate away when released
        initialized = true;
    }

    void Update(f32 deltaTime) {
        if (!initialized) { value = target; initialized = true; }
        AnimateFloat(value, target, deltaTime, speed);
    }

    void SnapToTarget() { value = target; }

    // Explicit is generally safer in C++ to prevent accidental implicit 
    // conversions in overloaded functions, but implicit is fine if you prefer brevity.
    explicit operator f32() const { return value; } 
    
    // If you use 'explicit', you'll need a getter for math:
    f32 Get() const { return value; }

private:
    f32 target = 0.0f;
    bool initialized = false;
};


// align {0,0.5} = left, {0.5,0.5} = center, {1,0.5} = right.
inline ImVec2 DrawTextSingleLine(ImDrawList* dl, const ImVec2& rectMin, const ImVec2& rectMax, const char* text, ImU32 color, ImVec2 align = ImVec2(0.5f, 0.5f), f32 fontSize = 0.0f){
    if (!dl || !text) return ImVec2(0.0f, 0.0f);

    ImFont* font = ImGui::GetFont();
    if (fontSize <= 0.0f) fontSize = ImGui::GetFontSize();

    ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, text);

    ImVec2 pos(
        rectMin.x + ImMax(0.0f, (rectMax.x - rectMin.x - textSize.x) * align.x),
        rectMin.y + ImMax(0.0f, (rectMax.y - rectMin.y - textSize.y) * align.y)
    );    

    dl->AddText(font, fontSize, pos, color, text);
    return textSize;
}    



// a long piece of text will not wrap, or be clipped, it completely overlaps into neighboring regions
inline void DrawTextCenteredSingleLine( ImDrawList* dl, const ImVec2& rectMin, const ImVec2& rectMax, const char* text, ImU32 color, f32 fontSize = 0.0f){
    DrawTextSingleLine(dl, rectMin, rectMax, text, color, ImVec2(0.5f, 0.5f), fontSize);
}    

// Left-aligned at explicit x, vertically centered. Returns size for chaining.
inline ImVec2 DrawTextAtX( ImDrawList* dl, f32 x, const ImRect& rect, const char* text, ImU32 color, f32 fontSize = 0.0f){
    ImVec2 min = rect.Min;
    min.x = x;
    return DrawTextSingleLine(dl, min, rect.Max, text, color, ImVec2(0.0f, 0.5f), fontSize);
}    

inline void DrawTextEllipsisSingleLine( ImDrawList* dl, const ImRect& rect, const char* text, ImU32 color, f32 fontSize = 0.0f){
    if (!dl || !text || rect.Max.x <= rect.Min.x) return;

    ImFont* font = ImGui::GetFont();
    if (fontSize <= 0.0f) fontSize = ImGui::GetFontSize();

    const ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, text);
    const ImVec2 textMin(
        rect.Min.x,
        rect.Min.y + ImMax(0.0f, (rect.GetHeight() - textSize.y) * 0.5f)
    );    

    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::RenderTextEllipsis(dl, textMin, rect.Max, rect.Max.x, text, nullptr, &textSize);
    ImGui::PopStyleColor();
}    


struct RowState{
    bool pressed = false;
    bool hovered = false;
    bool held    = false;
    ImRect rect;
};    

inline RowState RenderRow( const char* id, f32 height, ImU32 hoverCol, ImU32 activeCol, f32 insetX = 4.0f, f32 rounding = 4.0f){
    RowState s;
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return s;

    const f32 width  = ImGui::GetContentRegionAvail().x;
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    s.rect = ImRect(pos, ImVec2(pos.x + width, pos.y + height));

    ImGui::PushID(id);
    s.pressed = ImGui::InvisibleButton("row", ImVec2(width, height));
    s.hovered = ImGui::IsItemHovered();
    s.held    = ImGui::IsItemActive();
    ImGui::PopID();

    ImDrawList* dl = window->DrawList;
    const ImVec2 bMin(s.rect.Min.x + insetX, s.rect.Min.y + 1.0f);
    const ImVec2 bMax(s.rect.Max.x - insetX, s.rect.Max.y - 1.0f);

    if (s.held)         dl->AddRectFilled(bMin, bMax, activeCol, rounding);
    else if (s.hovered) dl->AddRectFilled(bMin, bMax, hoverCol,  rounding);

    return s;
}    

inline f32 CenterY(const ImRect& rect, f32 itemHeight){
    return rect.Min.y + (rect.GetHeight() - itemHeight) * 0.5f;
}    


// ---------------------------------------------------------------------------
// Shared primitives
//
// These three cover the patterns that were being hand-rolled slightly
// differently in every view (FileView.h, Sidebar.h, CmndBar.h). Prefer these
// over reimplementing hit-testing, hover/selected fills, or clipped grid
// loops locally.
// ---------------------------------------------------------------------------

// Standardized hit-testing for a rect that already has an ImGuiID (or one built by me e.g. window->GetID(...) or window->GetID((void*)index)).
// This is just ItemAdd + ButtonBehavior named and packaged - it doesn't replace ImGui::InvisibleButton (which already does the same thing when you
// want ImGui to also own cursor placement), it's for the places that were
// doing ItemAdd/ButtonBehavior by hand with raw bools.
struct Interaction {
    bool hovered = false;
    bool held    = false;
    bool pressed = false;
};

inline Interaction MakeInteractive(ImGuiID id, const ImRect& rect){
    Interaction it{};
    // ItemAdd returns true ONLY if the item is inside the visible clip rect
    const auto extraFlags = (int)ImGuiItemFlags_NoNav | (int)ImGuiItemFlags_NoNavDisableMouseHover;
    // So, Adding ImGuiItemFlags_NoNav because else, it causes undeterministic behaviour, imgui internal focus engine moves the hover 
    // So, Adding ImGuiItemFlags_NoNavDisableMouseHover because else, it causes the annoying behavior of where the item appears to lose focus, when the keyboard is used
    if (ImGui::ItemAdd(rect, id, nullptr, extraFlags)) {
        it.pressed = ImGui::ButtonBehavior(rect, id, &it.hovered, &it.held, ImGuiButtonFlags_NoNavFocus);
    }
    it.hovered = it.hovered && rect.Contains(ImGui::GetMousePos());
    return it;
}

inline Interaction MakeInteractive(const char* strID, const ImRect& rect){
    return MakeInteractive(ImGui::GetCurrentWindow()->GetID(strID), rect);
}

inline bool IsDoubleClick(ImGuiID id, bool clickedThisFrame){
    static ImGuiID lastId = 0;
    static double lastTime = -1.0;
    bool isDouble = false;
    if (clickedThisFrame){
        double now = ImGui::GetTime();
        isDouble = (id == lastId) && (now - lastTime) <= ImGui::GetIO().MouseDoubleClickTime;
        lastId = id;
        lastTime = now;
    }
    return isDouble;
}

// Fills `rect` with the theme's hover or selected color, or does nothing.
// Pass an already-inset rect if you want padding around the fill (see
// Sidebar.h's RenderItemRow for an example) - this only decides the color.
inline void DrawSelectableBg(ImDrawList* dl, const ImRect& rect, bool hovered, bool selected, f32 rounding = 0.0f){
    if (!hovered && !selected) return;
    ImU32 col = selected ? Theme::Current.palette.SurfaceActive:  Theme::Current.palette.SurfaceHover;
    dl->AddRectFilled(rect.Min, rect.Max, col, rounding);
}


inline bool RenderIconButton( const ImRect& rect, const char* idName, const char* icon, f32 rounding, ImU32 hoverCol, ImU32 activeCol, ImU32 textCol = Theme::Current.palette.Text, bool isDisabled = false){
    bool hovered = false;
    bool held = false;

    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    ImGuiID id = window->GetID(idName);

    if (!ImGui::ItemAdd(rect, id)) return false;
    bool pressed = ImGui::ButtonBehavior(rect, id, &hovered, &held);

    if (!isDisabled){
        if (held)           dl->AddRectFilled(rect.Min, rect.Max, activeCol, rounding);
        else if (hovered)   dl->AddRectFilled(rect.Min, rect.Max, hoverCol, rounding);
    }    

    if (isDisabled) textCol = Theme::Current.palette.TextDisabled;
    DrawTextCenteredSingleLine(dl, rect.Min, rect.Max, icon, textCol);

    if (isDisabled) return false;
    return pressed;
}    



inline void RenderHorizontalScrollbar(const char* idStr, ImRect trackRect, ImRect trackHoverRect, f32 contentWidth, f32 visibleWidth, f32& scrollOffset, f32 dpi, bool onlyShowIfHovered = true){
    if (contentWidth <= visibleWidth) {
        scrollOffset = 0.0f; // Reset if content fits
        return;
    }    

    ImVec2 mousePos = ImGui::GetMousePos();

    
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImGuiID id = window->GetID(idStr);

    // If we are actively dragging this scrollbar, do not hide it
    bool isDragging = (ImGui::GetActiveID() == id);
    
    if (onlyShowIfHovered && !trackHoverRect.Contains(mousePos) && !isDragging) return;

    // maximum posible scroll bounds, (total size - what we can see)
    f32 maxScroll = contentWidth - visibleWidth;
    scrollOffset = ImClamp(scrollOffset, 0.0f, maxScroll);

    f32 trackWidth = trackRect.GetWidth();
    f32 trackHeight = trackRect.GetHeight();

    // calculate thumb size and position
    f32 minThumbWidth = 24.0f * dpi;    // just a visual feauture, so its not too tiny to grab
    f32 thumbWidth = ImMax(minThumbWidth, (visibleWidth / contentWidth) * trackWidth); 
    f32 thumbX = trackRect.Min.x + (scrollOffset / maxScroll) * (trackWidth - thumbWidth);

    ImRect thumbRect(ImVec2(thumbX, trackRect.Min.y), ImVec2(thumbX + thumbWidth, trackRect.Max.y));

    
    ImGui::ItemAdd(thumbRect, id);

    // Handle dragging
    bool hovered, held;
    ImGui::ButtonBehavior(thumbRect, id, &hovered, &held);

    if (held){
        f32 mouseDeltaX = ImGui::GetIO().MouseDelta.x;
        if (mouseDeltaX != 0.0f){
            f32 grabSpace = trackWidth - thumbWidth;
            if (grabSpace > 0.0f){ // defensive check against divide by 0
                // convert pixel movement to scroll offset
                f32 scrollPerPixel = maxScroll / (trackWidth - thumbWidth);
                scrollOffset += mouseDeltaX * scrollPerPixel;
                scrollOffset = ImClamp(scrollOffset, 0.0f, maxScroll);  // prevent scrolling past bounds in each direction
            }    
        }    
    }    

    if (trackHoverRect.Contains(mousePos)){
        f32 wheelDeltaX = ImGui::GetIO().MouseWheelH;   // horizontal scrolling
        f32 wheelDeltaY = ImGui::GetIO().MouseWheel;    // vertical scrolling

        // Prioritize horizontal trackpad input, but fall back to vertical input 
        // if the user is hovering over the tab bar with a normal mouse wheel.
        f32 activeDelta = (wheelDeltaX != 0.0f) ? wheelDeltaX : wheelDeltaY;

        if (activeDelta != 0.0f){
            scrollOffset -= activeDelta * 30.0f * dpi;   // scroll speed
            scrollOffset = ImClamp(scrollOffset, 0.0f, maxScroll);
        }    

    }    

    ImDrawList* dl = window->DrawList;

    // draw faint track background. does not mess with theme
    dl -> AddRectFilled(trackRect.Min, trackRect.Max, IM_COL32(0, 0, 0, 40), trackHeight * 0.5f);

    // get thicker if hovered or held
    // f32 thickness = (hovered || held) ? trackHeight : trackHeight * 0.6f;
    f32 thickness = (trackRect.Contains(mousePos) || held) ? trackHeight : trackHeight * 0.6f;

    f32 yOffset = (trackHeight - thickness) * 0.5f;

    ImRect visualThumb(
        ImVec2(thumbRect.Min.x, trackRect.Min.y + yOffset),
        ImVec2(thumbRect.Max.x, trackRect.Max.y - yOffset)
    );    

    ImU32 thumbCol = held    ? Theme::Current.palette.SurfaceActive : 
                    hovered ? Theme::Current.palette.SurfaceHover : 
                           Theme::Current.palette.Border;
    dl->AddRectFilled(visualThumb.Min, visualThumb.Max, thumbCol, thickness * 0.5f);                       

}    

inline void ScrollTabIntoView(size_t tabIndex, f32 tabW, f32 gap, f32 availableWidth, f32& scrollOffset){
    f32 tabLeftRelative = tabIndex * (tabW + gap); // starting x coordinate of the tab
    f32 tabRightRelative = tabLeftRelative + tabW;  // ending x coordinate of the tab

    // if the tab is cut off on the right side, scroll right
    if (tabRightRelative - scrollOffset > availableWidth){
        scrollOffset = tabRightRelative - availableWidth;
    }    
    else if (tabLeftRelative - scrollOffset < 0.0f){
        scrollOffset = tabLeftRelative;
    }    
}    
 

inline f32 RenderResizeHorizontalHandle(const char* idStr, ImVec2 topLeftPos, f32 width, f32 height, ImU32 activeCol, ImU32 hoverCol){
    
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    
    ImVec2 min = topLeftPos;
    ImVec2 max = ImVec2(min.x + width, min.y + height);
    ImRect bb(min, max);
    
    ImGuiID id = window->GetID(idStr);

    ImGui::ItemAdd(bb, id);

    bool hovered = false;
    bool held = false;
    
    ImGui::ButtonBehavior(bb, id, &hovered, &held);

    if (hovered || held){
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW); // Horizontal arrows (←→)
    }    

    ImU32 color = 0; // Transparent by default
    
    if (held) {
        color = activeCol;
    }    
    else if (hovered) {
        color = hoverCol;
    }    

    if (color != 0) {
        dl->AddRectFilled(min, max, color);
    }    

    if (held) {
        return ImGui::GetIO().MouseDelta.x;
    }    

    return 0.0f;
}    

inline f32 RenderResizeVerticalHandle(const char* idStr, ImVec2 topLeftPos, f32 width, f32 handleHeight, ImU32 activeCol, ImU32 hoverCol){
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    
    // Calculate bounding box based on width and the thin handle height
    ImVec2 min = topLeftPos;
    ImVec2 max = ImVec2(min.x + width, min.y + handleHeight);
    ImRect bb(min, max);
    
    ImGuiID id = window->GetID(idStr);

    ImGui::ItemAdd(bb, id);

    bool hovered = false;
    bool held = false;
    
    ImGui::ButtonBehavior(bb, id, &hovered, &held);

    if (hovered || held) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS); // Vertical arrows (↑↓)
    }    

    ImU32 color = 0; // Transparent by default
    if (held) {
        color = activeCol;
    } else if (hovered) {
        color = hoverCol;
    }    

    if (color != 0) {
        window->DrawList->AddRectFilled(min, max, color);
    }    

    if (held) {
        return ImGui::GetIO().MouseDelta.y; // Track vertical mouse movement
    }    

    return 0.0f;
}    

inline void RenderVerticalScrollbar(const char* idStr, ImRect trackRect, ImRect trackHoverRect,
    f32 contentHeight, f32 visibleHeight, f32& scrollOffset, f32 dpi, bool onlyShowIfHovered = true){
    if (contentHeight <= visibleHeight){ scrollOffset = 0.0f; return; }
    ImVec2 mousePos = ImGui::GetMousePos();
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImGuiID id = window->GetID(idStr);
    bool isDragging = (ImGui::GetActiveID() == id);
    if (onlyShowIfHovered && !trackHoverRect.Contains(mousePos) && !isDragging) return;

    f32 maxScroll = contentHeight - visibleHeight;
    scrollOffset = ImClamp(scrollOffset, 0.0f, maxScroll);
    f32 trackH = trackRect.GetHeight();
    f32 trackW = trackRect.GetWidth();
    f32 minThumb = 24.0f * dpi;
    f32 thumbH = ImMax(minThumb, (visibleHeight / contentHeight) * trackH);
    f32 thumbY = trackRect.Min.y + (scrollOffset / maxScroll) * (trackH - thumbH);
    ImRect thumbRect(ImVec2(trackRect.Min.x, thumbY), ImVec2(trackRect.Max.x, thumbY + thumbH));

    ImGui::ItemAdd(thumbRect, id);
    bool hovered = false, held = false;
    ImGui::ButtonBehavior(thumbRect, id, &hovered, &held);
    if (held){
        f32 dy = ImGui::GetIO().MouseDelta.y;
        if (dy != 0.0f){
            f32 scrollPerPixel = maxScroll / (trackH - thumbH);
            scrollOffset = ImClamp(scrollOffset + dy * scrollPerPixel, 0.0f, maxScroll);
        }
    }
    if (trackHoverRect.Contains(mousePos)){
        f32 d = (ImGui::GetIO().MouseWheelH != 0.0f) ? ImGui::GetIO().MouseWheelH : ImGui::GetIO().MouseWheel;
        if (d != 0.0f) scrollOffset = ImClamp(scrollOffset - d * 30.0f * dpi, 0.0f, maxScroll);
    }

    ImDrawList* dl = window->DrawList;
    dl->AddRectFilled(trackRect.Min, trackRect.Max, IM_COL32(0, 0, 0, 40), trackW * 0.5f);
    f32 thickness = (trackRect.Contains(mousePos) || held) ? trackW : trackW * 0.6f;
    f32 xOff = (trackW - thickness) * 0.5f;
    ImRect visual(ImVec2(thumbRect.Min.x + xOff, thumbRect.Min.y), ImVec2(thumbRect.Max.x - xOff, thumbRect.Max.y));
    ImU32 col = held ? Theme::Current.palette.SurfaceActive
              : hovered ? Theme::Current.palette.SurfaceHover
                        : Theme::Current.palette.Border;
    dl->AddRectFilled(visual.Min, visual.Max, col, thickness * 0.5f);
}


inline int RenderTextWrappedCenteredEllipsis(ImDrawList* drawList, ImVec2 pos, ImVec2 size, const char* text, const char* textEnd = nullptr, int maxLines = 0) {
    if (!text || !*text) return 0;
    if (!textEnd) textEnd = text + strlen(text);

    ImFont* font = ImGui::GetFont();
    const f32 fontSize = ImGui::GetFontSize();
    const f32 lineHeight = fontSize;

    const char* s = text;
    int lineCount = 0;

    // Calculate max lines allowed by available vertical space if max_lines not set
    int maxAllowedLines = (maxLines > 0) ? maxLines : (int)(size.y / lineHeight); // truncates to prevent cutoff
    if (maxAllowedLines <= 0) maxAllowedLines = 1;
    while (s < textEnd) {

        // Check if this is the last line we can display
        bool isLastLine = (lineCount >= maxAllowedLines - 1);

        // Find where the word wrap occurs for this line
        const char* lineEnd = font->CalcWordWrapPositionA(1.0f, s, textEnd, size.x);

        // If s == lineEnd, a single word exceeds width; force advance at least 1 character
        if (lineEnd == s) lineEnd = s + 1; 

        // If this is the last line but there's still remaining text, draw with Ellipsis
        if (isLastLine && lineEnd < textEnd) {
            // Calculate starting X to center the line rect
            ImVec2 linePos(pos.x, pos.y + (lineCount * lineHeight));
            ImVec2 lineMax(pos.x + size.x, linePos.y + lineHeight);

            // Render line with trailing ellipsis (...)
            // void ImGui::RenderTextEllipsis(ImDrawList* draw_list, const ImVec2& pos_min, const ImVec2& pos_max, f32 ellipsis_max_x, const char* text, const char* text_end_full, const ImVec2* text_size_if_known)
            ImGui::RenderTextEllipsis(drawList, linePos, lineMax, lineMax.x, s, textEnd, nullptr);
            break; // Finished rendering (reached max lines)
        } 
        else {
            // Trim trailing spaces/newlines for cleaner center alignment
            const char* trimEnd = lineEnd;
            while (trimEnd > s && ImCharIsBlankA(trimEnd[-1])) {
                trimEnd--;
            }

            // Calculate actual text width of this line to center it
            f32 line_width = font->CalcTextSizeA(fontSize, FLT_MAX, size.x, s, trimEnd).x;
            f32 centerX = pos.x + (size.x - line_width) * 0.5f;

            // Draw line
            drawList->AddText(font, fontSize, ImVec2(centerX, pos.y + (lineCount * lineHeight)), Theme::Current.palette.Text, s, trimEnd);

            s = lineEnd;
            // Advance past standard line breaks
            while (s < textEnd && (*s == '\n' || *s == '\r')) s++;
            
            lineCount++;
        }
    }
    return lineCount;
}

inline void addSeparator(f32 padX, f32 width, f32 sepHeight){
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();

    f32 lineY = p.y + (sepHeight * 0.5f);
    
    ImU32 lineCol = Theme::Current.palette.Border;
    dl->AddLine(ImVec2(p.x + padX, lineY), ImVec2(p.x + width - padX, lineY), lineCol, 1.0f);

    // Advance layout cursor so ImGui auto-resizes correctly
    ImGui::Dummy(ImVec2(width, sepHeight)); 
}


enum class GrowAxis { X, Y };
enum class InputResult { Active, Committed, Cancelled };

struct AutoInputColors {
    ImVec4 bg          = ImVec4(0,0,0,0); // 0 alpha = use ImGui default
    ImVec4 border      = ImVec4(0,0,0,0);
    ImVec4 text        = ImVec4(0,0,0,0);
    ImVec4 selectionBg = ImVec4(0,0,0,0); // The highlight color when text is selected!
};

inline InputResult RenderAutoResizingInputText(const char* strId, ImVec2 pos, ImVec2 baseSize, ImVec2 maxSize, char* buffer, size_t bufferSize, GrowAxis axis, bool commitOnLostFocus, const AutoInputColors* colors, bool forceFocus){
    ImGui::SetCursorScreenPos(pos);
    ImGui::PushID(strId);

    // Compute vertical padding so that the internal text lands centered in baseSize.y to match DrawTextEllipsisSingleLine
    f32 lineH = ImGui::GetTextLineHeight();
    f32 hPad = 0;
    f32 vPad = ImMax(0.0f, (baseSize.y - lineH) * 0.5f);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(hPad, vPad));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);


    int colorPushCount = 0;
    if (colors) {
        if (colors->bg.w > 0)          { ImGui::PushStyleColor(ImGuiCol_FrameBg, colors->bg); colorPushCount++; }
        if (colors->border.w > 0)      { ImGui::PushStyleColor(ImGuiCol_Border, colors->border); colorPushCount++; }
        if (colors->text.w > 0)        { ImGui::PushStyleColor(ImGuiCol_Text, colors->text); colorPushCount++; }
        if (colors->selectionBg.w > 0) { ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, colors->selectionBg); colorPushCount++; }
    }

    // Calculate Box Size
    ImVec2 boxSize;
    if (axis == GrowAxis::Y){
        f32 wrapWidth = ImMax(baseSize.x - hPad * 2.0f, 1.0f);
        ImVec2 textSize = ImGui::CalcTextSize(buffer, nullptr, false, wrapWidth);
        f32 finalHeight = ImClamp(textSize.y + vPad * 2.0f + 4.0f, baseSize.y, maxSize.y);
        boxSize = { baseSize.x, finalHeight };
    } else {
        f32 textWidth = ImGui::CalcTextSize(buffer, nullptr, false, FLT_MAX).x;
        f32 finalWidth = ImClamp(textWidth + hPad * 2.0f + 8.0f, baseSize.x, maxSize.x);
        boxSize = { finalWidth, baseSize.y };
    }

    // 3. The "Select All" Callback (Nuclear option for Multiline)
    // We check if the widget is NOT currently active. If it's about to become active this frame,
    // the callback will fire and force SelectAll(), then disable itself.
    if (forceFocus) ImGui::SetKeyboardFocusHere();

    ImGuiID myId = ImGui::GetID("##input");
    struct CallbackData { bool justActivated; };
    CallbackData cbData{ ImGui::GetActiveID() != myId }; 
    
    auto callback = [](ImGuiInputTextCallbackData* data) -> int {
        if (data->EventFlag == ImGuiInputTextFlags_CallbackCharFilter) {
            if (data->EventChar == '\n' || data->EventChar == '\r') return 1; // reject
        }
        if (data->EventFlag == ImGuiInputTextFlags_CallbackAlways) {
            CallbackData* cd = (CallbackData*)data->UserData;
            if (cd->justActivated) {
                data->SelectAll();
                cd->justActivated = false; 
            }
        }
        return 0;
    };

    ImGui::SetNextItemWidth(boxSize.x);
    ImGuiInputTextFlags flags = ImGuiInputTextFlags_CallbackAlways | ImGuiInputTextFlags_CallbackCharFilter ;
    
    ImGui::InputTextMultiline("##input", buffer, bufferSize, boxSize, flags, callback, &cbData);
    ImVec2 reportedSize = ImGui::GetItemRectSize();

    bool isActive      = ImGui::IsItemActive();
    bool lostFocus     = ImGui::IsItemDeactivated();

    InputResult result = InputResult::Active;

    if (isActive){
        if (ImGui::IsKeyPressed(ImGuiKey_Enter, false) && !ImGui::GetIO().KeyShift) {
            result = InputResult::Committed;
        } 
        else if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            result = InputResult::Cancelled;
        }
    } 
    else if (lostFocus) {
        result = commitOnLostFocus ? InputResult::Committed : InputResult::Cancelled;
    }

    if (colorPushCount > 0) ImGui::PopStyleColor(colorPushCount);
    ImGui::PopStyleVar(3);
    
    ImGui::PopID();

    return result;
}