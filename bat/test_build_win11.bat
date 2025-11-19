@echo off
REM ============================================================================
REM test_build_win11.bat - Quick Build Test on Win11
REM ============================================================================
REM PURPOSE: Fast syntax/compile check on modern Windows before deploying to Win98
REM NOTE: This does NOT guarantee Win98 compatibility, only checks if code compiles
REM ============================================================================

echo ========================================
echo Quick Build Test for Win11
echo ========================================
echo.

REM --- Check if VC++ 6.0 is installed ---
if not exist "C:\PROGRA~1\MICROS~1\VC98\Bin\cl.exe" (
    echo ERROR: Visual C++ 6.0 not found!
    echo.
    echo You need to install VC++ 6.0 on Win11 first.
    echo Alternative: Use Visual Studio 2022 with v140_xp toolset
    echo.
    pause
    exit /b 1
)

REM --- Set VC++ 6.0 environment ---
set PATH=C:\PROGRA~1\MICROS~1\VC98\Bin;C:\PROGRA~1\MICROS~1\Common\MSDev98\Bin;%PATH%
set INCLUDE=C:\PROGRA~1\MICROS~1\VC98\Include;C:\PROGRA~1\MICROS~1\VC98\ATL\Include;C:\PROGRA~1\MICROS~1\VC98\MFC\Include
set LIB=C:\PROGRA~1\MICROS~1\VC98\Lib;C:\PROGRA~1\MICROS~1\VC98\MFC\Lib

echo Testing compilation...
echo.

REM --- Compile (same as b.bat) ---
cl.exe ^
    /nologo ^
    /MT ^
    /O2 ^
    /W3 ^
    /GX ^
    /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "WINVER=0x0410" ^
    /I "src\browser" /I "src\browser\core" /I "libs\libjpeg\include" /I "res" ^
    /Fe"RetroBrowser_Win11Test.exe" ^
    src\browser\core\stdafx.cpp ^
    src\browser\core\main.cpp ^
    src\browser\ui\ui.cpp ^
    src\browser\renderer\renderer.cpp ^
    src\browser\parser\parser.cpp ^
    src\browser\network\network.cpp ^
    res\app.rc ^
    /link kernel32.lib user32.lib gdi32.lib comctl32.lib ws2_32.lib advapi32.lib comdlg32.lib shell32.lib libs\libjpeg\lib\jpeg.lib

if errorlevel 1 (
    echo.
    echo ========================================
    echo BUILD FAILED - Fix errors above
    echo ========================================
    pause
    exit /b 1
)

echo.
echo ========================================
echo BUILD SUCCESS on Win11!
echo ========================================
echo.
echo Output: RetroBrowser_Win11Test.exe
echo.
echo IMPORTANT NOTES:
echo - This build may use Win11 APIs
echo - You MUST rebuild on Win98 for final deployment
echo - This is only for quick syntax checking
echo.
echo Next steps:
echo 1. Test run on Win11: RetroBrowser_Win11Test.exe
echo 2. Copy source to Win98 VM
echo 3. Run b.bat on Win98 for production build
echo ========================================
pause
