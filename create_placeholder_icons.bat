@echo off
echo Creating placeholder icon files...

REM Tạo thư mục nếu chưa có
if not exist src\browser\res mkdir src\browser\res

REM Tạo file ICO đơn giản nhất (16x16, 2 colors)
REM Header: 6 bytes + Entry: 16 bytes + Bitmap: 40 bytes header + 8 bytes palette + 64 bytes data
echo Creating app.ico...
(
echo 00 00 01 00 01 00 10 10 02 00 01 00 01 00 30 00
echo 00 00 16 00 00 00 28 00 00 00 10 00 00 00 20 00
echo 00 00 01 00 01 00 00 00 00 00 00 00 00 00 00 00
echo 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
echo 00 00 FF FF FF 00 FF FF FF FF FF FF FF FF FF FF
echo FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
echo FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
echo FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
echo FF FF 00 00 00 00 00 00 00 00 00 00 00 00 00 00
echo 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
echo 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
echo 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
) > temp_icon.txt

REM Convert hex to binary (cần debug.exe)
debug < temp_icon.txt > nul

REM Cách đơn giản hơn: Copy icon từ Windows
echo Copying default Windows icon...
copy /Y C:\Windows\notepad.exe src\browser\res\app.ico > nul 2>&1
if errorlevel 1 (
    echo Failed to copy icon, creating empty file...
    echo. > src\browser\res\app.ico
)

echo Creating hand.cur...
copy /Y C:\Windows\System\shell32.dll src\browser\res\hand.cur > nul 2>&1
if errorlevel 1 (
    echo Failed to copy cursor, creating empty file...
    echo. > src\browser\res\hand.cur
)

echo.
echo Files created:
dir src\browser\res\*.ico
dir src\browser\res\*.cur

echo.
echo NOTE: These are placeholder files.
echo For proper icons, use Paint or icon editor.
echo.
pause
