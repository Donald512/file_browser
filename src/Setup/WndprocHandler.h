#pragma once

#include <Windows.h>
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "BasicTypes.h"
#include <cstdio>
#include <windowsx.h>
#include "App.h"
#include "imgui_fonts.h"
#include "theme.h"

#include "UI/global.h"

inline bool g_dpiChanged = false;
inline bool g_dpiFromWndproc = 1.0f;

bool g_isResizing = false;
bool g_isMinimized = false;

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam){
    // Get the pointer if it's already been set
    App* app = reinterpret_cast<App*>(::GetWindowLongPtrW(hWnd, GWLP_USERDATA));

    //  If it hasn't been set yet, look for the creation message
    if (msg == WM_NCCREATE) {
        CREATESTRUCTW* pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
        app = reinterpret_cast<App*>(pCreate->lpCreateParams);
        
        // Glue the pointer directly onto this specific HWND instance
        ::SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    }

    // Safety check: early messages might fly by before WM_NCCREATE completes
    if (app == nullptr) {
        return ::DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg){
    case WM_NCLBUTTONDOWN:{ // when user presses one of the caption buton regions, windows generates a WM_NCLBUTTONDOWN message, this is handle to prevent the retro white box from rendering
        switch(wParam){
        case HTMINBUTTON:{
            ::PostMessage(hWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
            return 0; // tell windows i completely handled the message, so it doesnt render the accessibilty box
        }
        case HTMAXBUTTON:{
            ::IsZoomed(hWnd) ? ::PostMessage(hWnd, WM_SYSCOMMAND, SC_RESTORE, 0) : ::PostMessage(hWnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
            return 0;
        }
        case HTCLOSE:{
            ::PostMessage(hWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
            return 0;
        }
        break;
        }
    }   break;
    case WM_NCCALCSIZE :{
        if (wParam == TRUE){
            // lParam points to an NCCALCSIZE_PARAMS structure when wParam is TRUE
            NCCALCSIZE_PARAMS* pParams = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);

            // If the window is maximized, clamp its drawing area to the monitor's exact work area
            // This prevents Windows from pushing the top 8 pixels off the screen!
            if (::IsZoomed(hWnd)) {
                HMONITOR hMonitor = ::MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
                MONITORINFO mi = { sizeof(mi) };
                if (::GetMonitorInfoW(hMonitor, &mi)) {
                    pParams->rgrc[0] = mi.rcWork; // rcWork respects the Windows Taskbar
                }
            }
            return 0;   // Keep returning 0 to remove the default ugly title bar
        }
    } break;
    case WM_DPICHANGED:{
        UINT newDpi = HIWORD(wParam);
        g_dpiChanged = true;
        g_dpiFromWndproc = (f32) newDpi / 96.0f;

        RECT* prcNewWindow = reinterpret_cast<RECT*>(lParam);
        ::SetWindowPos(hWnd, nullptr, prcNewWindow->left, prcNewWindow->top,  prcNewWindow->right - prcNewWindow->left, prcNewWindow->bottom - prcNewWindow->top, SWP_NOZORDER | SWP_NOACTIVATE);
    }   break;
    case WM_NCHITTEST :{

        // 1. Get the screen mouse positions from lParam
        POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)}; // LOWORD lparam is mouse x and HIWORD lparam is mouse y
        ::ScreenToClient(hWnd, &pt);

        // window dimensions
        RECT rect;
        ::GetClientRect(hWnd, &rect);

        
        f32 topBarHeight = 32.0f * app->ui.dpi;
        LONG borderThickness = (LONG) ( 8.0 * app->ui.dpi);
        
        RECT innerRect = {borderThickness, borderThickness, rect.right - borderThickness, rect.bottom - borderThickness};

        // only resize if not maximized
        if (!::IsZoomed(hWnd)){
            // must check diagonals first
            if (pt.x < innerRect.left && pt.y < innerRect.top) return HTTOPLEFT;
            else if (pt.x >= innerRect.right && pt.y < innerRect.top) return HTTOPRIGHT;
            else if (pt.x < innerRect.left && pt.y >= innerRect.bottom) return HTBOTTOMLEFT;
            else if (pt.x >= innerRect.right && pt.y >= innerRect.bottom) return HTBOTTOMRIGHT;
    
            // check straight edges
            else if (pt.x < innerRect.left) return HTLEFT;
            else if (pt.y < innerRect.top) return HTTOP;
            else if (pt.x >= innerRect.right) return HTRIGHT;
            else if (pt.y >= innerRect.bottom) return HTBOTTOM;
        }
        
        if (pt.y < topBarHeight){

        if (g_HitTestRegistry.IsOverClientElement(ImVec2((f32)pt.x, (f32)pt.y))) {
            return HTCLIENT; // Pass control to ImGui inputs immediately
        }

            f32 windowWidth = (f32)(rect.right - rect.left);
            f32 clsBtnWidth = 46.0f;
            f32 maxBtnWidth = 45.0f;
            f32 minBtnWidth = 45.0f;
            f32 controlClusterWidth = (clsBtnWidth + maxBtnWidth + minBtnWidth) * app->ui.dpi;
            f32 buttonStartX = windowWidth - controlClusterWidth;   // 136px total
            
            if (pt.x >= buttonStartX ){
                f32 captionRelativePosition = pt.x - buttonStartX;

                if (captionRelativePosition < minBtnWidth * app->ui.dpi){
                    return HTMINBUTTON;     // minimize
                }
                else if (captionRelativePosition < (minBtnWidth + maxBtnWidth) * app->ui.dpi){
                    return HTMAXBUTTON;     // maximize
                }
                else{
                    return HTCLOSE;
                }
            }

            // else tell windows this is the title bar
            return HTCAPTION;
        }
        return HTCLIENT;    // todo check is this is redundant
    } 
    break;
    // case WM_GETMINMAXINFO: {
    //     MINMAXINFO* mmi = (MINMAXINFO*)lParam;
    //     mmi->ptMinTrackSize.x = sidebarWidth * app->ui.dpi + 300 /*random padding*/ + CAPTIONBTNSWIDTH * app->ui.dpi; // Set desired minimum width
    //     mmi->ptMinTrackSize.y = 600; 
    //     return 0;
    // }

    case WM_ENTERSIZEMOVE:{
        g_isResizing = true;
        break;
    }
    case WM_EXITSIZEMOVE:{
        g_isResizing = false;
        break;
    }

    case WM_SIZE: {
        if (wParam == SIZE_MINIMIZED) {
            return 0; // Do not resize swapchain to 0x0
        }
        
        UINT width = LOWORD(lParam);
        UINT height = HIWORD(lParam);
        
        if (width == 0 || height == 0) {
            return 0; // Extra safety against 0x0 resizes
        }
        
        app->gfx.resizeWidth = width;
        app->gfx.resizeHeight = height;
        
        RECT rect;
        ::GetClientRect(app->gfx.hwnd, &rect);
        app->gfx.width = (f32)(rect.right - rect.left);
        app->gfx.height = (f32)(rect.bottom - rect.top);
        
        // Forces a redraw *during* the drag, instead of waiting for you to let go
        InvalidateRect(app->gfx.hwnd, NULL, FALSE);
        
        return 0;
    }

    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

// TODO: Replace WaitMessage() with MsgWaitForMultipleObjects 
// and a 1-second Linger Timer to fix text cursor blinking and UI fade animations.

/*
Background Threading: Loading a folder with 10,000 files without freezing the UI.
Thumbnail Generation: Extracting icons and images for files efficiently.
File Operations: Copy, Paste, Delete, and handling Windows permission errors gracefully.
Navigation State: Handling drag-and-drop, tree-view expansion, and complex path parsing.
*/