"""
Win98 API Database
Contains comprehensive lists of Win32 APIs compatible with Windows 98 SE
"""

# Win98-compatible APIs organized by DLL
WIN98_COMPATIBLE_APIS = {
    "kernel32": {
        # File operations
        "CreateFileA", "CreateFileW", "ReadFile", "WriteFile", "CloseHandle",
        "DeleteFileA", "DeleteFileW", "CopyFileA", "CopyFileW", "MoveFileA", "MoveFileW",
        "GetFileSize", "SetFilePointer", "GetFileAttributesA", "GetFileAttributesW",
        "SetFileAttributesA", "SetFileAttributesW", "FindFirstFileA", "FindFirstFileW",
        "FindNextFileA", "FindNextFileW", "FindClose", "GetTempPathA", "GetTempPathW",
        "GetTempFileNameA", "GetTempFileNameW",
        
        # Memory operations
        "VirtualAlloc", "VirtualFree", "VirtualProtect", "VirtualQuery",
        "HeapCreate", "HeapDestroy", "HeapAlloc", "HeapFree", "HeapReAlloc",
        "GlobalAlloc", "GlobalFree", "GlobalLock", "GlobalUnlock", "GlobalReAlloc",
        "LocalAlloc", "LocalFree", "LocalLock", "LocalUnlock",
        
        # Process/Thread operations
        "CreateThread", "ExitThread", "TerminateThread", "SuspendThread", "ResumeThread",
        "GetCurrentThread", "GetCurrentThreadId", "GetCurrentProcess", "GetCurrentProcessId",
        "CreateProcessA", "CreateProcessW", "ExitProcess", "TerminateProcess",
        "WaitForSingleObject", "WaitForMultipleObjects", "Sleep", "GetExitCodeThread",
        "GetExitCodeProcess", "SetThreadPriority", "GetThreadPriority",
        
        # Synchronization
        "CreateMutexA", "CreateMutexW", "ReleaseMutex", "CreateEventA", "CreateEventW",
        "SetEvent", "ResetEvent", "CreateSemaphoreA", "CreateSemaphoreW", "ReleaseSemaphore",
        "InitializeCriticalSection", "DeleteCriticalSection", "EnterCriticalSection",
        "LeaveCriticalSection", "TryEnterCriticalSection",
        
        # System info
        "GetSystemInfo", "GetVersionExA", "GetVersionExW", "GetTickCount",
        "GetSystemTime", "GetLocalTime", "GetSystemDirectory", "GetWindowsDirectory",
        "GetComputerNameA", "GetComputerNameW", "GetEnvironmentVariableA", "GetEnvironmentVariableW",
        
        # Error handling
        "GetLastError", "SetLastError", "FormatMessageA", "FormatMessageW",
        
        # String operations
        "lstrlenA", "lstrlenW", "lstrcpyA", "lstrcpyW", "lstrcatA", "lstrcatW",
        "lstrcmpA", "lstrcmpW", "lstrcmpiA", "lstrcmpiW",
        
        # Module operations
        "LoadLibraryA", "LoadLibraryW", "FreeLibrary", "GetProcAddress",
        "GetModuleHandleA", "GetModuleHandleW", "GetModuleFileNameA", "GetModuleFileNameW",
    },
    
    "user32": {
        # Window management
        "CreateWindowExA", "CreateWindowExW", "CreateWindowEx", "DestroyWindow", "ShowWindow", "UpdateWindow",
        "MoveWindow", "SetWindowPos", "GetWindowRect", "GetClientRect", "AdjustWindowRect",
        "AdjustWindowRectEx", "IsWindow", "IsWindowVisible", "IsWindowEnabled",
        "EnableWindow", "SetFocus", "GetFocus", "SetActiveWindow", "GetActiveWindow",
        "GetForegroundWindow", "SetForegroundWindow", "BringWindowToTop", "GetParent",
        
        # Window properties
        "SetWindowTextA", "SetWindowTextW", "SetWindowText", "GetWindowTextA", "GetWindowTextW", "GetWindowText",
        "GetWindowTextLengthA", "GetWindowTextLengthW", "SetWindowLongA", "SetWindowLongW", "SetWindowLong",
        "GetWindowLongA", "GetWindowLongW", "GetWindowLong", "GetClassNameA", "GetClassNameW",
        
        # Window class registration
        "RegisterClassA", "RegisterClassW", "RegisterClassExA", "RegisterClassExW",
        "UnregisterClassA", "UnregisterClassW", "GetClassInfoA", "GetClassInfoW",
        "GetClassInfoExA", "GetClassInfoExW",
        
        # Message handling
        "GetMessageA", "GetMessageW", "GetMessage", "PeekMessageA", "PeekMessageW", "TranslateMessage",
        "DispatchMessageA", "DispatchMessageW", "PostMessageA", "PostMessageW",
        "SendMessageA", "SendMessageW", "SendMessage", "PostQuitMessage", "DefWindowProcA", "DefWindowProcW",
        "CallWindowProcA", "CallWindowProcW",
        
        # Dialog boxes
        "DialogBoxParamA", "DialogBoxParamW", "CreateDialogParamA", "CreateDialogParamW",
        "EndDialog", "GetDlgItem", "GetDlgItemTextA", "GetDlgItemTextW",
        "SetDlgItemTextA", "SetDlgItemTextW", "MessageBoxA", "MessageBoxW",
        
        # Input
        "GetKeyState", "GetAsyncKeyState", "GetKeyboardState", "SetKeyboardState",
        "GetCursorPos", "SetCursorPos", "SetCursor", "GetCapture", "SetCapture", "ReleaseCapture",
        "TrackMouseEvent",
        
        # Painting
        "BeginPaint", "EndPaint", "InvalidateRect", "ValidateRect", "GetDC", "ReleaseDC",
        "GetWindowDC", "GetDCEx", "RedrawWindow", "ScrollWindow", "ScrollWindowEx",
        
        # Menus
        "CreateMenu", "CreatePopupMenu", "DestroyMenu", "AppendMenuA", "AppendMenuW",
        "InsertMenuA", "InsertMenuW", "DeleteMenu", "SetMenu", "GetMenu", "GetSubMenu",
        "TrackPopupMenu", "EnableMenuItem", "CheckMenuItem",
        
        # Scrollbar
        "SetScrollInfo", "GetScrollInfo", "SetScrollPos", "GetScrollPos", "SetScrollRange", "GetScrollRange",
        
        # System
        "SystemParametersInfoA", "SystemParametersInfoW", "GetSystemMetrics",
        "LoadIconA", "LoadIconW", "LoadCursorA", "LoadCursorW", "LoadBitmapA", "LoadBitmapW",
        "LoadImageA", "LoadImageW",
    },
    
    "gdi32": {
        # Device context
        "CreateDCA", "CreateDCW", "CreateCompatibleDC", "DeleteDC", "SaveDC", "RestoreDC",
        "GetDeviceCaps", "SetMapMode", "GetMapMode", "SetViewportOrgEx", "GetViewportOrgEx",
        
        # Drawing
        "MoveToEx", "LineTo", "Polyline", "Polygon", "Rectangle", "Ellipse", "RoundRect",
        "Arc", "Pie", "Chord", "FillRect", "FrameRect", "InvertRect", "DrawEdge", "DrawFrameControl",
        
        # Bitmap operations
        "CreateBitmap", "CreateCompatibleBitmap", "CreateDIBSection", "GetDIBits", "SetDIBits",
        "BitBlt", "StretchBlt", "PatBlt", "MaskBlt", "PlgBlt",
        
        # GDI objects
        "SelectObject", "DeleteObject", "GetStockObject", "GetObject", "GetObjectType",
        
        # Pen and brush
        "CreatePen", "CreatePenIndirect", "CreateSolidBrush", "CreateHatchBrush",
        "CreateBrushIndirect", "CreatePatternBrush", "CreateDIBPatternBrush",
        
        # Font and text
        "CreateFontA", "CreateFontW", "CreateFont", "CreateFontIndirectA", "CreateFontIndirectW",
        "TextOutA", "TextOutW", "ExtTextOutA", "ExtTextOutW", "DrawTextA", "DrawTextW",
        "GetTextExtentPoint32A", "GetTextExtentPoint32W", "GetTextMetricsA", "GetTextMetricsW",
        "SetTextColor", "GetTextColor", "SetBkColor", "GetBkColor", "SetBkMode", "GetBkMode",
        "SetTextAlign", "GetTextAlign",
        
        # Color
        "SetPixel", "GetPixel", "RGB", "GetRValue", "GetGValue", "GetBValue",
        "CreatePalette", "SelectPalette", "RealizePalette",
        
        # Region
        "CreateRectRgn", "CreateRectRgnIndirect", "CreateEllipticRgn", "CreatePolygonRgn",
        "CombineRgn", "EqualRgn", "OffsetRgn", "PtInRegion", "RectInRegion",
    },
    
    "ws2_32": {
        # Winsock 2.2 (available on Win98 with update)
        "WSAStartup", "WSACleanup", "WSAGetLastError", "WSASetLastError",
        "socket", "closesocket", "bind", "listen", "accept", "connect",
        "send", "recv", "sendto", "recvfrom", "shutdown",
        "setsockopt", "getsockopt", "getsockname", "getpeername",
        "select", "ioctlsocket", "WSAAsyncSelect", "WSAEventSelect",
        "gethostbyname", "gethostbyaddr", "getservbyname", "getservbyport",
        "inet_addr", "inet_ntoa", "htons", "htonl", "ntohs", "ntohl",
    },
    
    "comctl32": {
        # Common controls (IE 5.0 version on Win98 SE)
        "InitCommonControls", "InitCommonControlsEx",
        "CreateStatusWindow", "DrawStatusText",
        "CreateToolbarEx", "CreateMappedBitmap",
        "ImageList_Create", "ImageList_Destroy", "ImageList_Add", "ImageList_Remove",
        "ImageList_Draw", "ImageList_GetImageCount",
    },
    
    "advapi32": {
        # Registry operations
        "RegOpenKeyA", "RegOpenKeyW", "RegOpenKeyExA", "RegOpenKeyExW",
        "RegCreateKeyA", "RegCreateKeyW", "RegCreateKeyExA", "RegCreateKeyExW",
        "RegCloseKey", "RegDeleteKeyA", "RegDeleteKeyW",
        "RegQueryValueA", "RegQueryValueW", "RegQueryValueExA", "RegQueryValueExW",
        "RegSetValueA", "RegSetValueW", "RegSetValueExA", "RegSetValueExW",
        "RegEnumKeyA", "RegEnumKeyW", "RegEnumValueA", "RegEnumValueW",
    },
}

