#define UNICODE
#define _UNICODE
#include "imgui.h"
#include <Windows.h>
#include <string>
#include <strsafe.h>
#include <tchar.h>
#include "imgui_impl_win32.h"
#include <d3d11.h>
#include "imgui_impl_dx11.h"

#pragma comment(lib, "user32.lib")

using f32 = float;
using u16 = uint16_t;
using u64 = uint64_t;
using u8 = uint8_t;

struct FileItem{
    TCHAR* name;
    bool isFolder;
};
namespace ui = ImGui;

struct directory_list{
    FileItem* start_of_file_items;
    u64 num_items;
};


// Data
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;

// Forward declarations of helper functions by imgui
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

//
directory_list* ReturnFilesInDir(const TCHAR* dirName);
void RenderFileExplorer(directory_list* dir_list);
HWND CreateMyOSWindow();
bool InitializeGraphicsAPI(HWND &window);
void InitializeImGui(HWND &window);
void ImGui_Backend_NewFrame();
void MyGraphicsAPI_PresentFrame();
void FreeDirectoryList(directory_list* dir_list);
void ShutdownImGui(HWND &window);

ImVec4 g_clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
TCHAR g_currentDir[MAX_PATH] = TEXT("C:");
directory_list* g_currentDirList = ReturnFilesInDir(g_currentDir);

int main(void){
    // 1. Setup phase (Runs ONCE)
    HWND window = CreateMyOSWindow(); 
    if (!window) return 1;

    if (!InitializeGraphicsAPI(window)) return 1;
    
    ::ShowWindow(window, SW_SHOWDEFAULT);
    ::UpdateWindow(window);


    InitializeImGui(window);
    
    // 2. The Main Loop (Runs continuously at 60+ frames per second)
    bool running = true;
    while (running) {
        
        // A. Handle Windows events (clicks, closes, moves)
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) running = false;
        }

        g_SwapChainOccluded = false;

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        ImGui_Backend_NewFrame();
        ImGui::NewFrame();

        // C. Call YOUR code!
        RenderFileExplorer(g_currentDirList);

        // D. Hand the instructions over to your graphics card to paint pixels
        ImGui::Render();
        MyGraphicsAPI_PresentFrame(); 
    }

    // 3. Cleanup phase (Runs ONCE when exiting)
    FreeDirectoryList(g_currentDirList);
    ShutdownImGui(window);
    return 0;
}


directory_list* ReturnFilesInDir(const TCHAR* dirName){   // todo IM so confused onn what text encoding i currently am

    _tprintf(TEXT("%s\n"), dirName);
    directory_list* dir_list = (directory_list*) malloc(sizeof(directory_list));
    if (!dir_list){
        _tprintf(TEXT("Malloc failed to alloc for directory_list.\n"));
    }

    WIN32_FIND_DATA ffd;
    LARGE_INTEGER filesize;
    TCHAR szDir[MAX_PATH];
    size_t length_of_path;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    DWORD dwError = 0;
    u64 fileCount = 0;

    // Check that the input path plus 3 is not longer than MAX_PATH.
    // Three characters are for the "\*" plus NULL appended below.

    StringCchLength(dirName, MAX_PATH, &length_of_path);
    if (length_of_path > MAX_PATH - 3){
        _tprintf(TEXT("Directory path is too long: %s\n"), dirName);
        assert(0);
    }

    StringCchCopy(szDir, MAX_PATH, dirName);
    StringCchCat(szDir, MAX_PATH, TEXT("\\*"));

    hFind = FindFirstFile(szDir, &ffd);
    if (hFind == INVALID_HANDLE_VALUE){
        dwError = GetLastError();
        printf("%lu\n", dwError);
        assert(0);
    }
    
    do{
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) continue; // its a fake folder and will break the code
        fileCount++;
    }   while(FindNextFile(hFind, &ffd));
    
    dwError = GetLastError();
    if (dwError != ERROR_NO_MORE_FILES){
        printf("%lu\n", dwError);
        printf("%p\n", hFind);
        assert(0);
    }

    dir_list->start_of_file_items = (FileItem*) malloc(sizeof(FileItem) * fileCount);
    dir_list->num_items = fileCount;

    FindClose(hFind);

    hFind = FindFirstFile(szDir, &ffd);
    u64 i = 0;
    do{
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) continue; // its a fake folder and will break the code
        FileItem currentItem;
        u64 stringLength;
        StringCchLength(ffd.cFileName, MAX_PATH, &stringLength);
        TCHAR* fileName = (TCHAR*)malloc(sizeof(TCHAR) * stringLength + 1 /*For \0*/);
        StringCchCopy(fileName, stringLength + 1, ffd.cFileName);
        currentItem.name = fileName;
        currentItem.isFolder = (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        dir_list->start_of_file_items[i] = currentItem;
        i++;
    }   while (FindNextFile(hFind, &ffd) != 0);

    FindClose(hFind);

    return dir_list;
}

