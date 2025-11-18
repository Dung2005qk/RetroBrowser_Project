@echo off
REM Compare Win98 build with original build

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86 >nul 2>&1

echo ============================================================================
echo   Comparing Win98 Build vs Original Build
echo ============================================================================
echo.

if not exist deploy\RetroBrowser.exe (
    echo ERROR: Original build not found at deploy\RetroBrowser.exe
    exit /b 1
)

if not exist deploy\RetroBrowser_Win98.exe (
    echo ERROR: Win98 build not found at deploy\RetroBrowser_Win98.exe
    exit /b 1
)

echo [1/3] File Size Comparison
echo ============================================================================
echo.
echo Original Build:
dir deploy\RetroBrowser.exe | find "RetroBrowser.exe"
echo.
echo Win98 Build:
dir deploy\RetroBrowser_Win98.exe | find "RetroBrowser_Win98.exe"
echo.

echo [2/3] PE Header Comparison
echo ============================================================================
echo.
echo --- Original Build ---
dumpbin /headers deploy\RetroBrowser.exe | findstr /i "machine subsystem linker"
echo.
echo --- Win98 Build ---
dumpbin /headers deploy\RetroBrowser_Win98.exe | findstr /i "machine subsystem linker"
echo.

echo [3/3] Dependencies Comparison
echo ============================================================================
echo.
echo --- Original Build Dependencies ---
dumpbin /dependents deploy\RetroBrowser.exe | findstr /i ".dll"
echo.
echo --- Win98 Build Dependencies ---
dumpbin /dependents deploy\RetroBrowser_Win98.exe | findstr /i ".dll"
echo.

echo ============================================================================
echo   Comparison Complete
echo ============================================================================
echo.
echo KEY DIFFERENCES:
echo   - Subsystem version: Original likely 6.0+, Win98 is 4.10
echo   - Dependencies: Original may have MSVCRT140.dll, Win98 is statically linked
echo   - Size: Win98 build may be larger due to static CRT linking
echo.
pause
