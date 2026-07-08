// backend.cpp
#include "core.h"
#include "Shlwapi.h"

namespace Backend{
    void EnumerateDirectory(AppContext& ctx, PIDLIST_ABSOLUTE targetPidl){
        if (!targetPidl) return;

        // Clean up old list
        FreeDirectoryArray(ctx.currentDirArray);

        IShellFolder* pDesktop = nullptr;       // the COM object for the Desktop (root)
        IShellFolder* pTargetFolder = nullptr;  // the COM object for the final target folder

        SHGetDesktopFolder(&pDesktop);  // fills the desktop COM object with its internal variables and vtable
        
        // Handle Desktop root differently, cannot Bind desktop to desktop
        if (ILIsEmpty(targetPidl)){
            pTargetFolder = pDesktop;
            pTargetFolder->AddRef();    // add reference for release at the bottom
        }
        else{
            if (FAILED(pDesktop->BindToObject(targetPidl, nullptr, IID_IShellFolder, (void**)&pTargetFolder))){
                pDesktop->Release();
                return;
            }
        }

        // Enumerate Objects
        IEnumIDList* pEnum = nullptr;
        if (FAILED(pTargetFolder->EnumObjects(nullptr, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &pEnum))){ // gets everything in the directory, both sub folders and physical files
            if (pEnum) pEnum->Release();
            pTargetFolder->Release();
            pDesktop->Release();
        }

        ctx.currentDirArray.capacity = 64;
        ctx.currentDirArray.entries = (ShellItem*) malloc(sizeof(ShellItem) * ctx.currentDirArray.capacity);

        LPITEMIDLIST childPidl = nullptr;   // buffer for PIDLs
        ULONG fetched = 0;                  // tracker required by Windows to confirm that an item was succefully copied
        
        while (pEnum->Next(1, &childPidl, &fetched) == S_OK){   // get 1 (first param) item from the collection, and continues until there are no items left
            if (ctx.currentDirArray.numEntries >= ctx.currentDirArray.capacity) {
                ctx.currentDirArray.capacity *= 2;
                ctx.currentDirArray.entries = (ShellItem*)realloc(ctx.currentDirArray.entries, sizeof(ShellItem) * ctx.currentDirArray.capacity);
            }

            ShellItem& entry= ctx.currentDirArray.entries[ctx.currentDirArray.numEntries];     // this is not a copy, entry is now just a shortcut name for ctx.currentDirArray.entries[ctx.currentDirArray.numEntries]

            // Get Display name
            STRRET strName;
            pTargetFolder->GetDisplayNameOf(childPidl, SHGDN_NORMAL, &strName); 
            wchar_t nameBuffer[MAX_PATH];
            StrRetToBufW(&strName, childPidl, nameBuffer, MAX_PATH);
            entry.name = Str::WideToString(nameBuffer);

            entry.attributes = SFGAO_FOLDER | SFGAO_CANRENAME | SFGAO_CANDELETE;  // todo add SFGAO_HASPROPSHEET
            pTargetFolder->GetAttributesOf(1, (LPCITEMIDLIST*)&childPidl, &entry.attributes);
            entry.pidl = ILCombine(targetPidl, childPidl);

            CoTaskMemFree(childPidl);   
            ctx.currentDirArray.numEntries++;
        }
        pEnum->Release();  // Done with enumerator object, call Release to decrement its count
        pTargetFolder->Release();
        pDesktop->Release();

    }

    void FreeDirectoryArray(DirectoryArray& array){
        if (!array.entries) return;
        for (u64 i = 0; i < array.numEntries; i++){
            Str::Free(array.entries[i].name);
            Utils::FreePidl(array.entries[i].pidl);
        }
        free(array.entries);
        array.entries = nullptr;
        array.numEntries = 0;
        array.capacity = 0;

    } 

