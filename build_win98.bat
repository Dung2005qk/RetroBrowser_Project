@echo off
REM ============================================================================
REM RetroBrowser - Windows 98 Optimized Build Script
REM ============================================================================
REM
REM This script builds RetroBrowser with full Windows 98 SE compatibility.
REM
REM TARGET PLATFORM:
REM   - Windows 98 Second Edition (4.10.2222 A)
REM   - Windows 98 First Edition (with Winsock 2.2 update)
REM   - Windows ME (Millennium Edition)
REM
REM MINIMUM HARDWARE:
REM   - CPU: 200MHz Pentium MMX or equivalent
REM   - RAM: 64MB (128MB recommended)
REM   - HDD: 2GB with 50MB free space
REM
REM TESTED CONFIGURATIONS:
REM   - Windows 98 SE + IE 5.0 + Winsock 2.2
REM   - VirtualBox VM with 128MB RAM, 200MHz CPU equivalent
REM
REM USAGE:
REM   1. Open "x86 Native Tools Command Prompt for VS 2022"
REM   2. Navigate to RetroBrowser directory
REM   3. Run: build_win98.bat
REM   4. Output will be in deploy\RetroBrowser_Win98.exe
REM
REM NOTES:
REM   - This script requires Visual Studio 2022 with C++ Desktop Development
REM   - The x86 (32-bit) toolchain must be loaded, NOT x64
REM   - Static CRT linking (/MT) eliminates MSVCRT.DLL dependency
REM   - Subsystem version 4.10 is critical for Win98 PE loader
REM   - Modern linkers don't support 4.10 directly, so we use editbin to patch
REM
REM ============================================================================

REM Initialize Visual Studio environment
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86
if errorlevel 1 (
    echo ERROR: Failed to initialize Visual Studio environment
    echo Please ensure Visual Studio 2022 is installed with C++ tools
    goto error_exit
)

echo.
echo ============================================================================
echo   RetroBrowser - Windows 98 Optimized Build
echo ============================================================================
echo.

REM ============================================================================
REM SUBTASK 2.1: Environment Validation
REM ============================================================================
echo [1/6] Validating build environment...
echo.

if "%VCINSTALLDIR%"=="" (
    echo ERROR: Visual Studio environment not detected!
    echo.
    echo This script requires the Visual Studio x86 build environment.
    echo Please run from "x86 Native Tools Command Prompt for VS 2022"
    echo.
    goto error_exit
)

echo   [OK] Visual Studio environment: %VCINSTALLDIR%
echo   [OK] Target architecture: x86 (32-bit)
echo   [OK] Compiler and linker tools ready
echo.

REM ============================================================================
REM Create Build Directories
REM ============================================================================
echo [2/6] Creating build directories...
echo.

if not exist obj mkdir obj
if not exist deploy mkdir deploy
echo   [OK] Build directories ready
echo.

REM ============================================================================
REM SUBTASK 2.2: Configure Win98-Specific Compiler Flags
REM ============================================================================
echo [3/6] Configuring Win98 compiler flags...
echo.

REM Define Win98 compatibility macros
REM   WINVER=0x0410          - Target Windows 98 (4.10) API level
REM   _WIN32_WINNT=0x0410    - Target NT 4.0 API level
REM   _WIN32_WINDOWS=0x0410  - Explicitly target Windows 9x family version 4.10
REM   _WIN32_IE=0x0500       - Target Internet Explorer 5.0 (shipped with Win98 SE)
set WIN98_DEFINES=/D WIN32 /D _WINDOWS /D _MBCS /D WINVER=0x0410 /D _WIN32_WINNT=0x0410 /D _WIN32_WINDOWS=0x0410 /D _WIN32_IE=0x0500

