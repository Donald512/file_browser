// core.h
#pragma once

#include <Windows.h>
// #include <string>
#include <strsafe.h>
#include <tchar.h>
#include "imgui.h"
#include <stdint.h>
#include "iconFilled.h"
#include "iconRegular.h"
#include <ShlObj.h>
#include <d3d11.h>
#include <KnownFolders.h>
#include <Shlwapi.h>
// #include "imgui_boilerplate.h"

// todo prefix variables used by Windows Wide function by tmp_
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "Ole32.lib")


using f32 = float;
using i16 = int16_t;
using u16 = uint16_t;
using u64 = uint64_t;
using i32 = int32_t;
using i64 = int64_t;
using u8 = uint8_t;


#define PrntErr(msg) printf("Error: %s, File: %s, Line: %u\n", msg, __FILE__, __LINE__)
#define PrntErrIsNULL printf("Already NULL, File: %s, Line: %u\n", __FILE__, __LINE__)
#define PrntErrInvalid printf("Invalid move, File: %s, Line: %u\n", __FILE__, __LINE__)

struct AppContext;

namespace Str{
    struct String{
        char* data = nullptr;
        u64 length = 0;
        u64 capacity = 0;
    };  // 24 bytes in total

    String Create(const char* strLiteral);
    void Free(String& target);
    String Clone(const String& source);
    char* WideToUtf8(const wchar_t* wide);
    String WideToString(const wchar_t* wide);
    wchar_t* Utf8ToWide(const char* utf8, u64 extraCharCount = 0, u64* outNumWideChars = nullptr);

} // namespace String
struct BreadcrumbItem{
    Str::String displayName;    // E.g: "This PC" "Local Disk (C:)"
    PIDLIST_ABSOLUTE pidl = nullptr;
    // bool hasSubFolders = false;
};

struct BreadcrumbArray { // just a list of breadcrumbs
    BreadcrumbItem* breadcrumbs;
    Str::String fullPath;
    HICON pathIcon = 0;
    u64 count = 0;
    u64 capacity = 0;
    bool hasSubFolders = 0;

};
//
namespace Backend{    
    struct ShellItem{
        Str::String name; // 24
        PIDLIST_ABSOLUTE pidl = nullptr;    // 8
        u64 fileSize = 0;   // 8
        FILETIME lastWriteTime; // 8
        SFGAOF attributes = 0;  // 4
    };

    struct LightShellItem{   // just a stripped down version of ShellItem
        Str::String name; // 24
        PIDLIST_ABSOLUTE pidl = nullptr;    // 8
    };


    struct DirectoryArray{
        ShellItem* entries;  // array of ShellItems
        u64 numEntries;
        u64 capacity;
        
        i64 selectedIndex = -1;
    };

    struct LightShellItemArray{
        LightShellItem* entries;  // array of ShellItems
        u64 numEntries;
        u64 capacity;
    };

    void EnumerateDirectory(AppContext& ctx, PIDLIST_ABSOLUTE targetPidl);
    void FreeDirectoryArray(DirectoryArray& array);

    LightShellItemArray GetDirectoryContents(PIDLIST_ABSOLUTE targetPidl);      
    void FreeLightShellItemArray(LightShellItemArray& array);

    void GenerateBreadcrumbs(AppContext& ctx, PIDLIST_ABSOLUTE targetPidl);
    void FreeBreadcrumbs(BreadcrumbArray& array);

    Str::String GetTypeablePath(PIDLIST_ABSOLUTE targetPidl);
    bool PidlHasSubFolders(PCIDLIST_ABSOLUTE targetPidl);
    PIDLIST_ABSOLUTE CreatePidlFromPath(const wchar_t* widePath);
    bool ExecuteFile(PIDLIST_ABSOLUTE pidl);    // opens the file with the default app
    
}

struct PathHistory {
    PIDLIST_ABSOLUTE* visitedPidls;
    u64 count;      // How many paths are currently valid
    u64 capacity;   // Max size of the array E.g: 64 strings
    i64 currentIndex;  // Where the current directory is in history
};



// Global state container
struct AppContext{
    // Windows & Graphics
    HWND hwnd = nullptr;
    ID3D11Device* d3dDevice = nullptr;      // the device
    ID3D11DeviceContext* d3dContext = nullptr;  // the painter
    IDXGISwapChain* swapChain = nullptr;   
    ID3D11RenderTargetView* renderTargetView = nullptr;     // contains information abt which pixels to draw to 

