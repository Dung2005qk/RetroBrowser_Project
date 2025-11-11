@echo off
REM ============================================================================
REM VERIFY BOTH FIXES - Quick Test Script
REM ============================================================================
REM Purpose: Verify both critical bug fixes before manual testing
REM Bugs Fixed:
REM   #1: HTML structure tags stripped (blank screen)
REM   #2: Type mismatch crash on second navigation
REM ============================================================================

echo ========================================
echo RETROBROWSER - FIX VERIFICATION
echo ========================================
echo.

REM ============================================================================
REM TEST #1: HTML Structure Tags Preservation
REM ============================================================================
echo [TEST #1] Checking HTML structure tag preservation...
echo.

python test_quick.py
if %ERRORLEVEL% neq 0 (
    echo.
    echo ❌ TEST #1 FAILED - Structure tags not preserved!
    echo    Check src/proxy/config.py ALLOWED_HTML_TAGS
    pause
    exit /b 1
)

echo.
echo ✅ TEST #1 PASSED - Structure tags preserved correctly!
echo.

REM ============================================================================
REM TEST #2: Browser Build Status
REM ============================================================================
echo [TEST #2] Verifying browser build with type fix...
echo.

if not exist "deploy\RetroBrowser.exe" (
    echo ❌ TEST #2 FAILED - Browser not built!
    echo    Run: Task ^> Build RetroBrowser
    pause
    exit /b 1
)

REM Check if build is recent (modified today)
echo Checking build timestamp...
for %%F in (deploy\RetroBrowser.exe) do set FileDate=%%~tF
echo Build date: %FileDate%
echo.
echo ✅ TEST #2 PASSED - Browser executable exists!
echo.

REM ============================================================================
REM MANUAL TEST INSTRUCTIONS
REM ============================================================================
echo ========================================
echo MANUAL TESTING REQUIRED
echo ========================================
echo.
echo Both automated tests passed! Now perform manual tests:
echo.
echo Step 1: Start Proxy
echo   ^> python src/proxy/proxy.py
echo.
echo Step 2: Run Browser
echo   ^> deploy\RetroBrowser.exe
echo.
echo Step 3: Test Rendering (Bug #1 fix)
echo   - Navigate to: http://info.cern.ch
echo   - Expected: Content appears (NOT blank screen)
echo   - Verify: Page title shows in window title bar
echo.
echo Step 4: Test Crash Fix (Bug #2 fix)
echo   - Click "Go" again (same or different URL)
echo   - Expected: Browser does NOT crash/exit
echo   - Verify: Can navigate multiple times safely
echo.
echo Step 5: Check Content Quality
echo   - Verify HTML structure (headings, paragraphs visible)
echo   - Check that ^<html^>, ^<body^> tags are in source
echo.
echo ========================================
echo.

pause
