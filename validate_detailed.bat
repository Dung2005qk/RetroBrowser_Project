@echo off
REM Detailed Win98 binary validation

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86 >nul 2>&1

echo ============================================================================
echo   Detailed Win98 Binary Validation
echo ============================================================================
echo.

if not exist deploy\RetroBrowser_Win98.exe (
    echo ERROR: Binary not found at deploy\RetroBrowser_Win98.exe
    exit /b 1
)

echo [1/4] PE Header Check...
echo.
dumpbin /headers deploy\RetroBrowser_Win98.exe | findstr /i "machine subsystem linker"
echo.

echo [2/4] Dependencies Check...
echo.
dumpbin /dependents deploy\RetroBrowser_Win98.exe
echo.

echo [3/4] Binary Size Check...
echo.
dir deploy\RetroBrowser_Win98.exe
echo.

echo [4/4] Python Validation...
echo.
python tools\validate_win98_binary.py deploy\RetroBrowser_Win98.exe
echo.

echo ============================================================================
echo   Validation Complete
echo ============================================================================
pause
