@echo off
REM Test both binaries on current Windows

echo Testing Original Build (Subsystem 6.00)...
deploy\RetroBrowser.exe --version 2>nul
if %ERRORLEVEL% EQU 0 (
    echo [OK] Original build runs on Windows 11
) else (
    echo [FAIL] Original build failed
)

echo.
echo Testing Win98 Build (Subsystem 4.10)...
deploy\RetroBrowser_Win98.exe --version 2>nul
if %ERRORLEVEL% EQU 0 (
    echo [OK] Win98 build runs on Windows 11
) else (
    echo [FAIL] Win98 build failed
)

echo.
echo Both builds should run on Windows 11 due to backward compatibility.
echo But only Win98 build will run on actual Windows 98.
pause