# Incompatible APIs (Windows 2000+, XP+, Vista+)
INCOMPATIBLE_APIS = {
    "windows_2000": {
        "apis": [
            "GetProcessDEPPolicy", "SetProcessDEPPolicy",
            "InitializeCriticalSectionEx", "InitializeCriticalSectionAndSpinCount",
            "SetCriticalSectionSpinCount",
            "GetLongPathNameA", "GetLongPathNameW",
            "GetDiskFreeSpaceExA", "GetDiskFreeSpaceExW",
            "GetFileAttributesExA", "GetFileAttributesExW",
            "CreateHardLinkA", "CreateHardLinkW",
            "GetSystemWindowsDirectoryA", "GetSystemWindowsDirectoryW",
            "GetSystemWow64DirectoryA", "GetSystemWow64DirectoryW",
            "IsDebuggerPresent", "CheckRemoteDebuggerPresent",
        ],
        "alternatives": {
            "GetLongPathNameA": "Use GetShortPathName and manual conversion",
            "GetDiskFreeSpaceExA": "Use GetDiskFreeSpace",
            "GetFileAttributesExA": "Use GetFileAttributes",
            "IsDebuggerPresent": "Check manually or use __try/__except",
        }
    },
    
    "windows_xp": {
        "apis": [
            "GetProcessId", "GetThreadId",
            "GetTickCount64",
            "CreateEventExA", "CreateEventExW",
            "CreateMutexExA", "CreateMutexExW",
            "CreateSemaphoreExA", "CreateSemaphoreExW",
            "SetFileInformationByHandle",
            "GetFileInformationByHandleEx",
            "GetFinalPathNameByHandleA", "GetFinalPathNameByHandleW",
            "InitOnceInitialize", "InitOnceExecuteOnce",
            "GetNativeSystemInfo",
            "ActivateActCtx", "CreateActCtxA", "CreateActCtxW",
        ],
        "alternatives": {
            "GetTickCount64": "Use GetTickCount (wraps after 49.7 days)",
            "GetProcessId": "Use GetCurrentProcessId or store PID manually",
            "CreateEventExA": "Use CreateEventA",
            "CreateMutexExA": "Use CreateMutexA",
        }
    },
    
    "windows_vista": {
        "apis": [
            "GetProductInfo",
            "GetLogicalProcessorInformation", "GetLogicalProcessorInformationEx",
            "InitializeSRWLock", "AcquireSRWLockExclusive", "ReleaseSRWLockExclusive",
            "InitializeConditionVariable", "SleepConditionVariableCS",
            "WakeConditionVariable", "WakeAllConditionVariable",
            "CreateThreadpoolWork", "SubmitThreadpoolWork",
            "GetCurrentProcessorNumber",
            "SetThreadpoolThreadMinimum", "SetThreadpoolThreadMaximum",
            "CancelIoEx", "CancelSynchronousIo",
        ],
        "alternatives": {
            "InitializeSRWLock": "Use CRITICAL_SECTION",
            "InitializeConditionVariable": "Use Events and Mutexes",
            "CreateThreadpoolWork": "Use CreateThread manually",
        }
    },
    
    "windows_7": {
        "apis": [
            "GetSystemTimePreciseAsFileTime",
            "QueryThreadCycleTime", "QueryProcessCycleTime",
            "SetFileInformationByHandle",
            "GetThreadInformation", "SetThreadInformation",
        ],
        "alternatives": {
            "GetSystemTimePreciseAsFileTime": "Use GetSystemTimeAsFileTime",
        }
    },
}

