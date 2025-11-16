@echo off
REM Win98 Binary Validation Wrapper
REM Usage: validate_win98.bat <path_to_exe>
REM Example: validate_win98.bat deploy\RetroBrowser_Win98.exe

if "%1"=="" (
    echo Usage: validate_win98.bat ^<path_to_exe^>
    echo Example: validate_win98.bat deploy\RetroBrowser_Win98.exe
    exit /b 1
)

python tools\validate_win98_binary.py %1
exit /b %ERRORLEVEL%
