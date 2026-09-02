
/*
To make a custom context menu, we need to ask windows what right click menu items exist for this file
It returns a Menu Handle - HMENU and a Shell Object - IContextMenu

WalkMenu is a translator: It inspects the HMENU, Grabs the text, Checkmarks and icons and turns them into ContextMenuItems for the UI

fMask is just a checklist of Flags telling Windows what information you want back:
MIIM_FTYPE: Is this normal text or a seperator (MFT_SEPARATOR).
MIIM_STRING: Fill my text buffer with the item name (e.g., "Copy", "Delete").
MIIM_ID: Give me the Command ID (so we know what to run when clicked).
MIIM_SUBMENU: Tell me if hovering over this opens another menu (hSubMenu).
MIIM_STATE: Tell me if it's disabled/grayed out or checked (MFS_DISABLED, MFS_CHECKED).
MIIM_BITMAP: Give me the image handle (hbmpItem) if it has a simple static icon.

Some stupid extensions, refuse to give us their textures and prefer to draw by themselves, so when they draw it, we steal it and send them away till the menu closes and we ask them again

IContextMenu2 and IContextMenu3 were created by Microsoft to support custom drawn images and animations
    IContextMenu: "Provides the text and command IDs for this menu."
    IContextMenu2: "Handles basic custom drawing (WM_DRAWITEM)."
    IContextMenu3: "Handle drawing and fancy keyboard navigation (WM_MENUCHAR)."

For Owner drawn items, we just give them a 16 x 16 canvas to draw.
When an app like 7-Zip or Git adds a menu item, they mark it as MFT_OWNERDRAW or HBMENU_CALLBACK, which tells windows, "Dont read my icon handle, i will draw it myself"
To steal the icon:
    We create a blank invisible 16 x 16 canvas in RAM - HDC
    We call pcm2->HandleMenuMsg(WM_DRAWITEM, ...) and give them the canvas
    They draw on the invisible canvas, thinking its drawing on the screen
    We steal the pixels, then convert to a DirectX texture, ID3D11ShaderResourceView aka ImTextureID and throw the canvas away
*/
// ===================================================================
#include "CtxMenu.h"

#include <d3d11.h>
#include "Shell.h"
#include "Pidl.h"
#include <ShlObj.h>
#include <ShlObj_core.h>
#include "Textures.h"
#include <algorithm>
#include <iostream>
#include "ClipboardManager.h"
#include "Str.h"
#include "Item.h"
#include "App.h"

