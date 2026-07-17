@echo off

:: 1. Check if file name provided
if "%~1"=="" (
    set "source_file=src\main.cpp"
    set "build_dir=build"
) else (
    set "source_file=%1"
    set "build_dir=%~n1"
)
:: %~n1 grabs only the name of the first argument, no extension

echo Target Folder: %build_dir%
echo Source File:   %source_file%


if not exist %build_dir% mkdir %build_dir%

set "imgui_root=thirdparty\imgui"
set "font_stuff=thirdparty\fontstuff"

echo Checking Dependences in %build_dir%
:: Compile ImGui if the object files don't exist 
if not exist %build_dir%\imgui.obj (
    echo [1/2] Compiling ImGui core libraries...
    cl /nologo /c /Z7 /std:c++17 /I"include" /I"%imgui_root%" /I"%imgui_root%\backends" /I"%font_stuff%"^
       "%imgui_root%\imgui.cpp" ^
       "%imgui_root%\imgui_widgets.cpp" ^
       "%imgui_root%\imgui_draw.cpp" ^
       "%imgui_root%\imgui_tables.cpp" ^
       "%imgui_root%\backends\imgui_impl_dx11.cpp" ^
       "%imgui_root%\backends\imgui_impl_win32.cpp" ^
       /Fo"%build_dir%\\" /Fd"%build_dir%\\"
)
:: the // are tres important 
:: /c flag makes the compiler only compile, and not call the linker

:: calling the linker manually
echo [2/2] Compiling %source_file%
:: Compile source_file and link it with the cached .obj files
cl /nologo /Z7 /W4 /MP /std:c++17 /I"include" /I"%imgui_root%" /I"%imgui_root%\backends" /I"%font_stuff%"^
   "%source_file%" "src\imgui_boilerplate.cpp" "src\backend.cpp" "src\string.cpp"^
   "src\renderer.cpp" "src\utils.cpp" "src\navigation.cpp" "src\history.cpp" "src\icons.cpp"^
   "%build_dir%\imgui.obj" ^
   "%build_dir%\imgui_widgets.obj" ^
   "%build_dir%\imgui_draw.obj" ^
   "%build_dir%\imgui_tables.obj" ^
   "%build_dir%\imgui_impl_dx11.obj" ^
   "%build_dir%\imgui_impl_win32.obj" ^
   /Fo"%build_dir%\\" /Fd"%build_dir%\\" /Fe"%build_dir%\main.exe" ^
   /link /DEBUG /ENTRY:mainCRTStartup /SUBSYSTEM:CONSOLE d3d11.lib
@REM    /link /DEBUG /ENTRY:mainCRTStartup /SUBSYSTEM:WINDOWS d3d11.lib

:: Check if Build succesful
if %ERRORLEVEL% neq 0 (
    echo .
    echo ❌ BUILD FAILED
    exit /b %ERRORLEVEL%
)

echo.
echo Build complete

:: neq 0 means not equal to 0