
@echo off
setlocal enabledelayedexpansion

:: --- Config ---
set "imgui_root=thirdparty\imgui"
set "font_stuff=thirdparty\fontstuff"
set "build_dir=build"
set "source_file=src\main.cpp"

:: Get the raw name of the source file (e.g., "main" from "src\main.cpp")
for %%A in ("%source_file%") do set "target_name=%%~nA"

:: We use this flag to decide if we need to run the final linker
set "needs_link=false"

if not exist %build_dir% mkdir %build_dir%

:: Compile ImGui if the object files don't exist 
if not exist %build_dir%\imgui.obj (
    echo [1/3] Compiling ImGui core libraries...
    cl /nologo /c /Z7 /std:c++17 /I"include" /I"%imgui_root%" /I"%imgui_root%\backends" /I"%font_stuff%"^
        "%imgui_root%\imgui.cpp" ^
        "%imgui_root%\imgui_widgets.cpp" ^
        "%imgui_root%\imgui_draw.cpp" ^
        "%imgui_root%\imgui_tables.cpp" ^
        "%imgui_root%\backends\imgui_impl_dx11.cpp" ^
        "%imgui_root%\backends\imgui_impl_win32.cpp" ^
        /Fo"%build_dir%\\" /Fd"%build_dir%\\"
    if !errorlevel! neq 0 exit /b !errorlevel!
    set "needs_link=true"
)
:: /c flag makes the compiler only compile, and not call the linker

:: Compile source files (including main.cpp)
echo [2/3] Checking source files...
set "src_files=imgui_boilerplate backend string renderer utils navigation history icons main"

for %%s in (%src_files%) do (
    set "obj_file=%build_dir%\%%s.obj"
    set "cpp_file=src\%%s.cpp"
    set "need_compile=false"

    if not exist "!obj_file!" (
        set "need_compile=true"
    ) else (
        :: Compare exact modification timestamps natively (no xcopy subshells needed)
        for %%F in ("!cpp_file!") do set "cpp_time=%%~tF"
        for %%F in ("!obj_file!") do set "obj_time=%%~tF"
        
        if "!cpp_time!" gtr "!obj_time!" set "need_compile=true"
    )

    if "!need_compile!"=="true" (
        echo   Compiling %%s.cpp...
        cl /nologo /c /Z7 /W4 /std:c++17 /I"include" /I"%imgui_root%" /I"%imgui_root%\backends" /I"%font_stuff%"^
            "!cpp_file!" /Fo"%build_dir%\\" /Fd"%build_dir%\\"
        if !errorlevel! neq 0 exit /b !errorlevel!
        set "needs_link=true"
    )
)

:: If the EXE was deleted, we must link regardless
set "target_exe=%build_dir%\%target_name%.exe"
if not exist "%target_exe%" set "needs_link=true"

if "%needs_link%"=="true" (
    echo [3/3] Linking executable: %target_exe%...
    
    REM We use link.exe directly because all files are already compiled .obj files
    link /nologo /OUT:"%target_exe%" /DEBUG /ENTRY:mainCRTStartup /SUBSYSTEM:CONSOLE d3d11.lib ^
       "%build_dir%\main.obj" ^
       "%build_dir%\imgui_boilerplate.obj" ^
       "%build_dir%\backend.obj" ^
       "%build_dir%\string.obj" ^
       "%build_dir%\renderer.obj" ^
       "%build_dir%\utils.obj" ^
       "%build_dir%\navigation.obj" ^
       "%build_dir%\history.obj" ^
       "%build_dir%\icons.obj" ^
       "%build_dir%\imgui.obj" ^
       "%build_dir%\imgui_widgets.obj" ^
       "%build_dir%\imgui_draw.obj" ^
       "%build_dir%\imgui_tables.obj" ^
       "%build_dir%\imgui_impl_dx11.obj" ^
       "%build_dir%\imgui_impl_win32.obj"
       
    if !errorlevel! neq 0 (
        echo.
        echo ❌ LINKING FAILED
        exit /b !errorlevel!
    )
) else (
    echo [3/3] Executable is up to date. Skipping linking!
)

echo.
echo Build complete.