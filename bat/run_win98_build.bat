@echo off
REM Wrapper script to set up VS environment and run Win98 build

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86

if %ERRORLEVEL% NEQ 0 (
    echo Failed to initialize Visual Studio environment
    exit /b 1
)

call build_win98.bat
