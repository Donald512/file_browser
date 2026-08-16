// shellInternal.h
#pragma once
#include "Shell.h"


std::string GetDisplayName(IShellFolder* folder, PITEMID_CHILD child, SHGDNF flags);
std::string GetDisplayName(PCIDLIST_ABSOLUTE pidl );

WShell::Pidl CombineChild(PCIDLIST_ABSOLUTE parent, PITEMID_CHILD child);


// The universal COM enumeration loop — every "list a folder's children" call
// in this file goes through here instead of hand-rolling BindToObject/EnumObjects.
SO