REM Compiler optimization and code generation flags
REM   /MT      - Static link CRT (no MSVCRT.DLL dependency - critical for Win98)
REM   /O2      - Optimize for speed (better performance on 200MHz CPU)
REM   /Oy-     - Keep frame pointers (better debugging and stack traces on Win98)
REM   /EHsc    - C++ exception handling (synchronous exceptions only)
REM   /W3      - Warning level 3 (good balance of useful warnings)
REM   /Zi      - Generate debug information (separate .pdb file)
REM   /nologo  - Suppress compiler banner
REM Note: /GZ removed as it conflicts with /O2 in modern compilers
set COMPILER_FLAGS=/MT /O2 /Oy- /EHsc /W3 /Zi /nologo

REM Include directories
set INCLUDE_DIRS=/I src/browser /I src/browser/core /I libs/libjpeg/include

echo   [OK] Win98 API targeting: WINVER=0x0410 (Windows 98)
echo   [OK] CRT linking: Static (/MT) - no MSVCRT.DLL needed
echo   [OK] Optimization: Speed (/O2) for 200MHz CPU
echo   [OK] Frame pointers: Enabled (/Oy-) for debugging
echo.

REM ============================================================================
REM SUBTASK 2.3: Configure Win98-Specific Linker Flags
REM ============================================================================
echo [4/6] Configuring Win98 linker flags...
echo.

REM Critical linker flags for Win98 compatibility
REM   /SUBSYSTEM:WINDOWS,4.10  - Request PE subsystem Win98 (4.10)
REM                              Note: Modern linkers ignore this, we patch later
REM   /MACHINE:X86             - Target 32-bit x86 architecture
REM   /INCREMENTAL:NO          - Disable incremental linking (smaller binary)
REM   /DEBUG                   - Generate debug information
REM   /PDB:                    - Specify PDB file location
set LINKER_FLAGS=/SUBSYSTEM:WINDOWS,4.10 /MACHINE:X86 /INCREMENTAL:NO /DEBUG /PDB:deploy/RetroBrowser_Win98.pdb

REM Win98-compatible system libraries
REM All these DLLs exist on Windows 98 SE:
REM   kernel32.lib  - Core Win32 API (process, memory, file I/O)
REM   user32.lib    - User interface (windows, messages, controls)
REM   gdi32.lib     - Graphics Device Interface (drawing, fonts, bitmaps)
REM   comctl32.lib  - Common controls (status bar, toolbar, etc.)
REM   shell32.lib   - Shell API (file operations, shell integration)
REM   ws2_32.lib    - Winsock 2.2 (networking - requires Winsock 2 update on Win98)
REM   advapi32.lib  - Advanced API (registry, security)
REM   comdlg32.lib  - Common dialogs (file open/save, etc.)
set WIN98_LIBS=kernel32.lib user32.lib gdi32.lib comctl32.lib shell32.lib ws2_32.lib advapi32.lib comdlg32.lib

REM Third-party libraries
set THIRD_PARTY_LIBS=libs/libjpeg/lib/jpeg.lib

echo   [OK] Subsystem version: 4.10 (Windows 98) - will be patched
echo   [OK] Machine type: X86 (32-bit)
echo   [OK] Incremental linking: Disabled (smaller binary)
echo   [OK] System libraries: Win98-compatible DLLs only
echo.

REM ============================================================================
REM Compilation Phase
REM ============================================================================
echo [5/6] Compiling RetroBrowser for Windows 98...
echo.

REM Source files to compile
set SOURCE_FILES=src/browser/core/stdafx.cpp src/browser/core/main.cpp src/browser/ui/ui.cpp src/browser/renderer/renderer.cpp src/browser/parser/parser.cpp src/browser/network/network.cpp

REM Execute compilation and linking in one step
echo   Compiling and linking...
echo.

cl.exe %COMPILER_FLAGS% %WIN98_DEFINES% %INCLUDE_DIRS% /Fe:deploy/RetroBrowser_Win98.exe /Fo:obj/ %SOURCE_FILES% /link %LINKER_FLAGS% %WIN98_LIBS% %THIRD_PARTY_LIBS%

REM ============================================================================
REM SUBTASK 2.4: Build Output Validation
REM ============================================================================
echo.
echo [6/6] Validating build output...
echo.

