@echo off
REM --- Thiết lập môi trường cho Visual C++ 6.0 ---
REM Lưu ý: Đường dẫn phải chính xác theo cài đặt VC++6.0 trên máy Win98
set PATH=C:\PROGRA~1\MICROS~1\VC98\Bin;C:\PROGRA~1\MICROS~1\Common\MSDev98\Bin;C:\WINDOWS;C:\WINDOWS\COMMAND
set INCLUDE=C:\PROGRA~1\MICROS~1\VC98\Include;C:\PROGRA~1\MICROS~1\VC98\ATL\Include;C:\PROGRA~1\MICROS~1\VC98\MFC\Include
set LIB=C:\PROGRA~1\MICROS~1\VC98\Lib;C:\PROGRA~1\MICROS~1\VC98\MFC\Lib

REM --- Debug: Show environment ---
echo Checking VC++ 6.0 environment...
echo LIB=%LIB%
echo.
if not exist "C:\PROGRA~1\MICROS~1\VC98\Lib\kernel32.lib" (
    echo ERROR: kernel32.lib not found!
    echo Please check VC++ 6.0 installation path.
    pause
    exit /b 1
)

REM Tạo thư mục obj để chứa các file tạm
if not exist obj mkdir obj

echo Compiling RetroBrowser for Win98...

REM --- Step 1: Compile resource file ---
echo [1/3] Compiling resources...
rc.exe /fo"obj\app.res" src\browser\res\app.rc
IF ERRORLEVEL 1 GOTO BUILD_FAILED

REM --- Step 2: Compile C++ source files into .obj files ---
echo [2/3] Compiling C++ source files...
cl.exe /c /nologo /MT /O2 /W3 /GX /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "WINVER=0x0410" /I "src\browser" /I "src\browser\core" /I "src\browser\res" /I "libs\libjpeg\include" /Fo"obj\\" src\browser\core\stdafx.cpp src\browser\core\main.cpp src\browser\ui\ui.cpp src\browser\renderer\renderer.cpp src\browser\parser\parser.cpp src\browser\network\network.cpp
IF ERRORLEVEL 1 GOTO BUILD_FAILED

REM --- Step 3: Link all .obj and .res files into the final .exe ---
echo [3/3] Linking...
REM Use explicit library paths to avoid LIB path issues on Win98
link.exe /nologo /OUT:"RetroBrowser.exe" /SUBSYSTEM:WINDOWS /LIBPATH:"C:\PROGRA~1\MICROS~1\VC98\Lib" obj\*.obj obj\app.res kernel32.lib user32.lib gdi32.lib comctl32.lib ws2_32.lib advapi32.lib comdlg32.lib shell32.lib libs\libjpeg\lib\jpeg.lib
IF ERRORLEVEL 1 GOTO BUILD_FAILED

GOTO BUILD_SUCCESS

:BUILD_FAILED
echo.
echo BUILD FAILED. Check errors above.
GOTO END

:BUILD_SUCCESS
echo.
echo BUILD SUCCESS!
GOTO END

:END
pause