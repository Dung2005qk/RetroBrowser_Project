@echo off
REM Run RetroBrowser with debug output capture
REM Requires DebugView (dbgview.exe) from Sysinternals to be in PATH

echo.
echo ========================================
echo RUNNING RETROBROWSER WITH DEBUG OUTPUT
echo ========================================
echo.
echo Open DebugView (dbgview.exe) from Sysinternals to see debug output
echo URL to load: http://localhost:9999/minimal_color_test.html
echo.

REM Create a simple Python script to monitor and display debug output
python - %* << 'PYEOF'
import sys
import os
import subprocess
import time

exe_path = r"deploy\RetroBrowser.exe"

if not os.path.exists(exe_path):
    print("ERROR: RetroBrowser.exe not found!")
    sys.exit(1)

print(f"Launching: {exe_path}")
print("\nIMPORTANT: You must capture debug output yourself:")
print("Option 1 (Best): Open Sysinternals DebugView before clicking Go in browser")
print("Option 2: Use Visual Studio debugger - Debug -> Break All (Ctrl+Alt+Break)")
print("Option 3: Check Event Viewer for Debug output")
print("\nAfter you load the test URL, check what blocks have colors:")
print("  - If all blocks have -1 values: colors not extracted")
print("  - If you see RGB(...) values: colors extracted but not rendered")

proc = subprocess.Popen(exe_path)
print(f"\nRetroBrowser PID: {proc.pid}")
print("Waiting for exit...")
proc.wait()
PYEOF

echo.
echo Done.
