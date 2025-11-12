#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Debug trace script - monitors HTML parser/renderer execution
Displays what happens during HTML parsing and rendering
"""

import subprocess
import sys
import time
import os

print("=" * 80)
print("DEBUG TRACE - START")
print("=" * 80)
print(f"Time: {time.strftime('%Y-%m-%d %H:%M:%S')}")

# Start RetroBrowser with debugger attachment
# On Windows, we can use debugbreak if built with /Zi flag

exe_path = r"deploy\RetroBrowser.exe"

if not os.path.exists(exe_path):
    print("ERROR: RetroBrowser.exe not found!")
    sys.exit(1)

print(f"\nLaunching: {exe_path}")
print("\nFOLLOW THESE STEPS:")
print("1. Load test file: http://localhost:9999/minimal_color_test.html")
print("2. Check Visual Studio Output window (Debug -> Windows -> Output)")
print("3. Look for debug messages starting with:")
print("   - 'ParseLegacyHtmlColors:'")
print("   - 'SetContent:'")
print("   - 'Block #:' with color values")
print("\nIf you see 'bgcolor=-1' and 'textColor=-1' for all blocks:")
print("  -> Colors NOT extracted by parser")
print("If you see 'RGB(0, 0, 0)' and 'RGB(0, 255, 0)':")
print("  -> Colors extracted correctly by parser")
print("If SetContent shows page colors but browser is still white:")
print("  -> Problem is in GDI rendering code")

# Just run it
try:
    subprocess.Popen(exe_path)
    print(f"\n✓ RetroBrowser started")
    print("Monitor the debugger output window for color extraction traces...")
except Exception as e:
    print(f"✗ Error launching: {e}")