void ExecuteContextMenuCommand(App& app, ComPtr<IContextMenu> menu, PCIDLIST_ABSOLUTE parentPidl, std::vector<PCITEMID_CHILD>& childPidls, UINT idOffset, HWND ownerHwnd){
    if (!menu) return;

    // Ask the shell what verb this ID represents (e.g., "copy", "cut", "open")
    char verbBuf[256] = {};
    bool gotVerb = SUCCEEDED(menu->GetCommandString(idOffset, GCS_VERBA, nullptr, verbBuf, sizeof(verbBuf)));

    if (gotVerb) {
        std::string verb(verbBuf);
        std::transform(verb.begin(), verb.end(), verb.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        std::cout << verb << std::endl;
        for (auto pidl : childPidls){
            PIDLIST_ABSOLUTE fullPidl = GetFullPidl(parentPidl, pidl);
            std::cout << WShell::GetShellName(fullPidl, SIGDN_FILESYSPATH) << std::endl; 
            ILFree(fullPidl);
        }

        if (verb == "copy") {
            PerformClipboardOperation(parentPidl, childPidls, false);
            return; 
        }
        if (verb == "cut") {
            PerformClipboardOperation(parentPidl, childPidls, true);
            return; 
        }
        if (verb == "rename") {
            if (childPidls.size() == 1) {
                Tab& activeTab = app.window.GetActiveTab();

                auto& renameState = activeTab.renameState;
                auto& selState = activeTab.selState;

                if (!selState.focusHash.has_value()) return;
                renameState.renamingItemId = selState.focusHash;

                const DirChildren* PChildren = app.directory.Get(activeTab.dir.HChildren);
                if (!PChildren) return;
    
                for (u32 i = 0; i < PChildren->ItemCount(); i++){
                    if (PChildren->hashes[i] == selState.focusHash.value()){
                        strncpy(renameState.renameBuffer, PChildren->GetChildName(i), sizeof(renameState.renameBuffer) - 1);
                        renameState.renameBuffer[sizeof(renameState.renameBuffer) - 1] = '\0';
                        break;
                    }
                }
            }
            return;
        }
    }

    CMINVOKECOMMANDINFOEX info{};
    info.cbSize = sizeof(info);
    info.fMask  = CMIC_MASK_ASYNCOK;      // don't add CMIC_MASK_UNICODE unless you also set lpVerbW
    info.hwnd   = ownerHwnd;              
    info.lpVerb = MAKEINTRESOURCEA(idOffset);
    info.nShow  = SW_SHOWNORMAL;

    HRESULT res = menu->InvokeCommand((CMINVOKECOMMANDINFO*)&info);
    (void)res;
}

    
// Helper function: Creates an invisible canvas, tricking IContextMenu2/3
// into drawing its owner-drawn icon into it, then converts it to HBITMAP.
// This function sends the fake WM_MEASUREITEM and WM_DRAWITEM messages to trick the shll extension to draw directly into our RAM instead of the screen
HBITMAP CaptureOwnerDrawnIcon(IContextMenu2* pcm2, IContextMenu3* pcm3, UINT itemID) {
    if (!pcm2 && !pcm3) return nullptr;

    // Step 1: Politely ask the shell extension how big it wants to draw.
    // It expects a WM_MEASUREITEM message.
    MEASUREITEMSTRUCT mis = {};
    mis.CtlType = ODT_MENU;
    mis.itemID = itemID;
    mis.itemWidth = 16;  // Fallback width
    mis.itemHeight = 16; // Fallback height

    LRESULT lres = 0;
    if (pcm3) { // Send the baits
        pcm3->HandleMenuMsg2(WM_MEASUREITEM, 0, (LPARAM)&mis, &lres);
    } else if (pcm2) {
        pcm2->HandleMenuMsg(WM_MEASUREITEM, 0, (LPARAM)&mis);
    }

    // Owner-drawn menus usually try to draw the ENTIRE row (Icon + Text).
    // We ONLY want the icon. To steal just the icon, we give them a perfect square canvas
    // (based on their requested height) and let GDI clip the text off the right edge.
    int iconSize = mis.itemHeight > 0 ? mis.itemHeight : 16;

    // Step 2: Create our invisible canvas in RAM (Memory Device Context)
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, iconSize, iconSize);

    // Bind the bitmap to the HDC (put the blank canvas on the easel)
    HGDIOBJ hOldBmp = SelectObject(hdcMem, hBitmap);

    // Fill it with the standard menu background color first (so transparency looks right)
    HBRUSH bgBrush = CreateSolidBrush(GetSysColor(COLOR_MENU));
    RECT rect = { 0, 0, iconSize, iconSize };
    FillRect(hdcMem, &rect, bgBrush);
    DeleteObject(bgBrush);

    // Step 3: Command the extension to draw
    DRAWITEMSTRUCT dis = {};
    dis.CtlType = ODT_MENU;
    dis.itemID = itemID;
    dis.itemAction = ODA_DRAWENTIRE; // Draw the whole thing
    dis.itemState = 0;               // Normal state (not hovered/selected)
    dis.hDC = hdcMem;                // Give them our invisible canvas
    dis.rcItem = rect;               // Constrain them to the square icon area

    if (pcm3) {
        pcm3->HandleMenuMsg2(WM_DRAWITEM, 0, (LPARAM)&dis, &lres);
    } else if (pcm2) {
        pcm2->HandleMenuMsg(WM_DRAWITEM, 0, (LPARAM)&dis);
    }

    // Step 4: Clean up. Take the canvas off the easel and throw away the easels.
    SelectObject(hdcMem, hOldBmp);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);

    // Return the stolen pixels!
    return hBitmap;
}

