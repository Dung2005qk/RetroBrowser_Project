@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86 >nul 2>&1

echo Checking imports in RetroBrowser_Win98.exe...
echo.
dumpbin /imports deploy\RetroBrowser_Win98.exe | findstr /i "SetFilePointer"
echo.
echo Checking all KERNEL32 imports...
dumpbin /imports deploy\RetroBrowser_Win98.exe | findstr /i "KERNEL32" -A 50
pause
