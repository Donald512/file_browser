#include "Shell.h"
#include <unordered_set>

using namespace WShell;


bool WShell::ExecuteFile(PCIDLIST_ABSOLUTE file){
    if (!file) return false;

    SHELLEXECUTEINFOW sei = {};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_IDLIST | SEE_MASK_ASYNCOK;
    sei.lpIDList = const_cast<PIDLIST_ABSOLUTE>(file);
    sei.nShow = SW_SHOWNORMAL;

    if (!::ShellExecuteExW(&sei)){
        // todo handle error, eg Access Denied, or No app associated
        DWORD err = GetLastError();
        printf("Failed to launch item. Error: %lu\n", err);
        return false;
    }
    return true;
}

void WShell::ExecuteContextMenuCommand(ComPtr<IContextMenu> menu, UINT id){
    if (!menu) return;

    CMINVOKECOMMANDINFO info{};
    info.cbSize = sizeof(CMINVOKECOMMANDINFO);
    info.fMask = 0;
    info.hwnd = GetActiveWindow();
    info.lpVerb = MAKEINTRESOURCEA(id);
    info.nShow = SW_SHOWNORMAL;
    menu->InvokeCommand(&info);
}