void WalkMenu(IContextMenu* pcm, HMENU hMenu, UINT idCmdFirst, UINT idCmdLast, std::vector<ContextMenuItem>& out, ID3D11Device* pDevice){
    ComPtr<IContextMenu2> pcm2;
    pcm->QueryInterface(IID_PPV_ARGS(&pcm2));
    ComPtr<IContextMenu3> pcm3;
    pcm->QueryInterface(IID_PPV_ARGS(&pcm3));

    int count = GetMenuItemCount(hMenu);
    for (int i = 0; i < count; i++) {
        ContextMenuItem item{};
        MENUITEMINFOW mii = {sizeof(mii)};
        mii.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID | MIIM_SUBMENU | MIIM_STATE | MIIM_BITMAP | MIIM_DATA;

        wchar_t textBuf[256] = {};
        mii.dwTypeData = textBuf;
        mii.cch = 256;

        if (!GetMenuItemInfoW(hMenu, i, TRUE, &mii)) continue;

        if (mii.fType & MFT_SEPARATOR) {
            item.isSeparator = true;
            out.push_back(item);
            continue;
        }

        std::wstring rawText = textBuf;
        size_t tabPos = rawText.find(L'\t');
        if (tabPos != std::wstring::npos) {
            item.text = Str::WideToString(Str::CleanAmpersands(rawText.substr(0, tabPos).c_str()));
            item.shortcut = Str::WideToString(rawText.substr(tabPos + 1).c_str());
        } else item.text = Str::WideToString(Str::CleanAmpersands(rawText.c_str()));
        
        item.id = mii.wID - idCmdFirst;
        item.enabled = !(mii.fState & MFS_DISABLED);
        item.checked = (mii.fState & MFS_CHECKED) != 0;

        // Icon Retrieval logic ==================================================
        bool isOwnerDrawn = (mii.fType & MFT_OWNERDRAW) != 0;
        HBITMAP hStolenBmp = nullptr;
        bool weCreatedBitmap = false;
        
        // Case 1: Static Bitmap provided nicely by extension
        if (mii.hbmpItem != nullptr && mii.hbmpItem != HBMMENU_CALLBACK) {
            hStolenBmp = mii.hbmpItem;
        }
        // Case 2. Hostile Extension (7-Zip, Git, etc) insisting on drawing it themselves
        else if (isOwnerDrawn || mii.hbmpItem == HBMMENU_CALLBACK) {
            hStolenBmp = CaptureOwnerDrawnIcon(pcm2.Get(), pcm3.Get(), mii.wID);
            weCreatedBitmap = true; // Flag it so we know to clean up the RAM later
        }
        
        // Convert whatever HBITMAP we got into a DirectX Texture for ImGui
        if (hStolenBmp != nullptr) {
            int w = 0, h = 0;
            std::vector<u8> pixels = BitmapToPixels(hStolenBmp, w, h);
            
            if (!pixels.empty() && pDevice != nullptr) {
                item.srv = CreateTextureFromRGBA(pDevice, pixels, w, h);
                item.hIconTex = (ImTextureID)item.srv.Get(); // Assign ImGui Handle
            }
            
            // GDI objects leak memory heavily if not destroyed. 
            // If WE generated it via CaptureOwnerDrawnIcon, destroy it.
            // If the shell provided it statically, leave it alone.
            if (weCreatedBitmap) {
                DeleteObject(hStolenBmp);
            }
        }
        // ==================================================
        // RECURSION and COMMAND VERBS
        if (mii.hSubMenu) {
            // Many extensions (WinRAR, 7-Zip, "New", Send To, ...) fill their
            // cascading submenu lazily, only in response to WM_INITMENUPOPUP,
            // which normal TrackPopupMenu tracking sends right before display.
            // We never call TrackPopupMenu, so we have to fake that nudge ourselves,
            // the same way we already fake WM_MEASUREITEM/WM_DRAWITEM above.
            LRESULT lresInit = 0;
            if (pcm3) {
                pcm3->HandleMenuMsg2(WM_INITMENUPOPUP, (WPARAM)mii.hSubMenu,
                                    MAKELPARAM(i, FALSE), &lresInit);
            } else if (pcm2) {
                pcm2->HandleMenuMsg(WM_INITMENUPOPUP, (WPARAM)mii.hSubMenu,
                                    MAKELPARAM(i, FALSE));
            }

            // Recurse now, while hSubMenu is still valid, and store
            // parsed children instead of the handle itself.
            WalkMenu(pcm, mii.hSubMenu, idCmdFirst, idCmdLast, item.subItems, pDevice);
        }
        else if (mii.wID >= idCmdFirst) {
            char verbBuf[256] = {};
            // If it succeeds, we get useful verbs like "copy", "delete", "rename".
            if (SUCCEEDED(pcm->GetCommandString(item.id, GCS_VERBA, nullptr, verbBuf, sizeof(verbBuf)))){
                item.verb = verbBuf;
            }
        }
        bool hasText = !item.text.empty();
        bool hasSubItems = !item.subItems.empty();

        // Only add if it has actual text OR it's a parent menu containing valid sub-items
        if (hasText || hasSubItems) {
            out.push_back(item);
        }
    }
}


