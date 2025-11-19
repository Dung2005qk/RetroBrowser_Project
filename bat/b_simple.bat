@echo off
REM ============================================================================
REM b_simple.bat - Simplified Build Script for Win98
REM ============================================================================
REM This version uses explicit paths and minimal commands for maximum compatibility

echo ========================================
echo Building RetroBrowser for Win98
echo ========================================
echo.

REM --- Setup VC++ 6.0 Environment ---
set VCDIR=C:\PROGRA~1\MICROS~1\VC98
set PATH=%VCDIR%\Bin;C:\WINDOWS;C:\WINDOWS\COMMAND
set INCLUDE=%VCDIR%\Include
set LIB=%VCDIR%\Lib

REM --- Create output directory ---
if not exist obj mkdir obj

REM --- Step 1: Compile Resources ---
echo [1/3] Compiling resources...
%VCDIR%\Bin\rc.exe /fo obj\app.res src\browser\res\app.rc
if errorlevel 1 goto ERROR
echo OK
echo.

REM --- Step 2: Compile C++ Files ---
echo [2/3] Compiling C++ files...
%VCDIR%\Bin\cl.exe /c /nologo /MT /O2 /W3 /GX ^
    /D WIN32 /D NDEBUG /D _WINDOWS /D WINVER=0x0410 ^
    /I src\browser /I src\browser\core /I src\browser\res ^
    /Fo obj\ ^
    src\browser\core\stdafx.cpp ^
    src\browser\core\main.cpp ^
    src\browser\ui\ui.cpp ^
    src\browser\renderer\renderer.cpp ^
    src\browser\parser\parser.cpp ^
    src\browser\network\network.cpp
if errorlevel 1 goto ERROR
echo OK
echo.

REM --- Step 3: Link ---
echo [3/3] Linking...
%VCDIR%\Bin\link.exe /nologo ^
    /OUT:RetroBrowser.exe ^
    /SUBSYSTEM:WINDOWS ^
    /LIBPATH:%VCDIR%\Lib ^
    obj\stdafx.obj ^
    obj\main.obj ^
    obj\ui.obj ^
    obj\renderer.obj ^
    obj\parser.obj ^
    obj\network.obj ^
    obj\app.res ^
    kernel32.lib user32.lib gdi32.lib comctl32.lib ws2_32.lib
if errorlevel 1 goto ERROR
echo OK
echo.

echo ========================================
echo BUILD SUCCESS!
echo Output: RetroBrowser.exe
echo ========================================
goto END

:ERROR
echo.
echo ========================================
echo BUILD FAILED!
echo Check errors above.
echo ========================================
echo.
echo Troubleshooting:
echo 1. Run test_env.bat to check VC++ installation
echo 2. Make sure VC++ 6.0 is installed at C:\Program Files\Microsoft Visual Studio\VC98
echo 3. Check that all source files exist
echo.

:END
pause
