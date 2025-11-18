@echo off
set PATH=C:\PROGRA~1\MICROS~1\VC98\Bin;C:\PROGRA~1\MICROS~1\Common\MSDev98\Bin;%PATH%
set INCLUDE=C:\PROGRA~1\MICROS~1\VC98\Include
set LIB=C:\PROGRA~1\MICROS~1\VC98\Lib
cl /nologo test.cpp /link /SUBSYSTEM:WINDOWS /OUT:test.exe user32.lib
pause
