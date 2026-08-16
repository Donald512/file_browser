#pragma once

// Deliberately Win32-only. Nothing here may include d3d11.h or imgui.h —
// this is what the pure filesystem/shell layer (Shell.cpp) is allowed to
// depend on for icons. Texture caching (which DOES need d3d11/imgui)
// lives in Icons.h and includes this header, not the other way around.
#include "Types.h"
#include <ShlObj.h>

namespace Icons{
    // Returns the system image list index for a pidl or path — cheap,
    // no GDI/D3D work happens here. 0 on failure (matches an empty icon).
    u32 GetIconIndex(PCIDLIST_ABSOLUTE pidl, const wchar_t* pszPath, DWORD dwFileAttributes, UINT uFlags);
}