    LightShellItemArray GetDirectoryContents(PIDLIST_ABSOLUTE targetPidl){
        if (!targetPidl) return LightShellItemArray{};

        IShellFolder* pDesktop = nullptr;      
        IShellFolder* pTargetFolder = nullptr; 

        SHGetDesktopFolder(&pDesktop); 

        if (FAILED(pDesktop->BindToObject(targetPidl, nullptr, IID_IShellFolder, (void**)&pTargetFolder))){
            pDesktop->Release();
            return LightShellItemArray{};
        }

       
        IEnumIDList* pEnum = nullptr;
        if (FAILED(pTargetFolder->EnumObjects(nullptr, SHCONTF_FOLDERS, &pEnum))){ // this is for breadcrumbs only, so only folders
            if (pEnum) pEnum->Release();
            pTargetFolder->Release();
            pDesktop->Release();
        }

        LightShellItemArray results{};

        results.capacity = 32;
        results.entries = (LightShellItem*) malloc(sizeof(LightShellItem) * results.capacity);

        LPITEMIDLIST childPidl = nullptr; 
        ULONG fetched = 0;                
        
        while (pEnum->Next(1, &childPidl, &fetched) == S_OK){ 
            if (results.numEntries >= results.capacity) {
                results.capacity *= 2;
                results.entries = (LightShellItem*)realloc(results.entries, sizeof(LightShellItem) * results.capacity);
            }

            LightShellItem& entry= results.entries[results.numEntries];   

    
            STRRET strName;
            pTargetFolder->GetDisplayNameOf(childPidl, SHGDN_NORMAL, &strName); 
            wchar_t nameBuffer[MAX_PATH];
            StrRetToBufW(&strName, childPidl, nameBuffer, MAX_PATH);
            entry.name = Str::WideToString(nameBuffer);
            entry.pidl = ILCombine(targetPidl, childPidl);

            CoTaskMemFree(childPidl);   
            results.numEntries++;
        }
        pEnum->Release();
        pTargetFolder->Release();
        pDesktop->Release();

        return results;
    }

    void FreeLightShellItemArray(LightShellItemArray& array){
        if (!array.entries) return;
        for (u64 i = 0; i < array.numEntries; i++){
            Str::Free(array.entries[i].name);
            Utils::FreePidl(array.entries[i].pidl);
        }
        free(array.entries);
        array.entries = nullptr;
        array.numEntries = 0;
        array.capacity = 0;
    }

    void GenerateBreadcrumbs(AppContext& ctx, PIDLIST_ABSOLUTE targetPidl){
        FreeBreadcrumbs(ctx.currentBreadcrumbs);
        if (!targetPidl) return;

        ctx.currentBreadcrumbs.count = 0;
        ctx.currentBreadcrumbs.capacity = 10;
        ctx.currentBreadcrumbs.breadcrumbs = (BreadcrumbItem*)malloc(sizeof(BreadcrumbItem) * ctx.currentBreadcrumbs.capacity);

        IShellFolder* pDesktop = nullptr;
        SHGetDesktopFolder(&pDesktop);

        PIDLIST_ABSOLUTE pAccumulated = nullptr;
        LPCITEMIDLIST pCurrentHop = targetPidl;

        while (pCurrentHop && pCurrentHop->mkid.cb >  0){
            LPITEMIDLIST pSingleItem = ILCloneFirst(pCurrentHop);
            PIDLIST_ABSOLUTE pNewAccumulated = ILCombine(pAccumulated, pSingleItem);

            if (pAccumulated) ILFree(pAccumulated);
            pAccumulated = pNewAccumulated;

            if (ctx.currentBreadcrumbs.count >= ctx.currentBreadcrumbs.capacity){
                ctx.currentBreadcrumbs.capacity *= 2;
                ctx.currentBreadcrumbs.breadcrumbs = (BreadcrumbItem*) realloc(ctx.currentBreadcrumbs.breadcrumbs, sizeof(BreadcrumbItem) * ctx.currentBreadcrumbs.capacity);
            }

            BreadcrumbItem& crumb = ctx.currentBreadcrumbs.breadcrumbs[ctx.currentBreadcrumbs.count];

            wchar_t* pAllocatedName = nullptr;
            if (SUCCEEDED(SHGetNameFromIDList(pAccumulated, SIGDN_NORMALDISPLAY, &pAllocatedName))) {
                crumb.displayName = Str::WideToString(pAllocatedName);
                CoTaskMemFree(pAllocatedName);
            }
            crumb.pidl = ILClone(pAccumulated);

            ctx.currentBreadcrumbs.count++;
            Utils::FreePidl(pSingleItem);
            pCurrentHop = ILGetNext(pCurrentHop);
        }

        ctx.currentBreadcrumbs.fullPath = GetTypeablePath(pAccumulated);
        ctx.currentBreadcrumbs.hasSubFolders = PidlHasSubFolders(pAccumulated);

        if (pAccumulated){
            Utils::FreePidl(pAccumulated);
        }
        if (pDesktop){
            pDesktop->Release();
        }
    }

