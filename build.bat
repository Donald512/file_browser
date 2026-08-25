@REM @echo off
@REM @REM call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

@REM :: 1. Check if file name provided
@REM if "%~1"=="" (
@REM     set "source_file=src\main.cpp"
@REM     set "build_dir=build"
@REM ) else (
@REM     set "source_file=%1"
@REM     set "build_dir=%~n1"
@REM )

@REM echo Target Folder: %build_dir%
@REM echo Source File:   %source_file%

@REM if not exist %build_dir% mkdir %build_dir%

@REM set "imgui_root=thirdparty\imgui"
@REM set "font_stuff=thirdparty\fontstuff"

@REM echo Checking Dependencies in %build_dir%
@REM :: Compile ImGui if the object files don't exist 
@REM if not exist %build_dir%\imgui.obj (
@REM     echo [1/2] Compiling ImGui core libraries...
@REM     cl  /EHsc /nologo /c /Z7 /std:c++20 /I"include" /I"%imgui_root%" /I"%imgui_root%\backends" /I"%font_stuff%" ^
@REM        "%imgui_root%\imgui.cpp" ^
@REM        "%imgui_root%\imgui_widgets.cpp" ^
@REM        "%imgui_root%\imgui_draw.cpp" ^
@REM        "%imgui_root%\imgui_tables.cpp" ^
@REM        "%imgui_root%\backends\imgui_impl_dx11.cpp" ^
@REM        "%imgui_root%\backends\imgui_impl_win32.cpp" ^
@REM        /Fo"%build_dir%\\" /Fd"%build_dir%\\"
@REM )

@REM :: Compile project sources and link with cached ImGui .obj files
@REM echo [2/2] Compiling project files and linking...
@REM cl  /EHsc /nologo /Z7 /W4 /MP /std:c++20 ^
@REM    /I"include" ^
@REM    /I"src" ^
@REM    /I"src\Global" ^
@REM    /I"src\Global\CtxMenu" ^
@REM    /I"src\Global\Icons" ^
@REM    /I"src\Global\Textures" ^
@REM    /I"src\Navigation" ^
@REM    /I"src\Setup" ^
@REM    /I"src\Shell" ^
@REM    /I"src\Types" ^
@REM    /I"src\UI" ^
@REM    /I"src\WindowManager" ^
@REM    /I"src\WindowManager\Navigation" ^
@REM    /I"%imgui_root%" ^
@REM    /I"%imgui_root%\backends" ^
@REM    /I"%imgui_root%\misc" ^
@REM    /I"%font_stuff%" ^
@REM    "%source_file%" ^
@REM    "src\Global\ClipboardManager.cpp" ^
@REM    "src\Setup\imgui_fonts.cpp" ^
@REM    "src\UI\Theme.cpp" ^
@REM    "%build_dir%\imgui.obj" ^
@REM    "%build_dir%\imgui_widgets.obj" ^
@REM    "%build_dir%\imgui_draw.obj" ^
@REM    "%build_dir%\imgui_tables.obj" ^
@REM    "%build_dir%\imgui_impl_dx11.obj" ^
@REM    "%build_dir%\imgui_impl_win32.obj" ^
@REM    /Fo"%build_dir%\\" /Fd"%build_dir%\\" /Fe"%build_dir%\main.exe" ^
@REM    /link /DEBUG /ENTRY:mainCRTStartup /SUBSYSTEM:CONSOLE d3d11.lib shell32.lib ole32.lib user32.lib

@REM :: Check if build was successful
@REM if %ERRORLEVEL% neq 0 (
@REM     echo.
@REM     echo ❌ BUILD FAILED
@REM     exit /b %ERRORLEVEL%
@REM )

@REM echo.
@REM echo Build complete.

@echo off
@echo off
setlocal enabledelayedexpansion

if "%~1"=="" (
    set "source_file=src\main.cpp"
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

if not exist %build_dir%\imgui.obj (
    echo [1/2] Compiling ImGui core libraries...
    cl /EHsc /nologo /c /Z7 /std:c++20 /I"include" /I"%imgui_root%" /I"%imgui_root%\backends" /I"%font_stuff%" ^
       "%imgui_root%\imgui.cpp" ^
       "%imgui_root%\imgui_widgets.cpp" ^
       "%imgui_root%\imgui_draw.cpp" ^
       "%imgui_root%\imgui_tables.cpp" ^
       "%imgui_root%\backends\imgui_impl_dx11.cpp" ^
       "%imgui_root%\backends\imgui_impl_win32.cpp" ^
       /Fo"%build_dir%\\" /Fd"%build_dir%\\" > nul
)

:: 1. Dynamically find ALL subdirectories under src\ for includes
set "inc_flags=/I"src""
for /D /R "src" %%d in (*) do (
    set "inc_flags=!inc_flags! /I"%%d""
)

:: 2. Dynamically collect all .cpp files under src\ (except main.cpp)
set "project_sources="
for /R "src" %%f in (*.cpp) do (
    if /I not "%%~nxf"=="main.cpp" (
        set "project_sources=!project_sources! "%%f""
    )
)

echo [2/2] Compiling project files and linking...

:: 3. Save cl output to a log file to preserve real ERRORLEVEL
cl /EHsc /nologo /Z7 /W4 /MP /std:c++20 ^
   /I"include" ^
   !inc_flags! ^
   /I"%imgui_root%" ^
   /I"%imgui_root%\backends" ^
   /I"%imgui_root%\misc" ^
   /I"%font_stuff%" ^
   "%source_file%" ^
   !project_sources! ^
   "%build_dir%\imgui.obj" ^
   "%build_dir%\imgui_widgets.obj" ^
   "%build_dir%\imgui_draw.obj" ^
   "%build_dir%\imgui_tables.obj" ^
   "%build_dir%\imgui_impl_dx11.obj" ^
   "%build_dir%\imgui_impl_win32.obj" ^
   /Fo"%build_dir%\\" /Fd"%build_dir%\\" /Fe"%build_dir%\main.exe" ^
   /link /DEBUG /ENTRY:mainCRTStartup /SUBSYSTEM:CONSOLE d3d11.lib shell32.lib ole32.lib user32.lib > %build_dir%\build.log 2>&1

set "BUILD_STATUS=%ERRORLEVEL%"

:: Print the filtered log to console without messing up error checks
type %build_dir%\build.log | findstr /V /R /C:"^[a-zA-Z0-9_]*\.cpp$"

if %BUILD_STATUS% neq 0 (
    echo.
    echo ❌ BUILD FAILED
    exit /b %BUILD_STATUS%
)

echo.
echo Build complete.