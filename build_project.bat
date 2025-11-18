@echo off
echo Building with Visual Studio project...
"C:\Program Files\Microsoft Visual Studio\Common\MSDev98\Bin\msdev.exe" browser.dsp /MAKE "browser - Win32 Release" /REBUILD
pause
