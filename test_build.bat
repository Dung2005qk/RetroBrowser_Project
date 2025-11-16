@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86

echo Testing environment variables...
echo VCINSTALLDIR=%VCINSTALLDIR%
echo VSCMD_ARG_TGT_ARCH=%VSCMD_ARG_TGT_ARCH%
echo Platform=%Platform%

if "%VCINSTALLDIR%"=="" (
    echo ERROR: VCINSTALLDIR not set
) else (
    echo OK: VCINSTALLDIR is set
)

set ARCH_OK=0
if "%VSCMD_ARG_TGT_ARCH%"=="x86" set ARCH_OK=1
if "%Platform%"=="x86" set ARCH_OK=1
echo ARCH_OK=%ARCH_OK%

if "%ARCH_OK%"=="0" (
    echo ERROR: Not x86 architecture
) else (
    echo OK: x86 architecture detected
)

pause
