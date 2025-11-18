@echo off
echo ========================================
echo Building RetroBrowser for Win98
echo ========================================

REM Setup Visual C++ 6.0 environment
set PATH=C:\PROGRA~1\MICROS~1\VC98\Bin;C:\PROGRA~1\MICROS~1\Common\MSDev98\Bin;%PATH%
set INCLUDE=C:\PROGRA~1\MICROS~1\VC98\Include
set LIB=C:\PROGRA~1\MICROS~1\VC98\Lib

REM Create temp directory for object files
if not exist temp mkdir temp

echo.
echo Step 1: Compiling stdafx.cpp (precompiled header)...
cl /c /nologo /MT /O2 /W3 /D WIN32 /D NDEBUG /D _WINDOWS ^
   /I src\browser\core /I src\browser ^
   /Yc"stdafx.h" /Fp"temp\stdafx.pch" ^
   /Fo"temp\stdafx.obj" ^
   src\browser\core\stdafx.cpp

if errorlevel 1 goto error

echo.
echo Step 2: Compiling main.cpp...
cl /c /nologo /MT /O2 /W3 /D WIN32 /D NDEBUG /D _WINDOWS ^
   /I src\browser\core /I src\browser ^
   /Yu"stdafx.h" /Fp"temp\stdafx.pch" ^
   /Fo"temp\main.obj" ^
   src\browser\core\main.cpp

if errorlevel 1 goto error

echo.
echo Step 3: Compiling ui.cpp...
cl /c /nologo /MT /O2 /W3 /D WIN32 /D NDEBUG /D _WINDOWS ^
   /I src\browser\core /I src\browser ^
   /Yu"stdafx.h" /Fp"temp\stdafx.pch" ^
   /Fo"temp\ui.obj" ^
   src\browser\ui\ui.cpp

if errorlevel 1 goto error

echo.
echo Step 4: Compiling network.cpp...
cl /c /nologo /MT /O2 /W3 /D WIN32 /D NDEBUG /D _WINDOWS ^
   /I src\browser\core /I src\browser ^
   /Yu"stdafx.h" /Fp"temp\stdafx.pch" ^
   /Fo"temp\network.obj" ^
   src\browser\network\network.cpp

if errorlevel 1 goto error

echo.
echo Step 5: Compiling parser.cpp...
cl /c /nologo /MT /O2 /W3 /D WIN32 /D NDEBUG /D _WINDOWS ^
   /I src\browser\core /I src\browser ^
   /Yu"stdafx.h" /Fp"temp\stdafx.pch" ^
   /Fo"temp\parser.obj" ^
   src\browser\parser\parser.cpp

if errorlevel 1 goto error

echo.
echo Step 6: Compiling renderer.cpp...
cl /c /nologo /MT /O2 /W3 /D WIN32 /D NDEBUG /D _WINDOWS ^
   /I src\browser\core /I src\browser ^
   /Yu"stdafx.h" /Fp"temp\stdafx.pch" ^
   /Fo"temp\renderer.obj" ^
   src\browser\renderer\renderer.cpp

if errorlevel 1 goto error

echo.
echo Step 7: Compiling resources...
rc /fo temp\resource.res src\browser\res\resource.rc

if errorlevel 1 goto error

echo.
echo Step 8: Linking...
link /nologo /SUBSYSTEM:WINDOWS /OUT:RetroBrowser.exe ^
     temp\stdafx.obj temp\main.obj temp\ui.obj ^
     temp\network.obj temp\parser.obj temp\renderer.obj ^
     temp\resource.res ^
     ws2_32.lib comctl32.lib gdi32.lib user32.lib kernel32.lib shell32.lib

if errorlevel 1 goto error

echo.
echo ========================================
echo BUILD SUCCESS!
echo Output: RetroBrowser.exe
echo ========================================
goto end

:error
echo.
echo ========================================
echo BUILD FAILED!
echo ========================================

:end
pause
