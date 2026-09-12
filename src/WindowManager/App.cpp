#include "App.h"
#include "FileOpAsync.h"

void App::ProcessCommands() {
    for (auto& cmd : cmdQueue.cmds) {
        std::visit(overloaded{
            [&](Cmd_NewTab& c)    { window.NewTab(c.targetPidl.get()); },
            [&](Cmd_CloseTab& c)  { window.CloseTab(c.tabIndex); },
            [&](Cmd_SwitchTab& c) { window.SetActiveTab(c.tabIndex); },
            [&](Cmd_GoTo& c)      { window.tabs[c.tabIndex].GoTo(c.targetPidl.get(), Actions::Normal); },
            [&](Cmd_Rename& c)    { (void)c; },
            [&](Cmd_Delete& c)    { (void)c; },
            [&](Cmd_Refresh& c)   { window.tabs[c.tabIndex].Refresh(); },
            [&](Cmd_GoBack& c)    { window.tabs[c.tabIndex].GoBack(); },
            [&](Cmd_GoForward& c) { window.tabs[c.tabIndex].GoForward(); },
            [&](Cmd_GoParent& c)  { window.tabs[c.tabIndex].GoParent(); },
            [&](Cmd_OpenFile& c)  { WShell::ExecuteFile(c.targetPidl.get()); },
            [&](Cmd_ReSort& c)    { window.tabs[c.tabIndex].ReSort(); },
            [&](Cmd_RefreshByHash& c){ 
                u64 hashToInvalidate = c.hash;
                for (auto& tab : window.tabs){
                    if (tab.dir.parent.hash == hashToInvalidate){ tab.Refresh(); }
                }
            },
            [&](Cmd_CopyOrMoveItems& c){
                struct FileOpParams params;
                params.type = c.isCopy ? FileOp::Copy : FileOp::Move;
                params.parentFolder = c.parentPidl; c.parentPidl = nullptr;
                params.childItems = std::move(c.items);    
                params.destFolder = c.targetPidl;   c.targetPidl = nullptr;
                params.setOpflags = FOF_ALLOWUNDO | FOF_NOCONFIRMMKDIR;
                ExecuteAsyncFileOp(gfx.hwnd, params);  
            }
        }, cmd);
    }
    cmdQueue.cmds.clear();
}