    bool swapChainOccluded = false;
    UINT resizeWidth = 0;
    UINT resizeHeight = 0;

    // UI State
    f32 dpiScale = 1.0f;
    ImFont* mainFont = nullptr;
    ImFont* iconFont = nullptr;
    ImVec4 clearColor = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);

    // Navigation State
    PIDLIST_ABSOLUTE currentFolderPidl = nullptr;
    Backend::DirectoryArray currentDirArray{};
    PathHistory history{};
    BreadcrumbArray currentBreadcrumbs{};

    PIDLIST_ABSOLUTE popupCachePidl = nullptr;
    Backend::LightShellItemArray popupCacheList{};
};

namespace Utils{
    void InitCOM();
    void FreePidl(PIDLIST_ABSOLUTE& pidl);
}




namespace Navigation{
    bool NavigateTo(AppContext& ctx, PIDLIST_ABSOLUTE newPidl);    
    bool CanGoBack(AppContext& ctx);
    bool CanGoForward(AppContext& ctx);
    bool CanGoParent(AppContext& ctx);
    bool GoBack(AppContext& ctx);
    bool GoForward(AppContext& ctx);
    bool GoParent(AppContext& ctx);
} // namespace Navigation

namespace History {
    void Init(AppContext& ctx);
    bool Append(AppContext& ctx, PIDLIST_ABSOLUTE newPidl);
    void Destroy(AppContext& ctx);
}

namespace UI{
    constexpr ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse;
    void Render(AppContext& ctx);
}

namespace UI::Colors{
    constexpr ImVec4 WindowBackground(0.12f, 0.12f, 0.12f, 1.00f);
    constexpr ImVec4 TopBarBackground(0.1725f, 0.1725f, 0.1725f, 1.00f);
    constexpr ImVec4 SidebarBackground(0.14f, 0.14f, 0.14f, 1.00f);
}

namespace UI::Style{
    constexpr ImVec2 NoPadding(0.0f, 0.0f);
    constexpr ImVec2 AutoFillRemnantWindow(0.0f, 0.0f);
    constexpr f32 NoBorder = 0.0f;
    constexpr f32 NoRounding = 0.0f;
}

namespace TopBar{
    constexpr ImGuiWindowFlags Flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    constexpr f32 Height            = 32.0f;
    constexpr f32 TotalBtnsWidth    = 145.0f; // 145 is the distance from the vertical bar to the end of the screen
    constexpr f32 BtnHeight         = 30.0f;    // experiment
    constexpr f32 BtnWidth          = 45.0f;
    constexpr f32 CloseBtnWidth     = 46.0f;
    /*  minimize and maximize buttons are 45 wide x 32 tall, but 
    close button is 46 to account for the 1 px margin 
    */
    void Render(AppContext& ctx);
}


namespace ToolBar{
    constexpr f32 Height               = 48.0f;
    constexpr f32 LeftPadding          = 3.0f;
    constexpr f32 AddressToSearchGap = 8.0f;
    constexpr f32 RightPadding         = 11.0f;
    constexpr f32 AddressRatio         = 0.706f;
    constexpr f32 SearchRatio          = 0.294f;

    constexpr ImGuiWindowFlags Flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    void Render(AppContext& ctx);
}

namespace UI::Style::ToolBarLayout{
    constexpr f32 LeftPadding = 3.0f;   // distance from left window edge
    constexpr f32 AddressToSearchGap = 8.0f;
    constexpr f32 RightPadding = 11.0f;
    
    constexpr f32 AddressRatio = 0.706f;
    constexpr f32 SearchRatio = 0.294f;
}

namespace NavBar{
    constexpr f32 Width       = 198.0f;
    constexpr f32 Height      = 48.0f;
    constexpr f32 BtnSize      = 32.0f;
    constexpr f32 XPadding      = 16.0f;
    constexpr f32 ButtonStartX = 8.0f;


    void Render(AppContext& ctx);
}

namespace AddressBar{
    constexpr f32 Height = 32.0f;
    constexpr f32 BarStartX = UI::Style::ToolBarLayout::LeftPadding + NavBar::Width;
    void Render(AppContext& ctx);
}

namespace FileView{
    constexpr f32 IconSize    = 48.0f;
    constexpr f32 XPadding    = 32.0f;
    void Render(AppContext& ctx);
}

	 