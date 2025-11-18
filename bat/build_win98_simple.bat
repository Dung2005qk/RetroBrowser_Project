@echo off
REM Simplified Win98 build script

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86
if errorlevel 1 goto error_exit

echo.
echo ============================================================================
echo   RetroBrowser - Windows 98 Build
echo ============================================================================
echo.

echo [1/5] Validating environment...
if "%VCINSTALLDIR%"=="" (
    echo ERROR: Visual Studio environment not detected!
    goto error_exit
)
echo   [OK] Visual Studio environment ready
echo.

echo [2/5] Creating directories...
if not exist obj mkdir obj
if not exist deploy mkdir deploy
echo   [OK] Directories ready
echo.

echo [3/5] Setting compiler flags...
set WIN98_DEFINES=/D WIN32 /D _WINDOWS /D _MBCS /D WINVER=0x0410 /D _WIN32_WINNT=0x0410 /D _WIN32_WINDOWS=0x0410 /D _WIN32_IE=0x0500
set COMPILER_FLAGS=/MT /O2 /Oy- /EHsc /W3 /Zi /nologo
set INCLUDE_DIRS=/I src/browser /I src/browser/core /I libs/libjpeg/include
echo   [OK] Compiler flags configured
echo.

echo [4/5] Setting linker flags...
set LINKER_FLAGS=/SUBSYSTEM:WINDOWS,4.10 /MACHINE:X86 /INCREMENTAL:NO /DEBUG /PDB:deploy/RetroBrowser_Win98.pdb
set WIN98_LIBS=kernel32.lib user32.lib gdi32.lib comctl32.lib shell32.lib ws2_32.lib advapi32.lib comdlg32.lib
set THIRD_PARTY_LIBS=libs/libjpeg/lib/jpeg.lib
echo   [OK] Linker flags configured
echo.

echo [5/5] Compiling and linking...
set SOURCE_FILES=src/browser/core/stdafx.cpp src/browser/core/main.cpp src/browser/ui/ui.cpp src/browser/renderer/renderer.cpp src/browser/parser/parser.cpp src/browser/network/network.cpp

cl.exe %COMPILER_FLAGS% %WIN98_DEFINES% %INCLUDE_DIRS% /Fe:deploy/RetroBrowser_Win98.exe /Fo:obj/ %SOURCE_FILES% /link %LINKER_FLAGS% %WIN98_LIBS% %THIRD_PARTY_LIBS%

if errorlevel 1 (
    echo.
    echo [FAILED] Compilation failed!
    goto error_exit
)

if not exist deploy\RetroBrowser_Win98.exe (
    echo.
    echo [FAILED] Binary not created!
    goto error_exit
)

echo.
echo ============================================================================
echo   BUILD SUCCESS!
echo ============================================================================
echo.
echo Output: deploy\RetroBrowser_Win98.exe
dir deploy\RetroBrowser_Win98.exe | find "RetroBrowser_Win98.exe"
echo.
echo [6/6] Patching PE header for Win98 compatibility...
echo.
echo   Fixing subsystem version to 4.10...
editbin.exe /SUBSYSTEM:WINDOWS,4.10 /NOLOGO deploy\RetroBrowser_Win98.exe
if errorlevel 1 (
    echo   [WARNING] editbin failed, but binary may still work
) else (
    echo   [OK] Subsystem version patched to 4.10
)
echo.
echo Running validation...
python tools\validate_win98_binary.py deploy\RetroBrowser_Win98.exe
echo.
echo ============================================================================
pause
exit /b 0

:error_exit
echo.
echo ============================================================================
echo   BUILD FAILED!
echo ============================================================================
pause
exit /b 1
