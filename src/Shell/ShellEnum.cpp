#include "Shell.h"
#include "ShellInternal.h"
#include <Shlwapi.h>
#include <algorithm>
#include "Str.h"


using namespace WShell;


std::vector<Item> WShell::EnumFolder(PCIDLIST_ABSOLUTE folder){
    std::vector<Item> items;
    IterateFolder(folder, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, [&](IShellFolder* pTarget, PITEMID_CHILD child) {
        Item item;
        item.name = GetDisplayName(pTarget, child, SHGDN_NORMAL);
        item.pidl = CombineChild(folder, child);
        item.hash = HashPidl(item.pidl.get());
        
        // item.attributes = 0xFFFFFFFF;
        item.attributes = SFGAO_FOLDER | SFGAO_CANRENAME | SFGAO_CANDELETE;
        pTarget->GetAttributesOf(1, (LPCITEMIDLIST*)&child, &item.attributes);

        bool isFolder = (item.attributes & SFGAO_FOLDER);

        WIN32_FIND_DATAW wfd{};
        if (SUCCEEDED(SHGetDataFromIDListW(pTarget, child, SHGDFIL_FINDDATA, &wfd, sizeof(wfd)))){
            if (!isFolder){
                item.size.value = (static_cast<u64>(wfd.nFileSizeHigh) << 32) | wfd.nFileSizeLow;
                item.size.resolved = true;
            } else{
                item.size.value = 0;
                item.size.resolved = true;
            }
            item.lastWriteTime = wfd.ftLastWriteTime;

            SHFILEINFOW sfi = {};
            DWORD dwAttr = (item.attributes & SFGAO_FOLDER) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
            if (SHGetFileInfoW(wfd.cFileName, dwAttr, &sfi, sizeof(sfi), SHGFI_TYPENAME | SHGFI_USEFILEATTRIBUTES)) {
                item.typeName.value = Str::WideToString(sfi.szTypeName);
                item.typeName.resolved = true;
            }
        }
        // VIRTUAL PATH: If SHGetDataFromIDListW failed (e.g. "This PC"), 
        // item.size and item.typeName remain unresolved (resolved = false) 
        // and will naturally execute their fallback lambdas if accessed.
        items.push_back(std::move(item));
    });

    return items;
}

std::vector<ItemLite>WShell::GetLiteItems(PCIDLIST_ABSOLUTE folder){
    
    std::vector<ItemLite> items;
    // IterateFolder(folder, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, [&](IShellFolder* pTarget, PITEMID_CHILD child) {
    IterateFolder(folder, SHCONTF_FOLDERS, [&](IShellFolder* pTarget, PITEMID_CHILD child) {
        ItemLite item;
        item.name = GetDisplayName(pTarget, child, SHGDN_NORMAL);
        item.pidl = CombineChild(folder, child);
        item.hash = HashPidl(item.pidl.get());
        items.push_back(std::move(item));
    });

    return items;
}



bool WShell::Directory::Load(PCIDLIST_ABSOLUTE folder){

    if (!folder) return false;
    items = WShell::EnumFolder(folder);
    access = WShell::GetFolderAccess(folder);
    selectedIndex = -1;

    ResortItems();
    return true;
}




