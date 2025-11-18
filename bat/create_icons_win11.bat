@echo off
setlocal
echo ========================================
echo Creating Compatible Icon and Cursor Files
echo ========================================
echo.

REM --- SỬA LỖI 1: ĐƯỜNG DẪN THƯ MỤC "res" ---
REM Thư mục "res" phải nằm ở thư mục gốc của dự án, không nằm trong "src".
set RES_DIR=res
if not exist "%RES_DIR%" (
    echo Creating directory: %RES_DIR%
    mkdir "%RES_DIR%"
)

REM --- SỬA LỖI 2: NGUỒN TẠO ICON ---
REM Sử dụng shell32.dll thay vì notepad.exe.
REM shell32.dll chứa các icon cũ, đảm bảo tương thích với định dạng 3.00.
echo Creating compatible app.ico from shell32.dll...
copy /Y "%SystemRoot%\System32\shell32.dll" "%RES_DIR%\app.ico" > nul
if errorlevel 1 (
    echo ERROR: Failed to copy from shell32.dll
    echo Trying alternative legacy source (moricons.dll)...
    copy /Y "%SystemRoot%\System32\moricons.dll" "%RES_DIR%\app.ico" > nul
    if errorlevel 1 (
        echo FATAL: Could not create a compatible icon file.
    )
)

REM Giữ nguyên logic tạo hand.cur, nó đã đủ tốt.
echo Creating hand.cur from Windows cursors...
copy /Y "%SystemRoot%\Cursors\aero_link.cur" "%RES_DIR%\hand.cur" > nul
if errorlevel 1 (
    echo INFO: aero_link.cur not found. Trying standard hand.cur...
    copy /Y "%SystemRoot%\Cursors\hand.cur" "%RES_DIR%\hand.cur" > nul
    if errorlevel 1 (
        echo INFO: hand.cur not found. Using basic arrow.cur as fallback...
        copy /Y "%SystemRoot%\Cursors\arrow.cur" "%RES_DIR%\hand.cur" > nul
    )
)

echo.
echo ========================================
echo Files Created in '%RES_DIR%':
echo ========================================
dir "%RES_DIR%\app.ico" /B 2>nul
dir "%RES_DIR%\hand.cur" /B 2>nul

echo.
echo ========================================
echo SUCCESS!
echo ========================================
echo Compatible resource files have been created.
echo Your project structure should now be correct.
echo.
echo Now, proceed to build the project in your Win98 VM.
echo.
pause
endlocal