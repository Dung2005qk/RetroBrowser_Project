@echo off
REM Prepare Win98 deployment package

echo ============================================================================
echo   Preparing Win98 Deployment Package
echo ============================================================================
echo.

if not exist deploy\RetroBrowser_Win98.exe (
    echo ERROR: Win98 binary not found!
    echo Please run build_win98.bat first.
    pause
    exit /b 1
)

echo [1/4] Creating deployment directory...
if not exist deploy\win98_package mkdir deploy\win98_package
if not exist deploy\win98_package\docs mkdir deploy\win98_package\docs
echo   [OK] Directories created
echo.

echo [2/4] Copying binary...
copy deploy\RetroBrowser_Win98.exe deploy\win98_package\RetroBrowser.exe
echo   [OK] Binary copied
echo.

echo [3/4] Copying documentation...
copy docs\win98_vm_testing_instructions.md deploy\win98_package\docs\
copy docs\win98_testing_guide.md deploy\win98_package\docs\
copy docs\win98_compatibility_report.md deploy\win98_package\docs\
copy README.md deploy\win98_package\
echo   [OK] Documentation copied
echo.

echo [4/4] Creating README for Win98 users...
(
echo RetroBrowser for Windows 98
echo ===========================
echo.
echo SYSTEM REQUIREMENTS:
echo   - Windows 98 Second Edition
echo   - 64MB RAM minimum, 128MB recommended
echo   - 200MHz Pentium or faster
echo   - Winsock 2.2 ^(included in Win98 SE^)
echo.
echo INSTALLATION:
echo   1. Copy RetroBrowser.exe to C:\RetroBrowser\
echo   2. Double-click to run
echo   3. Configure proxy settings if needed
echo.
echo PROXY SETUP:
echo   - Host: Your host machine IP
echo   - Port: 8080
echo   - Run proxy server on host: python src/proxy/proxy.py
echo.
echo TROUBLESHOOTING:
echo   - If "procedure not found" error: Install Winsock 2.2 update
echo   - If network fails: Check proxy server is running
echo   - If crashes: Ensure at least 64MB RAM available
echo.
echo For detailed instructions, see docs\win98_vm_testing_instructions.md
echo.
echo Build Date: %DATE% %TIME%
echo Build Version: Win98 Compatible ^(Subsystem 4.10^)
) > deploy\win98_package\README_WIN98.txt

echo   [OK] README created
echo.

echo ============================================================================
echo   Deployment Package Ready!
echo ============================================================================
echo.
echo Package location: deploy\win98_package\
echo.
echo Contents:
dir /b deploy\win98_package
echo.
echo NEXT STEPS:
echo   1. Transfer deploy\win98_package\ to Win98 VM
echo   2. Follow instructions in docs\win98_vm_testing_instructions.md
echo   3. Start proxy server: python src\proxy\proxy.py
echo   4. Run RetroBrowser.exe on Win98
echo.
echo ============================================================================
pause
