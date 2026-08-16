@echo off
@REM call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

:: 1. Check if file name provided
if "%~1"=="" (
    set "source_file=src\main.cpp"
    set "build_dir=build_release"
) else (
    set "source_file=%1"
    set "build_dir=%~n1_release"
)

echo 🚀 BUILDING FOR PROFILING / RELEASE (Optimized with Symbols)
echo Target Folder: %build_dir%
echo Source File:   %source_file%

if not exist %build_dir% mkdir %build_dir%

set "imgui_root=thirdparty\imgui"
set "font_stuff=thirdparty\fontstuff"

:: Flags for maximum performance while keeping debug symbols
set "c_flags=/O2 /Oi /Gy /DNDEBUG"

echo Checking Dependencies in %build_dir%
if not exist %build_dir%\imgui.obj (
    echo [1/2] Compiling ImGui core libraries...
    cl /nologo /c /Z7 /std:c++20 %c_flags% /I"include" /I"%imgui_root%" /I"%imgui_root%\backends" /I"%font_stuff%" ^
       "%imgui_root%\imgui.cpp" ^
       "%imgui_root%\imgui_widgets.cpp" ^
       "%imgui_root%\imgui_draw.cpp" ^
       "%imgui_root%\imgui_tables.cpp" ^
       "%imgui_root%\backends\imgui_impl_dx11.cpp" ^
       "%imgui_root%\backends\imgui_impl_win32.cpp" ^
       /Fo"%build_dir%\\" /Fd"%build_dir%\\"
)

echo [2/2] Compiling project files and linking...
cl /nologo /Z7 /W4 /MP /std:c++20 %c_flags% ^
   /I"include" ^
   /I"src" ^
   /I"src\Global\Icons" ^
   /I"src\Navigation" ^
   /I"src\Setup" ^
   /I"src\Shell" ^
   /I"src\Types" ^
   /I"src\UI" ^
   /I"src\WindowManager" ^
   /I"src\WindowManager\Navigation" ^
   /I"%imgui_root%" ^
   /I"%imgui_root%\backends" ^
   /I"%imgui_root%\misc" ^
   /I"%font_stuff%" ^
   "%source_file%" ^
   "src\Setup\imgui_fonts.cpp" ^
   "src\UI\Theme.cpp" ^
   "%build_dir%\imgui.obj" ^
   "%build_dir%\imgui_widgets.obj" ^
   "%build_dir%\imgui_draw.obj" ^
   "%build_dir%\imgui_tables.obj" ^
   "%build_dir%\imgui_impl_dx11.obj" ^
   "%build_dir%\imgui_impl_win32.obj" ^
   /Fo"%build_dir%\\" /Fd"%build_dir%\\" /Fe"%build_dir%\main.exe" ^
   /link /DEBUG /OPT:REF /OPT:ICF /ENTRY:mainCRTStartup /SUBSYSTEM:CONSOLE d3d11.lib shell32.lib ole32.lib user32.lib

if %ERRORLEVEL% neq 0 (
    echo.
    echo ❌ BUILD FAILED
    exit /b %ERRORLEVEL%
)

echo.
echo Build complete. Run 'build_release\main.exe' in the Performance Profiler!