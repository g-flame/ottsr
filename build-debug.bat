@echo off
echo Building OTTSR (Debug)...

if not exist "build" mkdir build

echo Compiling debug version...
call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if errorlevel 1 (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
)

cl.exe /Fe:build\ottsr_debug.exe /Zi /Od /MDd /D_DEBUG src\ottsr.c /link user32.lib gdi32.lib kernel32.lib comctl32.lib winmm.lib

if errorlevel 1 (
    echo Debug build failed!
    pause
    exit /b 1
)

echo Debug build completed!
echo Output: build\ottsr_debug.exe
pause
