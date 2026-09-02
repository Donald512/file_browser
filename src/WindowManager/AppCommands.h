// Commands.h
#pragma once
#include <variant>
#include <vector>
#include <string>
#include "Pidl.h"


struct Cmd_NewTab      { WShell::Pidl targetPidl; };
struct Cmd_CloseTab    { size_t tabIndex; };
struct Cmd_SwitchTab   { size_t tabIndex; };
struct Cmd_GoTo        { size_t tabIndex; WShell::Pidl targetPidl; };
struct Cmd_Rename      { std::wstring newName; };
struct Cmd_Delete      { std::vector<PCITEMID_CHILD> items; bool permanent = false; };
struct Cmd_Refresh     { size_t tabIndex;};
struct Cmd_GoBack      { size_t tabIndex;};
struct Cmd_GoForward   { size_t tabIndex;};
struct Cmd_GoParent    { size_t tabIndex;};
struct Cmd_OpenFile    { WShell::Pidl targetPidl; };
struct Cmd_ReSort      { size_t tabIndex; };
struct Cmd_RefreshByHash { u64 hash;};

using AppCommand = std::variant<
    Cmd_NewTab, Cmd_CloseTab, Cmd_SwitchTab, Cmd_GoTo,
    Cmd_Rename, Cmd_Delete, Cmd_Refresh, Cmd_RefreshByHash, Cmd_GoBack,
    Cmd_GoForward, Cmd_GoParent, Cmd_OpenFile, Cmd_ReSort
>;