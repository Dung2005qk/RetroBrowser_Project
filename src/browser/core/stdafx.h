#pragma once
// ============================================================================
// stdafx.h - Precompiled Header for Win98 Retro Browser Project
// ============================================================================
// PURPOSE: Central optimization hub for Visual C++ 6.0 compilation on Win98.
//          This file serves three critical roles:
//          1. COMPILATION SPEED: Pre-compiles stable system headers into .pch,
//             reducing rebuild times from minutes to seconds on legacy VMs.
//          2. BACKWARD COMPATIBILITY: Locks API version to Win98-era functions,
//             preventing accidental use of newer APIs that cause runtime errors.
//          3. DEPENDENCY CENTRALIZATION: Consolidates common includes/macros,
//             keeping module code clean and maintainable.
//
// GOLDEN RULE: Only include STABLE headers (system APIs, standard library).
//              NEVER include project headers (ui.h, network.h, parser.h, etc.)
//              as this would destroy PCH benefits and force full rebuilds.
//
// USAGE: stdafx.cpp includes this to generate .pch file.
//        All other .cpp files must #include "stdafx.h" as FIRST line.
//        Changes here trigger full project rebuild - edit with caution.
// ============================================================================


// ============================================================================
// SECTION 1: PLATFORM VERSION LOCKING (CRITICAL - MUST PRECEDE ALL INCLUDES)
// ============================================================================
// Lock API surface to Windows 98 era to prevent linking against functions
// unavailable on target OS. These defines MUST appear before any Windows
// headers to correctly filter API declarations.

#define WINVER          0x0410  // Windows 98 (0x0410 = 4.10 = Win98)
#define _WIN32_WINNT    0x0410  // NT-level APIs compatible with Win98
#define _WIN32_WINDOWS  0x0410  // Explicit Win9x family target
#define _WIN32_IE       0x0500  // IE 5.0 common controls (shipped with Win98 SE)

// RATIONALE: Without these locks, windows.h exposes XP+ functions (e.g.,
//            GetProcessDEPPolicy) that compile fine but crash at runtime on
//            Win98 with "procedure not found" errors. Defense in depth.


// ============================================================================
// SECTION 2: HEADER OPTIMIZATION AND TYPE SAFETY
// ============================================================================
// Minimize PCH size and enforce strict type checking for robust code.

#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used services from windows.h:
                                 // - Cryptography (wincrypt.h)
                                 // - DDE (ddeml.h)
                                 // - RPC (rpc.h)
                                 // - Shell extensions (shellapi.h partially)
                                 // BENEFIT: Reduces windows.h parse time by ~40%,
                                 // shrinks PCH from ~15MB to ~9MB on VC++ 6.0.

#define VC_EXTRALEAN            // Further optimization for MFC-free projects.
                                 // Strips additional OLE/COM bloat not needed for
                                 // pure Win32 development. Complements LEAN_AND_MEAN.

#define STRICT                  // Enable strict type checking for handles.
                                 // Forces distinct types for HWND, HDC, HBITMAP, etc.
                                 // Catches errors like passing HWND to HDC parameter.
                                 // MANDATORY for maintainable Win32 code.

#define _CRT_SECURE_NO_WARNINGS // Suppress C4996 warnings for legacy CRT functions
                                 // (strcpy, sprintf, fopen) used in VC++ 6.0 codebase.
                                 // Modern replacements (_s functions) unavailable.
                                 // ACCEPTABLE: Controlled legacy environment, manual
                                 // buffer validation in code reviews.


// ============================================================================
// SECTION 3: NETWORKING HEADERS (MUST PRECEDE <windows.h>)
// ============================================================================
// *** CRITICAL ORDER WARNING ***
// Winsock2 headers MUST be included BEFORE <windows.h> to avoid catastrophic
// symbol conflicts. windows.h implicitly includes <winsock.h> (v1.1) which
// redefines fd_set, timeval, select() incompatibly with Winsock2 (v2.2).
// Symptoms of wrong order: C2011 "type redefinition" errors, linker chaos.

#include <winsock2.h>           // Winsock 2.2 API (WSAStartup, socket, send/recv)
#include <ws2tcpip.h>           // TCP/IP extensions (getaddrinfo, IPv6 stubs)
                                 // NOTE: Full IPv6 unsupported on Win98, but header
                                 // safe to include for forward compatibility.


// ============================================================================
// SECTION 4: CORE WIN32 API HEADERS
// ============================================================================
// Foundation headers for window management, messaging, and GDI rendering.
// Now safe to include after Winsock conflict resolution above.

