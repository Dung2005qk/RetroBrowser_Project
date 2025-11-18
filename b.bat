@echo off
set PATH=C:\PROGRA~1\MICROS~1\VC98\Bin;C:\PROGRA~1\MICROS~1\Common\MSDev98\Bin;%PATH%
set INCLUDE=C:\PROGRA~1\MICROS~1\VC98\Include
set LIB=C:\PROGRA~1\MICROS~1\VC98\Lib
cl /nologo /MT /O2 /W3 /D WIN32 /I src\browser\core /I src\browser src\browser\core\main.cpp /link /OUT:RetroBrowser.exe
pause