void WShell::Directory::ResortItems(){
    std::sort(items.begin(), items.end(), [this](const Item& a, const Item& b){

        // Always keep Folders at the top, regardless of sort mode/direction
        bool aIsFolder = (a.attributes & SFGAO_FOLDER) != 0;
        bool bIsFolder = (b.attributes & SFGAO_FOLDER) != 0;

        if (aIsFolder != bIsFolder){
            return aIsFolder;
        }

        // compare based on selected sortmode
        int cmp = 0;
        switch (sortMode) {
            case SortMode::Name:{
                // _stricmp does a case-insensitive ASCII comparison.
                cmp = _stricmp(a.name.c_str(), b.name.c_str());
            }
                break;

            case SortMode::DateModified:{
                // CompareFileTime returns -1, 0, or 1
                cmp = ::CompareFileTime(&a.lastWriteTime, &b.lastWriteTime);
            }
                break;

            case SortMode::Type:{
                // cmp = _stricmp(a.TypeName().c_str(), b.TypeName().c_str());
                // Delibrately does not call .TypeName() here, those lazily resolve on first call, a sychronous shell call from inside a sort comparator. Unresolved items like virtual items that EnumFolder couldn't resolve up front just compare as "" until something  E.g: (a visible row's) WShell::Async::RequestMeta call) resolves them for real; sorting again afterward will then reflect the real type name.
                std::string aType = a.typeName.resolved ? a.typeName.value : std::string();
                std::string bType = b.typeName.resolved ? b.typeName.value : std::string();
                cmp = _stricmp(aType.c_str(), bType.c_str());
            }
                break;

            case SortMode::Size:{
                // if (a.Size() < b.Size()) cmp = -1;
                // else if (a.Size() > b.Size()) cmp = 1;
                // read the resolved value rather than call the lazy size() 
                u64 aSize = a.size.resolved ? a.size.value : 0ULL;
                u64 bSize = b.size.resolved ? b.size.value : 0ULL;
                if (aSize < bSize) cmp = -1;
                else if (aSize > bSize) cmp = 1;
            }
            break;
    
        }

        //  Strict Weak Ordering: If the primary condition is a tie, fallback to Name
        if (cmp == 0) {
            cmp = _stricmp(a.name.c_str(), b.name.c_str());
        }

        if (sortDirection == SortDirection::Descending){
            return cmp > 0;
        }
        return cmp < 0;
    });
}

// RegQueryValueExW reads a VALUE, this function is just boilerplate that will be used to read the registry
static std::wstring GetDefaultValue(HKEY root, const wchar_t* subkey){
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(root, subkey, 0, KEY_READ, &hKey) != ERROR_SUCCESS) return L"";

    wchar_t buffer[256] = {};
    DWORD size = sizeof(buffer);
    std::wstring result;

    if (RegQueryValueExW(hKey, nullptr, nullptr, nullptr, reinterpret_cast<BYTE*>(buffer), &size) == ERROR_SUCCESS) {
        result = buffer;
    }
    RegCloseKey(hKey);
    return result;
}


std::vector<NewMenuItem> WShell::EnumerateNewMenu(){
    std::vector<NewMenuItem> menuItems;

    HKEY hKeyRoot = nullptr;
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, nullptr, 0, KEY_READ, &hKeyRoot) != ERROR_SUCCESS){
        return menuItems;
    }

    wchar_t subKeyName[256];
    for (DWORD index = 0; ; index++){
        DWORD nameLen = 256;   // reset every iteration, not just once before the loop
        if (RegEnumKeyExW(hKeyRoot, index, subKeyName, &nameLen, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS){
            break;   // ERROR_NO_MORE_ITEMS, or a real error — either way, done
        }

         if (subKeyName[0] != L'.') continue;

        wchar_t shellNewSubkey[280];   // separate buffer — never mutate subKeyName itself
        swprintf_s(shellNewSubkey, L"%s\\ShellNew", subKeyName);

        HKEY hKeyShellNew = nullptr;
        if (RegOpenKeyExW(HKEY_CLASSES_ROOT, shellNewSubkey, 0, KEY_READ, &hKeyShellNew) != ERROR_SUCCESS){
            continue;   // no ShellNew key = doesn't belong in the New menu
        }

        NewMenuItem item;
        item.extension = Str::WideToString(subKeyName);

        wchar_t templateFile[MAX_PATH] = {};
        DWORD templateSize = sizeof(templateFile);
        if (RegQueryValueExW(hKeyShellNew, L"FileName", nullptr, nullptr, (LPBYTE)templateFile, &templateSize) == ERROR_SUCCESS){
            item.action = NewItemAction::FromTemplate;
            item.templatePath = TypeablePathToPidl(templateFile);
        }
        RegCloseKey(hKeyShellNew);

        std::wstring progID = GetDefaultValue(HKEY_CLASSES_ROOT, subKeyName);
        if (!progID.empty()){
            std::wstring friendlyName = GetDefaultValue(HKEY_CLASSES_ROOT, progID.c_str());
            if (!friendlyName.empty()){
                item.displayName = Str::WideToString(friendlyName.c_str());
            }
        }
        if (item.displayName.empty()){
            item.displayName = item.extension;   // fallback so the entry isn't blank
        }

        menuItems.push_back(std::move(item));
    }
    RegCloseKey(hKeyRoot);
    return menuItems;
    
}