    void FreeBreadcrumbs(BreadcrumbArray& array){
        if (!array.breadcrumbs) return;
        for (u64 i = 0; i < array.count; i++){
            Str::Free(array.breadcrumbs[i].displayName);
            Utils::FreePidl(array.breadcrumbs[i].pidl);
        }
        Str::Free(array.fullPath);
        free(array.breadcrumbs);
        array.breadcrumbs = 0;
        array.count = 0;
        array.capacity = 0;
    }

    Str::String GetTypeablePath(PIDLIST_ABSOLUTE targetPidl){
        if (!targetPidl || targetPidl->mkid.cb == 0) {
            // If it's the root Desktop, just return "Desktop"
            return Str::Create("Desktop");
        }

        wchar_t* pAllocatedPath = nullptr;
        Str::String resultStr{};

        // Try to get a real file system path (e.g., "C:\Users\Documents")
        if (SUCCEEDED(SHGetNameFromIDList(targetPidl, SIGDN_FILESYSPATH, &pAllocatedPath))) {
            resultStr = Str::WideToString(pAllocatedPath);
            CoTaskMemFree(pAllocatedPath);
            return resultStr;
        }

        // If it's a known folder or virtual shortcut, get the friendly parsing name (e.g., "Documents")
        if (SUCCEEDED(SHGetNameFromIDList(targetPidl, SIGDN_DESKTOPABSOLUTEPARSING, &pAllocatedPath))) {
            // Check if Windows handed us a nasty GUID string (starts with "::")
            if (pAllocatedPath[0] == L':' && pAllocatedPath[1] == L':') {
                CoTaskMemFree(pAllocatedPath);
                pAllocatedPath = nullptr;
                
                // It's a pure virtual root like "This PC". Get its display name instead.
                if (SUCCEEDED(SHGetNameFromIDList(targetPidl, SIGDN_NORMALDISPLAY, &pAllocatedPath))) {
                    resultStr = Str::WideToString(pAllocatedPath);
                    CoTaskMemFree(pAllocatedPath);
                    return resultStr;
                }
            } else {
                resultStr = Str::WideToString(pAllocatedPath);
                CoTaskMemFree(pAllocatedPath);
                return resultStr;
            }
        }
        return Str::String{};
    }

    bool PidlHasSubFolders(PCIDLIST_ABSOLUTE targetPidl){   // PCIDLIST_ABSOLUTE is const PIDLIST_ABSOLUTE
        if (!targetPidl) return false;

        bool hasSubFolders = false;
        IShellItem* pParentItem = nullptr;
        
        if (FAILED(SHCreateItemFromIDList(targetPidl, IID_PPV_ARGS(&pParentItem)))){
            return false;
        }
        // check if folder
        SFGAOF attr = 0;
        if (FAILED(pParentItem->GetAttributes(SFGAO_FOLDER, &attr)) || !(attr & SFGAO_FOLDER)){
            pParentItem->Release();
            return false;
        }
        
        IEnumShellItems* pEnum = nullptr;
        if (FAILED(pParentItem->BindToHandler(nullptr, BHID_EnumItems, IID_PPV_ARGS(&pEnum)))){
            pParentItem->Release();
            return false;
        }
        
        IShellItem* pChild = nullptr;
        ULONG fetched = 0; 

        // exit once a folder is found
        while (pEnum->Next(1, &pChild, &fetched) == S_OK && fetched == 1){
            SFGAOF childAttr = 0;
            if (SUCCEEDED(pChild->GetAttributes(SFGAO_FOLDER, &childAttr))){
                if (childAttr & SFGAO_FOLDER){
                    hasSubFolders = true;
                    pChild->Release();
                    break;
                }
            }
            pChild->Release();
        }
        
        pEnum->Release();
        pParentItem->Release();

        return hasSubFolders;
    }