#include <windows.h>            // Master header: WinMain, HWND, MSG, CreateWindow,
                                 // GetMessage, DispatchMessage, basic GDI (TextOut),
                                 // memory (GlobalAlloc), strings (lstrlen), files.
                                 // LEAN_AND_MEAN keeps this manageable (~8000 lines).

#include <windowsx.h>           // Message cracker macros (GET_X_LPARAM, FORWARD_WM_*).
                                 // Simplifies WndProc parameter extraction, reduces
                                 // error-prone casts. Optional but improves readability.


// ============================================================================
// SECTION 5: UI CONTROLS AND SHELL INTEGRATION
// ============================================================================
// Extended controls and system integration for browser UI elements.

#include <commctrl.h>           // Common controls: Edit (address bar), Button (Go),
                                 // StatusBar (status text), Toolbar (future nav buttons).
                                 // Requires comctl32.lib, IE5 version (_WIN32_IE 0x0500).

#include <shellapi.h>           // Shell operations: ShellExecute (open external URLs),
                                 // ExtractIcon (load system icons for UI chrome).
                                 // Lightweight addition despite LEAN_AND_MEAN.


// ============================================================================
// SECTION 6: CHARACTER ENCODING SUPPORT
// ============================================================================
// Flexible text handling for Win98 ANSI environment with Unicode future-proofing.

#include <tchar.h>              // Generic text mappings: TCHAR, _T(), _tcslen().
                                 // Win98 defaults to ANSI (char, "string"), but macros
                                 // allow easy Unicode port if needed (WCHAR, L"string").
                                 // PROJECT POLICY: Use TCHAR in UI strings, char* for
                                 // HTML parsing (UTF-8 byte streams).


// ============================================================================
// SECTION 7: STANDARD C/C++ LIBRARY HEADERS
// ============================================================================
// Selected STL components and C runtime for data structures, algorithms, I/O.
// CONSTRAINT: VC++ 6.0 has partial STL compliance (no std::wstring, no C++11).
// Only include headers used across MULTIPLE modules to justify PCH overhead.

//--- C++ Standard Library (STL) ---
#include <string>               // std::string for HTML content, URL buffers, parsing.
                                 // Core data structure; universally used.

#include <vector>               // std::vector<T> for dynamic arrays:
                                 // - Parser: list of HTML blocks/tokens
                                 // - Renderer: display list of drawable elements
                                 // Replaces manual malloc/realloc, safer memory mgmt.

#include <map>                  // std::map<K,V> for key-value stores:
                                 // - Parser: tag attributes (e.g., <a href="...">)
                                 // - Network: HTTP header storage
                                 // Efficient O(log n) lookups, stable in VC++ 6.0.

#include <algorithm>            // STL algorithms: std::sort, std::find, std::transform.
                                 // Used in parser (tag sorting) and renderer (z-order).

#include <iostream>             // std::cout, std::cerr for console debug output.
                                 // NOTE: Win98 GUI app has no console by default; use
                                 // AllocConsole() for debug builds or redirect to file.

#include <sstream>              // std::stringstream for string formatting:
                                 // - Build HTTP requests (headers concatenation)
                                 // - Format log messages (int-to-string conversions)
                                 // Safer than sprintf with dynamic allocation.

//--- C Standard Library ---
#include <stdio.h>              // Legacy C I/O: fopen, fwrite (for saving cache/logs).
#include <stdlib.h>             // Standard utilities: malloc, free, atoi, rand.
#include <string.h>             // C string ops: strcpy, strcmp, memcpy, memset.
                                 // Still faster than std::string for fixed buffers.
#include <stdarg.h>             // Variable argument lists: va_list, va_start, va_end
#include <malloc.h>             // Memory allocation: _alloca (stack allocation for
                                 // temp buffers), _msize (query allocation size).
#include <memory.h>             // Low-level memory: memcmp, memmove. Redundant with
                                 // string.h on Win32 but explicit for clarity.

// RATIONALE FOR SELECTION: These are the "greatest hits" used in 80%+ of
// project files. Omitted: <fstream> (use Win32 file APIs), <list> (vector
// sufficient), <set> (map adequate), modern headers (unavailable in VC++ 6.0).


// ============================================================================
// SECTION 8: AUTOMATIC LIBRARY LINKING
// ============================================================================
// Embed linker directives to auto-link required system libraries, eliminating
// manual project configuration. Self-documenting dependencies.