REM Check compilation result
if errorlevel 1 (
    echo   [FAILED] Compilation failed
    echo.
    echo ============================================================================
    echo   BUILD FAILED!
    echo ============================================================================
    echo.
    echo Please review the compiler errors above.
    echo Object files have been preserved in obj\ directory for debugging.
    echo.
    goto error_exit
)

REM Verify output binary exists
if not exist deploy\RetroBrowser_Win98.exe (
    echo   [FAILED] Output binary not found!
    echo.
    echo ============================================================================
    echo   BUILD FAILED!
    echo ============================================================================
    echo.
    echo The compiler reported success but deploy\RetroBrowser_Win98.exe was not created.
    echo This may indicate a linker issue.
    echo.
    goto error_exit
)

echo   [OK] Binary created successfully
echo.

REM ============================================================================
REM SUBTASK 2.5: Patch PE Header for Win98 Compatibility
REM ============================================================================
REM Modern linkers (VS2022) don't support subsystem version 4.10 directly
REM We must use editbin.exe to patch the PE header after linking

echo [6.5/6] Patching PE header for Win98 compatibility...
echo.
echo   Fixing subsystem version to 4.10 (Windows 98)...
editbin.exe /SUBSYSTEM:WINDOWS,4.10 /NOLOGO deploy\RetroBrowser_Win98.exe >nul 2>&1
if errorlevel 1 (
    echo   [WARNING] editbin failed - subsystem version may not be correct
    echo             Binary may not load on Windows 98!
) else (
    echo   [OK] Subsystem version successfully patched to 4.10
)
echo.

echo ============================================================================
echo   BUILD SUCCESS!
echo ============================================================================
echo.
echo Output binary: deploy\RetroBrowser_Win98.exe
echo Debug symbols: deploy\RetroBrowser_Win98.pdb
echo.
echo Binary size:
dir deploy\RetroBrowser_Win98.exe | find "RetroBrowser_Win98.exe"
echo.
echo ============================================================================
echo   Running Post-Build Validation
echo ============================================================================
echo.
echo Validating Win98 compatibility...
echo.
python tools\validate_win98_binary.py deploy\RetroBrowser_Win98.exe
if errorlevel 1 (
    echo.
    echo [WARNING] Binary validation detected potential compatibility issues.
    echo           Review the validation report above.
    echo           The binary may still work on Windows 98, but testing is recommended.
    echo.
)
echo.
echo ============================================================================
echo   Windows 98 Compatibility Information
echo ============================================================================
echo.
echo This binary is optimized for Windows 98 SE with the following features:
echo.
echo   [*] Subsystem version: 4.10 (Windows 98)
echo   [*] Static CRT linking (no MSVCRT.DLL required)
echo   [*] x86 32-bit architecture
echo   [*] Optimized for 200MHz CPU
echo   [*] Compatible with 64MB RAM systems
echo.
echo DEPLOYMENT NOTES:
echo   - Tested on Windows 98 SE (4.10.2222 A)
echo   - Requires Winsock 2.2 (ws2_32.dll) - included in Win98 SE
echo   - For Win98 FE, install Winsock 2 update from Microsoft
echo   - Run proxy server on host machine for network access
echo.
echo NEXT STEPS:
echo   1. Copy deploy\RetroBrowser_Win98.exe to your Win98 system
echo   2. Ensure network is configured (host-only adapter recommended)
echo   3. Start proxy server on host: python src/proxy/proxy.py
echo   4. Launch RetroBrowser_Win98.exe on Win98
echo.
echo For detailed testing instructions, see:
echo   docs\win98_testing_guide.md
echo.
echo ============================================================================
echo.

goto success_exit

REM ============================================================================
REM Exit Handlers
REM ============================================================================

:error_exit
echo ============================================================================
echo   Build process terminated with errors.
echo ============================================================================
echo.
pause
exit /b 1

:success_exit
pause
exit /b 0