std::vector<ContextMenuItem> GetBackgroundContextMenu(ComPtr<IContextMenu>& outMenu, PCIDLIST_ABSOLUTE folderPidl, ID3D11Device* dev){
    // same as GetContextMenu, but BHID_SFViewObject instead of BHID_SFUIObject, and QueryContextMenu with CMF_EXPLORE (gives View/Sort by/New/Properties)
    std::vector<ContextMenuItem> result{};
    outMenu.Reset();

    if (!folderPidl) return result;

    ComPtr<IShellItem> pItem;
    if (FAILED(SHCreateItemFromIDList(folderPidl, IID_PPV_ARGS(&pItem)))) return result;
    if (FAILED(pItem->BindToHandler(nullptr, BHID_SFViewObject, IID_PPV_ARGS(&outMenu)))) return result;

    HMENU hMenu = CreatePopupMenu();
    constexpr UINT idCmdFirst = 1;
    constexpr UINT idCmdLast = 0x7FFF;
    if (!hMenu) return result;
    
    UINT flags = CMF_NORMAL | CMF_EXPLORE | CMF_CANRENAME;
    if (SUCCEEDED(outMenu->QueryContextMenu(hMenu, 0, 1, 0x7FFF, flags))){
        // Fill vector using recursive WalkMenu
        WalkMenu(outMenu.Get(), hMenu, idCmdFirst, idCmdLast, result, dev);
    }
    DestroyMenu(hMenu); 

    return result;
}


std::vector<ContextMenuItem> GetContextMenu(ComPtr<IContextMenu>& outActiveMenu, PCIDLIST_ABSOLUTE parentPidl, std::vector<PCITEMID_CHILD>& childPidls, HWND hwnd, ID3D11Device* pDevice){
    std::vector<ContextMenuItem> result{};
    outActiveMenu.Reset();

    if (!parentPidl || childPidls.empty()) return result;

    ComPtr<IShellFolder> desktop;
    if (FAILED(SHGetDesktopFolder(&desktop))) return result;

    ComPtr<IShellFolder> parentFolder;
    if (FAILED(desktop->BindToObject(parentPidl, nullptr, IID_PPV_ARGS(&parentFolder))))
        return result;

    if (FAILED(parentFolder->GetUIObjectOf(hwnd, (UINT)childPidls.size(), const_cast<LPCITEMIDLIST*>(childPidls.data()) ,IID_IContextMenu,nullptr,(void**)&outActiveMenu)))
        return result;

    HMENU hMenu = CreatePopupMenu();
    constexpr UINT idCmdFirst = 1;
    constexpr UINT idCmdLast = 0x7FFF;
    if (!hMenu) return result;

    UINT flags = CMF_NORMAL | CMF_EXPLORE | CMF_CANRENAME;
    if (SUCCEEDED(outActiveMenu->QueryContextMenu(hMenu, 0, idCmdFirst, idCmdLast, flags))){
        WalkMenu(outActiveMenu.Get(), hMenu, idCmdFirst, idCmdLast, result, pDevice);
    }
    DestroyMenu(hMenu);

    return result;
}