#pragma comment(lib, "ws2_32.lib")      // Winsock 2.2 (network module)
#pragma comment(lib, "comctl32.lib")    // Common controls (UI module)
#pragma comment(lib, "gdi32.lib")       // GDI rendering (renderer module)
#pragma comment(lib, "user32.lib")      // User interface base (all modules)
#pragma comment(lib, "kernel32.lib")    // Core OS services (memory, files)
#pragma comment(lib, "shell32.lib")     // Shell integration (external links)

// BENEFIT: Source-level dependency declaration. Portable across VC++ projects,
// reduces errors from missing .lib in project settings. MinGW ignores pragmas
// gracefully, requiring manual -l flags instead.


// ============================================================================
// SECTION 9: GLOBAL UTILITY MACROS AND CONSTANTS
// ============================================================================
// Project-wide helper macros for safety, debugging, and common operations.

//--- Memory Safety ---
#define SAFE_DELETE(p)      do { if(p) { delete (p); (p) = NULL; } } while(0)
#define SAFE_DELETE_ARRAY(p) do { if(p) { delete[] (p); (p) = NULL; } } while(0)
#define SAFE_RELEASE(p)     do { if(p) { (p)->Release(); (p) = NULL; } } while(0)
// RATIONALE: Idiomatic delete wrappers prevent double-free bugs and dangling
// pointers. do-while(0) ensures safe usage in all contexts (if/else blocks).
// SAFE_RELEASE for future COM integration (e.g., loading images via IStream).

//--- Debug Logging ---
#ifdef _DEBUG
    #define DEBUG_LOG(msg)      std::cout << "[DEBUG] " << msg << std::endl
    // VC++ 6.0 doesn't support variadic macros well, use inline function instead
    inline void DEBUG_LOGF(const char* fmt, ...) {
        printf("[DEBUG] ");
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
        printf("\n");
    }
#else
    #define DEBUG_LOG(msg)      ((void)0)
    inline void DEBUG_LOGF(const char*, ...) {}
#endif
// RATIONALE: Zero-cost logging in release builds (optimized away). Console
// output requires AllocConsole() in WinMain for _DEBUG builds. Alternative:
// OutputDebugString() for debugger output without console window.

//--- Network Constants ---
#define DEFAULT_BUFFER_SIZE     4096    // Standard recv/send buffer (1 TCP packet)
#define MAX_URL_LENGTH          2048    // IE5 limit; sufficient for typical URLs
#define HTTP_TIMEOUT_MS         30000   // 30 seconds for socket operations

//--- Application Metadata ---
#define APP_NAME                "Win98 Retro Browser"
#define APP_VERSION             "1.0"
#define USER_AGENT              "Mozilla/4.0 (compatible; MSIE 5.0; Windows 98)"
// RATIONALE: IE5 user-agent ensures maximum compatibility with retro websites
// and proxy server expectations. Modern sites may reject but that's out-of-scope.

#define PROXY_DEFAULT_HOST      "192.168.56.1"
#define PROXY_DEFAULT_PORT      8080
// NOTE: Proxy runs on host machine, VM connects via host-only network. Adjust
// IP in config if using bridged/NAT networking instead.


// ============================================================================
// SECTION 10: PROJECT RESOURCE DEFINITIONS
// ============================================================================
// Include resource IDs (icons, menus, dialogs) for UI integration. Resource
// files are stable (change infrequently), justifying PCH inclusion.

#include "../res/resource.h"    // Relative path from src/browser/core/stdafx.h
                                 // Contains: IDI_APP_ICON, IDM_FILE_EXIT, IDD_ABOUT, etc.
                                 // IMPORTANT: If resource.h is frequently modified (e.g.,
                                 // adding dialog controls during dev), consider moving
                                 // this include OUT of stdafx.h to avoid full rebuilds.
                                 // For mature projects, keep here for convenience.


// ============================================================================
// END OF PRECOMPILED HEADER
// ============================================================================
// USAGE REMINDER:
// - stdafx.cpp: #include "stdafx.h" only, compile with /Yc"stdafx.h" to CREATE .pch
// - All other .cpp: #include "stdafx.h" FIRST, compile with /Yu"stdafx.h" to USE .pch
// - Modify with extreme caution: every change rebuilds entire project (2-5 min on VM)
//
// FORBIDDEN INCLUDES (would destroy PCH benefits):
// - ui/ui.h, network/network.h, parser/parser.h, renderer/renderer.h
// - Any project header that changes frequently during development
//
// MAINTENANCE: Review quarterly to remove unused includes, keeping PCH lean.
// ============================================================================