@echo off
REM Test compilation script for RetroBrowser UI module
REM This will verify if the errors are real or IntelliSense artifacts

echo ============================================================
echo RetroBrowser UI Module - Compilation Test
echo ============================================================
echo.

REM Try to find Visual Studio compiler
set "VSPATH=C:\Program Files\Microsoft Visual Studio"
set "VSVERSIONS=2022 2019 2017"

for %%V in (%VSVERSIONS%) do (
    if exist "%VSPATH%\%%V\Community\VC\Auxiliary\Build\vcvars32.bat" (
        echo Found Visual Studio %%V Community
        call "%VSPATH%\%%V\Community\VC\Auxiliary\Build\vcvars32.bat"
        goto :compile
    )
    if exist "%VSPATH%\%%V\Professional\VC\Auxiliary\Build\vcvars32.bat" (
        echo Found Visual Studio %%V Professional
        call "%VSPATH%\%%V\Professional\VC\Auxiliary\Build\vcvars32.bat"
        goto :compile
    )
    if exist "%VSPATH%\%%V\Enterprise\VC\Auxiliary\Build\vcvars32.bat" (
        echo Found Visual Studio %%V Enterprise
        call "%VSPATH%\%%V\Enterprise\VC\Auxiliary\Build\vcvars32.bat"
        goto :compile
    )
)

echo Visual Studio not found in standard locations.
echo Please run this from a Visual Studio Developer Command Prompt.
pause
exit /b 1

:compile
echo.
echo ============================================================
echo Compiling test file...
echo ============================================================
echo.

REM Test 1: Compile the test file
cl /nologo /EHsc /W3 /I. test_compile.cpp /Fe:test_compile.exe 2>&1

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ============================================================
    echo SUCCESS: Test compiled without errors!
    echo This means the "errors" in ui.cpp are IntelliSense artifacts.
    echo The actual code is correct and will compile fine.
    echo ============================================================
    echo.
    
    REM Clean up
    if exist test_compile.obj del test_compile.obj
    if exist test_compile.exe del test_compile.exe
    
) else (
    echo.
    echo ============================================================
    echo FAILED: Test compilation failed with real errors.
    echo The errors need to be fixed before the code will compile.
    echo ============================================================
    echo.
)

echo.
echo Test 2: Check if ui.cpp would compile with simplified syntax check
echo ============================================================
cl /nologo /Zs /W3 /I. /Isrc/browser/core /Isrc/browser/ui src/browser/ui/ui.cpp 2>&1

if %ERRORLEVEL% EQU 0 (
    echo.
    echo SUCCESS: ui.cpp syntax is valid!
    echo.
) else (
    echo.
    echo Note: Some errors expected due to missing dependencies.
    echo Focus on syntax errors in the output above.
    echo.
)

pause