void RenderFileExplorer(directory_list* dir_list){
    
    // 1. Setup a main window panel
    ImGui::SetNextWindowPos(ImVec2(0, 0));  // Snap to top left
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);   // fill the canvas

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
    if (ImGui::Begin("File Explorer"), NULL, flags){  // Starts a window with this name, returns true if window is visible and rendered 
        
        // Top control bar ( could add back buttons, search paths here later)
        ImGui::Text("Directory Content:");
        ImGui::Separator(); // Draws a horizontal line 

        // 2. Create a Scrolling region for the files
        ImGui::BeginChild("FileViewRegion", ImVec2(0, 0), ImGuiChildFlags_Borders, ImGuiChildFlags_NavFlattened);  
        // Starts a Child Window with ID "FileViewRegion", 2nd param is size of the child region - ImVec2(0, 0) means fill all available space, ImGuiChildFlags_Borders adds a visible border around the child, 

        // Define standard item size configs
        const f32 iconSize = 48.0f;
        const f32 cellWidth = iconSize + 32.0f; // Horizontal padding 

        f32 availWidth = ImGui::GetContentRegionAvail().x;    // Calculate how much width is available currently

        // Determine how many columns can fit based on width. Minimum 1 column
        u16 columnsCount = availWidth / cellWidth;
        columnsCount = columnsCount ? columnsCount: 1;  // if columnsCount is 0, make it 1

        // 3. Create a layout grid using Tables
        // ImGuiTableFlags_NoSavedSettings stops ImGui from remembering column settings between runs
        if (ImGui::BeginTable("ExplorerGrid", columnsCount, ImGuiTableFlags_NoSavedSettings)){
            for (size_t i = 0; i < dir_list->num_items; i++){
                ImGui::TableNextColumn();
                FileItem currentItem = dir_list->start_of_file_items[i];

                // Group makes sure the item acts as a single cohesize block for interaction
                ImGui::BeginGroup();
                // - DRAW THE GRAPHICS ICON -
                // todo Replace the text icons with actual textures (via ImTextureID) 
                ImVec2 startCursorPos = ImGui::GetCursorScreenPos();

                if (currentItem.isFolder){
                    // Folder Graphic placeholder: Tinted Yellow button 
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.65f, 0.2f, 1.0f));    // Background color that RGBA color 
                    ImGui::Button(("[F]##" + std::to_string(i)).c_str(), ImVec2(iconSize, iconSize)); 
                    // CLickable square button on the screen with visible text [F], everuthing after ## is hidden from the user but used by ImGui as a unique id, .c_str() converts from std::string to string literal, imvec2(iconsize, iconsize) makes it a perfect square
                    ImGui::PopStyleColor(); // makes the current button colorer the default style color, so other buttons dont inherit the same color
                }
                else{
                    // File Graphic placeholder: Tinted Blue/Grey Button
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.7f, 1.0f));
                    ImGui::Button(("[#]##" + std::to_string(i)).c_str(), ImVec2(iconSize, iconSize));
                    ImGui::PopStyleColor();
                }

                // - DRAW TEXT BELOW ICON -
                // Center alignment math for text strings within the defined column grid block
                char Buffer[MAX_PATH];
                WideCharToMultiByte(CP_UTF8, 0, currentItem.name, -1, Buffer, sizeof(Buffer), NULL, NULL);
                f32 textWidth = ImGui::CalcTextSize(Buffer).x;  // todo convert wide to utf8
                if (textWidth < cellWidth){
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (iconSize - textWidth) * 0.5f);
                }

                // Wraps text smoothly if name exceeds column size limit
                ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + cellWidth);
                ImGui::Text("%s", Buffer);
                ImGui::PopTextWrapPos();

                ImGui::EndGroup();

                // - INTERATION HANDLING -
                if (ImGui::IsItemHovered()){
                    ImGui::SetTooltip("Type: %s", currentItem.isFolder? "Folder" : "File");
                }
                if (ImGui::IsItemClicked(ImGuiMouseButton_Left)){

                }
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && (ImGui::IsItemHovered())){
                    if (currentItem.isFolder){
                        StringCchCat(g_currentDir, MAX_PATH, TEXT("\\"));
                        StringCchCat(g_currentDir, MAX_PATH, currentItem.name);

                        FreeDirectoryList(g_currentDirList);
                        g_currentDirList = ReturnFilesInDir(g_currentDir);
                        break; // ! very very important
                        
                    }
                }
            }
            ImGui::EndTable();
        }
        ImGui::EndChild();
        
        ImGui::End();
    }
}


