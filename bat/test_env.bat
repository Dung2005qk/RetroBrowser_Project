@echo off
REM ============================================================================
REM test_env.bat - Test VC++6.0 Environment on Win98
REM ============================================================================
echo Testing Visual C++ 6.0 environment...
echo.

REM --- Set environment ---
set PATH=C:\PROGRA~1\MICROS~1\VC98\Bin;C:\PROGRA~1\MICROS~1\Common\MSDev98\Bin;C:\WINDOWS;C:\WINDOWS\COMMAND
set INCLUDE=C:\PROGRA~1\MICROS~1\VC98\Include
set LIB=C:\PROGRA~1\MICROS~1\VC98\Lib

echo [1] Checking PATH...
echo PATH=%PATH%
echo.

echo [2] Checking INCLUDE...
echo INCLUDE=%INCLUDE%
echo.

echo [3] Checking LIB...
echo LIB=%LIB%
echo.

echo [4] Checking if cl.exe exists...
if exist "C:\PROGRA~1\MICROS~1\VC98\Bin\cl.exe" (
    echo OK: cl.exe found
) else (
    echo ERROR: cl.exe not found!
)
echo.

echo [5] Checking if link.exe exists...
if exist "C:\PROGRA~1\MICROS~1\VC98\Bin\link.exe" (
    echo OK: link.exe found
) else (
    echo ERROR: link.exe not found!
)
echo.

echo [6] Checking if kernel32.lib exists...
if exist "C:\PROGRA~1\MICROS~1\VC98\Lib\kernel32.lib" (
    echo OK: kernel32.lib found
) else (
    echo ERROR: kernel32.lib not found!
    echo.
    echo Possible causes:
    echo - VC++ 6.0 not installed correctly
    echo - Wrong installation path
    echo - Libraries not installed
    echo.
    echo Try these paths:
    dir /b C:\PROGRA~1\MICROS~1\VC98\Lib\*.lib
)
echo.

echo [7] Testing cl.exe version...
cl.exe 2>&1 | find "Microsoft"
echo.

echo ============================================
echo Environment test complete!
echo ============================================
pause
