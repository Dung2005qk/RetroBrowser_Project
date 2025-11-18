@echo off
REM Build RetroBrowser for Win98 using MinGW

echo ============================================================================
echo   RetroBrowser - MinGW Build for Windows 98
echo ============================================================================
echo.

REM Check MinGW
where gcc >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: MinGW GCC not found in PATH!
    pause
    exit /b 1
)

echo [1/4] Checking MinGW...
gcc --version | findstr "gcc"
echo.

echo [2/4] Creating directories...
if not exist obj mkdir obj
if not exist deploy mkdir deploy
echo   [OK] Directories ready
echo.

echo [3/4] Compiling with Win98 compatibility...
echo.

REM Win98 compatibility flags
REM -D WINVER=0x0410          - Target Windows 98
REM -D _WIN32_WINNT=0x0410    - NT 4.0 APIs
REM -D _WIN32_WINDOWS=0x0410  - Win9x 4.10
REM -D _WIN32_IE=0x0500       - IE 5.0
REM -mwindows                 - GUI application
REM -static                   - Static linking (no DLL dependencies)
REM -m32                      - 32-bit x86
REM -O2                       - Optimize for speed
REM -s                        - Strip symbols (smaller exe)

gcc -m32 -mwindows -static -O2 ^
    -D WINVER=0x0410 ^
    -D _WIN32_WINNT=0x0410 ^
    -D _WIN32_WINDOWS=0x0410 ^
    -D _WIN32_IE=0x0500 ^
    -D WIN32 ^
    -D _WINDOWS ^
    -D _MBCS ^
    -I src/browser ^
    -I src/browser/core ^
    -I libs/libjpeg/include ^
    src/browser/core/stdafx.cpp ^
    src/browser/core/main.cpp ^
    src/browser/ui/ui.cpp ^
    src/browser/renderer/renderer.cpp ^
    src/browser/parser/parser.cpp ^
    src/browser/network/network.cpp ^
    -o deploy/RetroBrowser_MinGW.exe ^
    -lstdc++ -lkernel32 -luser32 -lgdi32 -lcomctl32 -lws2_32 -ladvapi32 -lcomdlg32 ^
    libs/libjpeg/lib/jpeg.lib

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [FAILED] Compilation failed!
    pause
    exit /b 1
)

echo.
echo [4/4] Build complete!
echo.
echo ============================================================================
echo   BUILD SUCCESS!
echo ============================================================================
echo.
echo Output: deploy\RetroBrowser_MinGW.exe
dir deploy\RetroBrowser_MinGW.exe | find "RetroBrowser_MinGW.exe"
echo.
echo This exe should work on Windows 98 without KernelEx!
echo Copy to Win98 and test.
echo.
pause
