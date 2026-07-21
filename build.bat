@echo off
@REM call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

:: 1. Check if file name provided
if "%~1"=="" (
    set "source_file=src\Core\main.cpp"
    set "build_dir=build"
) else (
    set "source_file=%1"
    set "build_dir=%~n1"
)

echo Target Folder: %build_dir%
echo Source File:   %source_file%

if not exist %build_dir% mkdir %build_dir%

set "imgui_root=thirdparty\imgui"
set "font_stuff=thirdparty\fontstuff"

echo Checking Dependencies in %build_dir%
:: Compile ImGui if the object files don't exist 
if not exist %build_dir%\imgui.obj (
    echo [1/2] Compiling ImGui core libraries...
    cl /nologo /c /Z7 /std:c++17 /I"include" /I"%imgui_root%" /I"%imgui_root%\backends" /I"%font_stuff%" ^
       "%imgui_root%\imgui.cpp" ^
       "%imgui_root%\imgui_widgets.cpp" ^
       "%imgui_root%\imgui_draw.cpp" ^
       "%imgui_root%\imgui_tables.cpp" ^
       "%imgui_root%\backends\imgui_impl_dx11.cpp" ^
       "%imgui_root%\backends\imgui_impl_win32.cpp" ^
       /Fo"%build_dir%\\" /Fd"%build_dir%\\"
)

:: Compile project sources and link with cached ImGui .obj files
echo [2/2] Compiling project files and linking...
cl /nologo /Z7 /W4 /MP /std:c++17 ^
   /I"include" ^
   /I"src" ^
   /I"src\Core" ^
   /I"src\Icons" ^
   /I"src\Navigation" ^
   /I"src\Shell" ^
   /I"src\Str" ^
   /I"src\UI" ^
   /I"%imgui_root%" ^
   /I"%imgui_root%\backends" ^
   /I"%font_stuff%" ^
   "%source_file%" ^
   "src\Core\imgui_boilerplate.cpp" ^
   "src\Icons\icons.cpp" ^
   "src\Navigation\Navigation.cpp" ^
   "src\Shell\Shell.cpp" ^
   "src\Str\Str.cpp" ^
   "src\UI\AddressBar.cpp" ^
   "src\UI\CommandBar.cpp" ^
   "src\UI\FileView.cpp" ^
   "src\UI\NavBar.cpp" ^
   "src\UI\ToolBar.cpp" ^
   "src\UI\TopBar.cpp" ^
   "src\UI\UI.cpp" ^
   "%build_dir%\imgui.obj" ^
   "%build_dir%\imgui_widgets.obj" ^
   "%build_dir%\imgui_draw.obj" ^
   "%build_dir%\imgui_tables.obj" ^
   "%build_dir%\imgui_impl_dx11.obj" ^
   "%build_dir%\imgui_impl_win32.obj" ^
   /Fo"%build_dir%\\" /Fd"%build_dir%\\" /Fe"%build_dir%\main.exe" ^
   /link /DEBUG /ENTRY:mainCRTStartup /SUBSYSTEM:CONSOLE d3d11.lib shell32.lib ole32.lib user32.lib

:: Check if build was successful
if %ERRORLEVEL% neq 0 (
    echo.
    echo ❌ BUILD FAILED
    exit /b %ERRORLEVEL%
)

echo.
echo Build complete.