# Suspicious patterns that may indicate newer API usage
SUSPICIOUS_PATTERNS = [
    # C++11 and later features
    r"std::thread",
    r"std::mutex",
    r"std::condition_variable",
    r"std::atomic",
    r"std::shared_ptr",
    r"std::unique_ptr",
    r"std::make_shared",
    r"std::make_unique",
    r"auto\s+\w+\s*=",  # auto keyword
    r"nullptr",
    r"override",
    r"final",
    r"\[\[.*\]\]",  # attributes
    
    # .NET Framework
    r"System::",
    r"using\s+namespace\s+System",
    r"gcnew",
    r"cli::",
    
    # Modern Windows APIs
    r"Windows\.h",  # Should be windows.h
    r"WinRT::",
    r"Windows::Foundation",
    
    # Unicode issues
    r"UNICODE\s+",  # Check if UNICODE is defined without _UNICODE
    
    # Newer CRT functions
    r"sprintf_s",
    r"strcpy_s",
    r"strcat_s",
    r"_s\(",  # Secure CRT functions
]

# API metadata with minimum OS version
API_METADATA = {
    "CreateFileA": {"dll": "kernel32", "min_os": "Windows 95", "signature": "HANDLE CreateFileA(LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE)"},
    "CreateWindowExA": {"dll": "user32", "min_os": "Windows 95", "signature": "HWND CreateWindowExA(DWORD, LPCSTR, LPCSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID)"},
    "WSAStartup": {"dll": "ws2_32", "min_os": "Windows 95 + Winsock 2 update", "signature": "int WSAStartup(WORD, LPWSADATA)"},
    "GetTickCount": {"dll": "kernel32", "min_os": "Windows 95", "signature": "DWORD GetTickCount(void)"},
    "GetTickCount64": {"dll": "kernel32", "min_os": "Windows Vista", "signature": "ULONGLONG GetTickCount64(void)"},
}
