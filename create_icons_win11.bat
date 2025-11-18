@echo off
echo ========================================
echo Creating Icon and Cursor Files
echo ========================================
echo.

REM Tạo thư mục res nếu chưa có
if not exist "src\browser\res" (
    echo Creating directory: src\browser\res
    mkdir "src\browser\res"
)

REM Tạo app.ico từ notepad.exe
echo Creating app.ico from notepad.exe...
copy /Y "C:\Windows\System32\notepad.exe" "src\browser\res\app.ico" > nul
if errorlevel 1 (
    echo ERROR: Failed to copy notepad.exe
    echo Trying alternative source...
    copy /Y "C:\Windows\System32\calc.exe" "src\browser\res\app.ico" > nul
)

REM Tạo hand.cur từ Windows cursors
echo Creating hand.cur from Windows cursors...
copy /Y "C:\Windows\Cursors\aero_link.cur" "src\browser\res\hand.cur" > nul
if errorlevel 1 (
    echo ERROR: Failed to copy aero_link.cur
    echo Trying alternative cursor...
    copy /Y "C:\Windows\Cursors\hand.cur" "src\browser\res\hand.cur" > nul
    if errorlevel 1 (
        copy /Y "C:\Windows\Cursors\arrow.cur" "src\browser\res\hand.cur" > nul
    )
)

echo.
echo ========================================
echo Files Created:
echo ========================================
dir "src\browser\res\app.ico" 2>nul
dir "src\browser\res\hand.cur" 2>nul

echo.
echo ========================================
echo SUCCESS!
echo ========================================
echo Icon files have been created in:
echo   src\browser\res\app.ico
echo   src\browser\res\hand.cur
echo.
echo Now you can:
echo 1. Copy the entire src folder to Win98 VM
echo 2. Build the project in Visual Studio on Win98
echo.
pause
