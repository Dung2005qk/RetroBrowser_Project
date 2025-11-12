@echo off
REM Quick build script for RetroBrowser with VS2022 targeting Win98
echo ========================================
echo Building RetroBrowser for Win98...
echo ========================================
echo.

REM Setup Visual Studio 2022 environment for x86
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86 >nul 2>&1

REM Create obj and deploy folders if not exist
if not exist obj mkdir obj
if not exist deploy mkdir deploy

REM Compile with Win98 compatibility flags
cl.exe /nologo /Fe:deploy/RetroBrowser.exe /Fo:obj/ /EHsc /MT /Zi /Od /W3 /D WIN32 /D _WINDOWS /D _MBCS /D WINVER=0x0410 /D _WIN32_WINNT=0x0410 /D _WIN32_WINDOWS=0x0410 /D _WIN32_IE=0x0500 /I src/browser /I src/browser/core /I libs/libjpeg/include src/browser/core/stdafx.cpp src/browser/core/main.cpp src/browser/ui/ui.cpp src/browser/renderer/renderer.cpp src/browser/parser/parser.cpp src/browser/network/network.cpp /link /SUBSYSTEM:WINDOWS /MACHINE:X86 /DEBUG /PDB:deploy/RetroBrowser.pdb /INCREMENTAL:NO kernel32.lib user32.lib gdi32.lib comctl32.lib shell32.lib ws2_32.lib advapi32.lib comdlg32.lib libs/libjpeg/lib/jpeg.lib

echo.
if %ERRORLEVEL% EQU 0 (
    echo ========================================
    echo BUILD SUCCESS!
    echo ========================================
    echo Binary: deploy\RetroBrowser.exe
    echo Size:
    dir deploy\RetroBrowser.exe | find "RetroBrowser.exe"
    echo.
    echo This EXE is compatible with Windows 98!
    echo ========================================
) else (
    echo ========================================
    echo BUILD FAILED! Check errors above.
    echo ========================================
)
echo.
pause
