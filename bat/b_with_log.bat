@echo off
REM --- Build with full error logging ---
REM Thiết lập môi trường cho Visual C++ 6.0
set PATH=C:\PROGRA~1\MICROS~1\VC98\Bin;C:\PROGRA~1\MICROS~1\Common\MSDev98\Bin;C:\WINDOWS;C:\WINDOWS\COMMAND
set INCLUDE=C:\PROGRA~1\MICROS~1\VC98\Include;C:\PROGRA~1\MICROS~1\VC98\ATL\Include;C:\PROGRA~1\MICROS~1\VC98\MFC\Include
set LIB=C:\PROGRA~1\MICROS~1\VC98\Lib;C:\PROGRA~1\MICROS~1\VC98\MFC\Lib

echo Compiling with full logging to build.log...

REM Redirect output to file
cl.exe /nologo /MT /O2 /W3 /GX /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "WINVER=0x0410" /I "src\browser" /I "src\browser\core" /I "libs\libjpeg\include" /I "res" /Fe"RetroBrowser.exe" src\browser\core\stdafx.cpp src\browser\core\main.cpp src\browser\ui\ui.cpp src\browser\renderer\renderer.cpp src\browser\parser\parser.cpp src\browser\network\network.cpp res\app.rc /link kernel32.lib user32.lib gdi32.lib comctl32.lib ws2_32.lib advapi32.lib comdlg32.lib shell32.lib libs\libjpeg\lib\jpeg.lib > build.log 2>&1

IF ERRORLEVEL 1 GOTO BUILD_FAILED
GOTO BUILD_SUCCESS

:BUILD_FAILED
echo.
echo BUILD FAILED - Check build.log for all errors
echo.
type build.log
GOTO END

:BUILD_SUCCESS
echo.
echo BUILD SUCCESS!
GOTO END

:END
pause
