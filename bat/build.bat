@echo off
echo Setting up Visual C++ environment...
call "C:\Program Files\Microsoft Visual Studio\VC98\Bin\vcvars32.bat"

echo.
echo Building RetroBrowser...
cl /nologo /MT /O2 /W3 /EHsc /D WIN32 /D NDEBUG /D _WINDOWS /I src\browser /I src\browser\core /I libs\libpng\include src\browser\core\stdafx.cpp src\browser\core\main.cpp /link /OUT:RetroBrowser.exe

if errorlevel 1 (
    echo Build FAILED!
) else (
    echo Build SUCCESS!
)

pause
