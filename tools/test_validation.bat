@echo off
REM Test script for Win98 binary validation
REM This script demonstrates the validation tool functionality

echo ============================================================================
echo   Win98 Binary Validation - Test Script
echo ============================================================================
echo.

echo Testing validation on existing binary...
echo.
echo [Test 1] Validating deploy\RetroBrowser.exe
echo Expected: FAIL (subsystem version 6.00)
echo.
python tools\validate_win98_binary.py deploy\RetroBrowser.exe
echo.
echo Test 1 completed. Exit code: %ERRORLEVEL%
echo.

echo ============================================================================
echo.
echo To test with a Win98-compatible binary:
echo   1. Run: build_win98.bat
echo   2. The validation will run automatically
echo   3. Or manually run: python tools\validate_win98_binary.py deploy\RetroBrowser_Win98.exe
echo.
echo ============================================================================

pause
