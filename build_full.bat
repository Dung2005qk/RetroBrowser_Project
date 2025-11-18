@echo off
echo ========================================
echo Building RetroBrowser for Win98
echo ========================================

set PATH=C:\PROGRA~1\MICROS~1\VC98\Bin;C:\PROGRA~1\MICROS~1\Common\MSDev98\Bin;%PATH%
set INCLUDE=C:\PROGRA~1\MICROS~1\VC98\Include
set LIB=C:\PROGRA~1\MICROS~1\VC98\Lib

echo.
echo Compiling source files...
cl /nologo /MT /O2 /W3 /D WIN32 /D NDEBUG /D _WINDOWS ^
   /I src\browser\core ^
   /I src\browser\ui ^
   /I src\browser\network ^
   /I src\browser\parser ^
   /I src\browser\renderer ^
   src\browser\core\stdafx.cpp ^
   src\browser\core\main.cpp ^
   src\browser\ui\ui.cpp ^
   src\browser\network\network.cpp ^
   src\browser\parser\parser.cpp ^
   src\browser\renderer\renderer.cpp ^
   /link /SUBSYSTEM:WINDOWS /OUT:RetroBrowser.exe ^
   user32.lib gdi32.lib ws2_32.lib comctl32.lib shell32.lib

if errorlevel 1 (
    echo.
    echo ========================================
    echo BUILD FAILED!
    echo ========================================
) else (
    echo.
    echo ========================================
    echo BUILD SUCCESS!
    echo Output: RetroBrowser.exe
    echo ========================================
)

pause
