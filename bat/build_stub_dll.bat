@echo off
REM Build stub DLL for Win98 missing APIs

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86 >nul 2>&1

echo Building kernel32 stub DLL for Win98...

cl /LD /MT /O2 /D WINVER=0x0410 src\win98_stub\kernel32_stub.c /link /DEF:src\win98_stub\kernel32_stub.def /OUT:deploy\kernel32_stub.dll /SUBSYSTEM:WINDOWS,4.10

if %ERRORLEVEL% EQU 0 (
    echo.
    echo SUCCESS! Copy kernel32_stub.dll to Win98 alongside RetroBrowser_Win98.exe
    echo.
) else (
    echo.
    echo BUILD FAILED!
)

pause
