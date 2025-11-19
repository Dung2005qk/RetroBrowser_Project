@echo off
REM ============================================================================
REM check_vc6_compat.bat - Comprehensive VC++6.0 Compatibility Check
REM ============================================================================
echo Checking all source files for VC++6.0 compatibility issues...
echo.

set ISSUES_FOUND=0

echo [1/5] Checking for .clear() method calls...
findstr /S /N /C:".clear()" src\browser\*.cpp > nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo    ERROR: Found .clear^(^) calls - not supported in VC++6.0
    findstr /S /N /C:".clear()" src\browser\*.cpp
    set ISSUES_FOUND=1
) else (
    echo    OK: No .clear^(^) calls found
)
echo.

echo [2/5] Checking for .data() method calls...
findstr /S /N /C:".data()" src\browser\*.cpp > nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo    ERROR: Found .data^(^) calls - not supported in VC++6.0
    findstr /S /N /C:".data()" src\browser\*.cpp
    set ISSUES_FOUND=1
) else (
    echo    OK: No .data^(^) calls found
)
echo.

echo [3/5] Checking for DEBUG_LOGF usage...
findstr /S /N /C:"DEBUG_LOGF" src\browser\*.cpp > nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo    ERROR: Found DEBUG_LOGF calls - macro removed
    findstr /S /N /C:"DEBUG_LOGF" src\browser\*.cpp
    set ISSUES_FOUND=1
) else (
    echo    OK: No DEBUG_LOGF calls found
)
echo.

echo [4/5] Checking for nullptr keyword...
findstr /S /N /C:"nullptr" src\browser\*.cpp > nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo    WARNING: Found nullptr - use NULL instead
    findstr /S /N /C:"nullptr" src\browser\*.cpp
    set ISSUES_FOUND=1
) else (
    echo    OK: No nullptr found
)
echo.

echo [5/5] Checking for auto keyword...
findstr /S /N /R "\<auto\>" src\browser\*.cpp > nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo    WARNING: Found auto keyword - not supported in VC++6.0
    findstr /S /N /R "\<auto\>" src\browser\*.cpp
    set ISSUES_FOUND=1
) else (
    echo    OK: No auto keyword found
)
echo.

echo ============================================
if %ISSUES_FOUND% EQU 0 (
    echo RESULT: All checks passed! Code is VC++6.0 compatible.
    echo You can now run b.bat to build.
) else (
    echo RESULT: Issues found! Fix them before building.
)
echo ============================================
pause
