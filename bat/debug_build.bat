@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86 >nul 2>&1

echo Testing line by line...

REM Test 1
if "%VCINSTALLDIR%"=="" (
    echo Test 1 FAILED
) else (
    echo Test 1 OK
)

REM Test 2
set ARCH_OK=0
if "%VSCMD_ARG_TGT_ARCH%"=="x86" set ARCH_OK=1
if "%Platform%"=="x86" set ARCH_OK=1
if "%ARCH_OK%"=="0" (
    echo Test 2 FAILED
) else (
    echo Test 2 OK
)

REM Test 3
where cl.exe >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Test 3 FAILED
) else (
    echo Test 3 OK
)

REM Test 4
where link.exe >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Test 4 FAILED
) else (
    echo Test 4 OK
)

REM Test 5
if not exist obj (
    echo Test 5: obj not exist
) else (
    echo Test 5: obj exists
)

echo.
echo All tests completed
pause