HWND CreateMyOSWindow(){
    // Make process DPI aware
    ImGui_ImplWin32_EnableDpiAwareness();

    // Obtain main monitor scale
    f32 main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));
    
    // Create application window
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, TEXT("File Browser Window"), nullptr };
    if (::RegisterClassExW(&wc) == 0){
        printf("Register Class failed");
        return false;
    }

    // DWORD dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    HWND hwnd = ::CreateWindow(wc.lpszClassName, TEXT("File Browser"), WS_OVERLAPPEDWINDOW, 100, 100, (int)(1280 * main_scale), (int)(800 * main_scale), nullptr, nullptr, wc.hInstance, nullptr);
    
    return hwnd;
}

bool InitializeGraphicsAPI(HWND &window){
    // Initialize Direct3D
    if (!CreateDeviceD3D(window))
    {
        CleanupDeviceD3D();
        // todo ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return false;
    }
    return true;
}

void InitializeImGui(HWND &window){
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls


    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup scaling
    // todo f32 main_scale is repeated in CreateMyOSWindow
    f32 main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

}

void ImGui_Backend_NewFrame(){
    // B. Tell ImGui you are starting a new frame
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
}

void MyGraphicsAPI_PresentFrame(){
    // Rendering
    // 1. Calculate the raw triangle data (Step D placeholder 1)
    ImGui::Render();

    // 2. Prep your clear color (handles alpha blending math)
    ImVec4 clear_color = g_clear_color;
    const float clear_color_with_alpha[4] = {clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };

    // 3. Tell your GPU to target your main window view
    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);

    // 4. Wipe the previous frame's pixels off the screen using your clear color
    g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);

    // 5. Hand the calculated ImGui triangles over to DirectX 11 to draw them
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    HRESULT hr = g_pSwapChain->Present(1, 0); // 1 = Lock to your monitor's VSync refresh rate
    g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
}

void FreeDirectoryList(directory_list* dir_list){
    if (!dir_list) return;
    // Free every string allocated
    for (u64 i = 0; i < dir_list->num_items; i++){
        free(dir_list->start_of_file_items[i].name);
    }
    free(dir_list->start_of_file_items);
    free(dir_list);
}

void ShutdownImGui(HWND &window){
    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(window);
    // todo ::UnregisterClassW(wc.lpszClassName, wc.hInstance);


}

// ! cl main4.cpp imgui\imgui.cpp imgui\imgui_tables.cpp imgui\imgui_widgets.cpp imgui\imgui_draw.cpp
// !backends\imgui_impl_dx11.cpp

// Helper functions

bool CreateDeviceD3D(HWND hWnd){
    // Setup swap chain
    // This is a basic setup. Optimally could use e.g. DXGI_SWAP_EFFECT_FLIP_DISCARD and handle fullscreen mode differently. See #8979 for suggestions.
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D(){
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget(){
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// todo find out what the fuck this is later
// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
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

