Windows File Browser
A tabbed Windows file browser built with C++20, Win32, Windows Shell COM, Direct3D 11, and Dear ImGui.

I built this project to explore what goes into a native Windows file manager: working with the Windows Shell namespace, designing a responsive file-listing UI, managing cached directory data, and keeping expensive enumeration work off the UI thread.
Navigation and Windows Shell integration
- Browser-style tabs with back, forward, parent-folder, and refresh navigation.
- Breadcrumb address bar for navigating locations.
- Windows Shell COM and PIDLs for enumerating and navigating filesystem locations and supported virtual Shell folders.
- Native Shell context-menu integration and opening files with their registered default applications.
File views and interaction
- Five view modes: Icons, Small, List, Details, and Tiles.
- Sorting by name, date modified, type, and size, with ascending and descending order.
- Toggle hidden items.
- Mouse and keyboard interaction, including Ctrl-selection, Shift-range selection, marquee selection, keyboard navigation, and F2/click-to-rename.
- Create folders and other items exposed by Windows Shell's New menu.
- Shell clipboard cut/copy/paste support and OLE drag-and-drop. Cross-application drag-and-drop is still experimental.
- A directory watcher that refreshes filesystem views when changes are detected.
Engineering highlights
- Asynchronous directory enumeration: directory data is built on worker threads and applied to shared application state on the main thread.
- Generation-checked directory cache: cached folders use reusable slots and generation counters so a stale handle can be detected after invalidation or slot reuse.
- Cache validation: directory cache entries are checked against the folder's last-modified timestamp before being reused.
- Compact directory representation: file names and child PIDLs are stored in byte arenas, with metadata kept in parallel vectors (a structure-of-arrays layout).
- Reduced repeated work: icon lookups are cached, file selection uses a bitmask over directory-entry indices, and the Details view clips rows outside the visible region.
- Explicit ownership and task dispatch: the code uses WRL ComPtr, an owning PIDL wrapper, and a move-only callable for queued tasks. Worker jobs produce results; the main thread applies those results to UI-facing state.
These are implementation choices rather than benchmark claims; performance should be evaluated on the target machine and workload.
Technology
- Language: C++20
- Platform APIs: Win32, Windows Shell COM/PIDLs, OLE drag-and-drop, ReadDirectoryChangesW, and IFileOperation
- Rendering: Direct3D 11
- UI: Dear ImGui
- Build toolchain: MSVC; the local build scripts invoke CMake
- Cross-application drag-and-drop is a work in progress.
- Directory watching is non-recursive and applies to filesystem paths; virtual Shell locations are not watched.