    PIDLIST_ABSOLUTE CreatePidlFromPath(const wchar_t* widePath){
        // If they typed a physical path (e.g., "C:\Windows" or "D:\")
        PIDLIST_ABSOLUTE pidl = nullptr;
        DWORD attrs = 0;
        
        // Try as a standard path or GUID Parsing Name (e.g. "C:\Windows" or "::{GUID}")
        if (SUCCEEDED(::SHParseDisplayName(widePath, nullptr, &pidl, 0, &attrs))){
            return pidl;   
        }

        // if it failed, try searching the Desktop. (Catches "This PC", "Recycle Bin", "Linux", custom virtual folders)
// 2. DYNAMIC SEARCH: The Desktop
        IShellFolder* pDesktop = nullptr;
        if (SUCCEEDED(::SHGetDesktopFolder(&pDesktop))) {
            IEnumIDList* pEnum = nullptr;
            if (SUCCEEDED(pDesktop->EnumObjects(nullptr, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &pEnum))) {
                LPITEMIDLIST childPidl = nullptr;
                ULONG fetched = 0;
                
                while (pEnum->Next(1, &childPidl, &fetched) == S_OK) {
                    STRRET strName;
                    if (SUCCEEDED(pDesktop->GetDisplayNameOf(childPidl, SHGDN_NORMAL, &strName))) {
                        wchar_t nameBuf[MAX_PATH];
                        StrRetToBufW(&strName, childPidl, nameBuf, MAX_PATH);

                        if (_wcsicmp(nameBuf, widePath) == 0) {
                            pidl = ILClone(childPidl); 
                            CoTaskMemFree(childPidl); // FIX: Free this BEFORE breaking to prevent memory leak
                            break;
                        }
                    }
                    CoTaskMemFree(childPidl);
                }
                pEnum->Release();
            }
            pDesktop->Release();
            
            if (pidl) return pidl; 
        }

        // 3. DYNAMIC SEARCH: Known Folders
        IKnownFolderManager* pManager = nullptr;
        if (SUCCEEDED(CoCreateInstance(CLSID_KnownFolderManager, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pManager)))) {
            UINT count = 0;
            KNOWNFOLDERID* pIds = nullptr;
            
            if (SUCCEEDED(pManager->GetFolderIds(&pIds, &count))) {
                for (UINT i = 0; i < count; i++) {
                    IKnownFolder* pFolder = nullptr;
                    if (SUCCEEDED(pManager->GetFolder(pIds[i], &pFolder))) {
                        IShellItem* pItem = nullptr;
                        if (SUCCEEDED(pFolder->GetShellItem(0, IID_PPV_ARGS(&pItem)))) {
                            LPWSTR pName = nullptr;
                            if (SUCCEEDED(pItem->GetDisplayName(SIGDN_NORMALDISPLAY, &pName))) {
                                if (_wcsicmp(pName, widePath) == 0) {
                                    SHGetKnownFolderIDList(pIds[i], 0, NULL, &pidl);
                                    CoTaskMemFree(pName);
                                    pItem->Release();
                                    pFolder->Release();
                                    break; 
                                }
                                CoTaskMemFree(pName);
                            }
                            pItem->Release();
                        }
                        pFolder->Release();
                    }
                    if (pidl) break;
                }
                CoTaskMemFree(pIds);
            }
            pManager->Release();
            
            if (pidl) return pidl; 
        }

        return nullptr; 
    }

} // namespace Backend



/*
An IShellFolder instance is an interface used to manage a specific folder in the Windows Shell
You have to start at the root pDesktop
r
    SHGetDesktopFolder(&pDesktop);  // Does not create a new instance of the desktop folder, it just gives a pointer to the desktop, or the specific folder, 
        which is why it collects a pointer to pointer. switches it from holding nullptr to desktop object pointer. 

A PIDL is a variable length binary sturcture. if a folder is 12 layers deep, its PIDLIST_ABSOLUTE is a long chain of 12 individual binary IDs stitched together 

    SHGDN_NORMAL is optimized for display in a UI E.G: "Local Disk (C:)" or "Documents"
    SHGDN_FORADDRESSBAR: Optimized for an editable path bar E.G: "C:\" or "::{20D04FE0-3AEA-1069-A2D8-08002B30309D}" for virtual folders.

The method to implement virtual folders uses COM because different folders, arent alike, recycle bin, control panel, normal ntfs, desktop, etc the COM is a contract for unity, 
    like how each appliance like tv, toaster, iron could have different plugs, but they follow the outlet standard so we can have one outlet for all appliance
*/
