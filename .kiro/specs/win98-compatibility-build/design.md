# Design Document

## Overview

Thiết kế giải pháp toàn diện để đảm bảo RetroBrowser tương thích 100% với Windows 98 SE. Giải pháp bao gồm:

1. **Automated Source Code Analysis**: Script Python để scan tất cả source files và phát hiện API không tương thích
2. **Optimized Build Script**: File `build_win98.bat` mới với compiler flags tối ưu cho Win98
3. **Compatibility Report**: Document chi tiết về các API được sử dụng và tính tương thích
4. **Testing Guidelines**: Hướng dẫn test trên Win98 VM

### Key Design Principles

- **Defense in Depth**: Nhiều lớp kiểm tra (compile-time, link-time, runtime)
- **Explicit over Implicit**: Mọi setting đều được khai báo rõ ràng
- **Fail Fast**: Phát hiện lỗi sớm nhất có thể trong build process
- **Documentation First**: Mọi decision đều được document

## Architecture

### Component Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                   Win98 Compatibility System                │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌──────────────────┐         ┌──────────────────┐         │
│  │  Source Code     │────────>│  API Checker     │         │
│  │  Scanner         │         │  (Python)        │         │
│  └──────────────────┘         └────────┬─────────┘         │
│                                        │                   │
│                                        v                   │
│                              ┌──────────────────┐          │
│                              │  Compatibility   │          │
│                              │  Report          │          │
│                              └──────────────────┘          │
│                                                             │
│  ┌──────────────────┐         ┌──────────────────┐         │
│  │  build_win98.bat │────────>│  Compiler        │         │
│  │  (Optimized)     │         │  (VC++ / VS)     │         │
│  └──────────────────┘         └────────┬─────────┘         │
│                                        │                   │
│                                        v                   │
│                              ┌──────────────────┐          │
│                              │  RetroBrowser    │          │
│                              │  _Win98.exe      │          │
│                              └──────────────────┘          │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### Build Flow

```
[Developer] ──> [build_win98.bat]
                      │
                      ├──> Check VS Environment
                      │
                      ├──> Create Directories (obj/, deploy/)
                      │
                      ├──> Set Win98 Compiler Flags
                      │    • WINVER=0x0410
                      │    • /SUBSYSTEM:WINDOWS,4.10
                      │    • /MACHINE:X86
                      │    • /MT (static CRT)
                      │
                      ├──> Compile All Modules
                      │    • stdafx.cpp (PCH)
                      │    • main.cpp
                      │    • ui.cpp
                      │    • network.cpp
                      │    • parser.cpp
                      │    • renderer.cpp
                      │
                      ├──> Link with Win98 Libraries
                      │    • kernel32.lib
                      │    • user32.lib
                      │    • gdi32.lib
                      │    • ws2_32.lib
                      │    • comctl32.lib
                      │
                      └──> Output: RetroBrowser_Win98.exe
```

## Components and Interfaces

### 1. API Compatibility Checker (Python Script)

**File**: `tools/check_win98_compatibility.py`

**Purpose**: Scan source code và phát hiện API calls không tương thích với Win98

**Interface**:
```python
class Win98APIChecker:
    def __init__(self):
        # Load Win98 API whitelist
        self.win98_apis = self._load_win98_api_list()
        # Load incompatible API patterns
        self.incompatible_patterns = self._load_incompatible_patterns()
    
    def scan_file(self, filepath: str) -> List[APIIssue]:
        """Scan một file và return list các issues"""
        pass
    
    def scan_directory(self, dirpath: str) -> Dict[str, List[APIIssue]]:
        """Scan toàn bộ directory"""
        pass
    
    def generate_report(self, issues: Dict) -> str:
        """Generate HTML/text report"""
        pass
```

**Win98 API Whitelist** (Partial):
- **kernel32.dll**: CreateFile, ReadFile, WriteFile, GetTempPath, GetTickCount, CreateThread, WaitForSingleObject
- **user32.dll**: CreateWindowEx, ShowWindow, UpdateWindow, GetMessage, DispatchMessage, MessageBox, SetWindowText
- **gdi32.dll**: CreateCompatibleDC, SelectObject, BitBlt, TextOut, CreateFont, DeleteObject
- **ws2_32.dll**: WSAStartup, socket, connect, send, recv, closesocket, WSACleanup

**Incompatible API Patterns**:
- Windows 2000+: `GetProcessDEPPolicy`, `InitializeCriticalSectionEx`, `GetTickCount64`
- Windows XP+: `GetProcessId`, `CreateEventEx`, `SetFileInformationByHandle`
- Windows Vista+: `GetProductInfo`, `GetLogicalProcessorInformation`
- .NET Framework: Any `System::` namespace usage

### 2. Build Script (build_win98.bat)

**File**: `build_win98.bat`

**Purpose**: Optimized build script cho Win98 với tất cả compatibility flags

**Key Features**:
- Environment validation
- Win98-specific compiler flags
- Static CRT linking (/MT)
- Subsystem version targeting (4.10)
- Comprehensive error handling
- Build output validation

**Compiler Flags Breakdown**:

| Flag | Purpose | Win98 Impact |
|------|---------|--------------|
| `/DWINVER=0x0410` | Target Win98 API | Prevents newer API usage |
| `/D_WIN32_WINNT=0x0410` | NT API level | Ensures NT 4.0 compatibility |
| `/D_WIN32_WINDOWS=0x0410` | Win9x family | Explicit Win98 targeting |
| `/D_WIN32_IE=0x0500` | IE version | IE 5.0 (shipped with Win98 SE) |
| `/MT` | Static CRT | No MSVCRT.DLL dependency |
| `/O2` | Optimize speed | Better performance on 200MHz CPU |
| `/Oy-` | No frame pointer omission | Better debugging on Win98 |
| `/GZ` | Stack checks | Catch buffer overruns |
| `/SUBSYSTEM:WINDOWS,4.10` | Win98 subsystem | Critical for Win98 loader |
| `/MACHINE:X86` | 32-bit x86 | Win98 only supports x86 |

### 3. Compatibility Report Generator

**Output**: `docs/win98_compatibility_report.md`

**Sections**:
1. **Executive Summary**: Overall compatibility status
2. **API Usage Analysis**: All Win32 APIs used, categorized by DLL
3. **Potential Issues**: Flagged APIs that need review
4. **Dependencies**: External libraries and their Win98 status
5. **Recommendations**: Suggested changes if any
6. **Test Results**: Results from Win98 VM testing

### 4. Enhanced Build Validation

**Post-Build Checks**:
```batch
REM Check PE header for correct subsystem version
dumpbin /headers deploy\RetroBrowser_Win98.exe | findstr "subsystem"

REM Check for dependencies
dumpbin /dependents deploy\RetroBrowser_Win98.exe

REM Verify file size (should be < 2MB for Win98)
dir deploy\RetroBrowser_Win98.exe
```

## Data Models

### APIIssue Structure
```python
@dataclass
class APIIssue:
    file: str           # Source file path
    line: int           # Line number
    api_name: str       # API function name
    severity: str       # "ERROR", "WARNING", "INFO"
    message: str        # Description
    suggestion: str     # Recommended fix
```

### BuildConfig Structure
```python
@dataclass
class BuildConfig:
    target_os: str = "Windows 98 SE"
    winver: str = "0x0410"
    subsystem_version: str = "4.10"
    machine: str = "X86"
    crt_linking: str = "static"  # /MT
    optimization: str = "speed"  # /O2
    debug_info: bool = True      # /Zi
```

## Error Handling

### Build Script Error Scenarios

1. **VS Environment Not Found**
   - Error: "Visual Studio environment not detected"
   - Action: Display paths to check, suggest manual vcvarsall.bat call
   - Exit code: 1

2. **Compilation Failed**
   - Error: "Compilation failed for [module]"
   - Action: Keep obj files, display compiler errors, pause
   - Exit code: 2

3. **Linking Failed**
   - Error: "Linking failed - check library paths"
   - Action: Display linker errors, suggest library installation
   - Exit code: 3

4. **Post-Build Validation Failed**
   - Error: "Binary validation failed - may not run on Win98"
   - Action: Display dumpbin output, warn user
   - Exit code: 4

### API Checker Error Scenarios

1. **Incompatible API Detected**
   - Severity: ERROR
   - Message: "API [name] requires Windows [version]+"
   - Suggestion: "Use [alternative] instead"

2. **Suspicious Pattern**
   - Severity: WARNING
   - Message: "Pattern [pattern] may indicate Win2K+ usage"
   - Suggestion: "Verify manually in MSDN"

3. **Unknown API**
   - Severity: INFO
   - Message: "API [name] not in Win98 whitelist"
   - Suggestion: "Verify in MSDN Win98 documentation"

## Testing Strategy

### Phase 1: Static Analysis
- Run API checker on all source files
- Verify no ERROR-level issues
- Review all WARNING-level issues
- Document all INFO-level findings

### Phase 2: Build Validation
- Build with build_win98.bat
- Verify PE header subsystem version = 4.10
- Check dependencies (no MSVCR100.dll, etc.)
- Verify file size < 2MB

### Phase 3: Win98 VM Testing

**Test Environment**:
- OS: Windows 98 SE (4.10.2222 A)
- CPU: 200MHz (or VM equivalent)
- RAM: 128MB
- Network: Host-only adapter to proxy

**Test Cases**:
1. **Launch Test**: Binary starts without "procedure not found" errors
2. **UI Test**: Window displays correctly, controls functional
3. **Network Test**: Can connect to proxy, fetch simple page
4. **Rendering Test**: HTML displays correctly
5. **Image Test**: BMP images load and display
6. **Stability Test**: Run for 30 minutes, check for crashes/leaks
7. **Performance Test**: Page load time < 5 seconds for simple pages

### Phase 4: Regression Testing
- Test on Windows 98 FE (First Edition)
- Test on Windows ME (if available)
- Test with different proxy configurations
- Test with various HTML content types

## Performance Considerations

### Optimization for Win98 Hardware

**Target Hardware**:
- CPU: 200MHz Pentium MMX
- RAM: 64-128MB
- HDD: 2GB, 5400 RPM

**Optimization Strategies**:

1. **Code Size**:
   - Use /O1 (optimize for size) if binary > 2MB
   - Consider removing debug symbols for release build
   - Static linking reduces DLL dependencies but increases size

2. **Memory Usage**:
   - Limit image cache to 4MB (renderer.cpp)
   - Cap response buffer to 1MB (network.cpp)
   - Use stack allocation where possible

3. **CPU Usage**:
   - Minimize string copies (use references)
   - Cache font handles (renderer.cpp already does this)
   - Avoid unnecessary repaints

4. **Disk I/O**:
   - Minimize temp file usage
   - Batch log writes
   - Use memory buffers for small data

### Build Time Optimization

- Precompiled headers (stdafx.h) already implemented
- Incremental linking disabled (/INCREMENTAL:NO) for smaller binary
- Parallel compilation not available in VC++6, but VS2022 can use /MP

## Security Considerations

### Win98 Security Limitations

**Known Issues**:
- No DEP (Data Execution Prevention)
- No ASLR (Address Space Layout Randomization)
- Limited SSL/TLS support (proxy handles this)

**Mitigations**:
- Input validation in parser (already implemented)
- Buffer overflow checks (/GZ flag)
- Proxy sanitizes malicious content
- No script execution (by design)

### Build Security

- Static CRT linking prevents DLL hijacking
- No external DLL dependencies (except system DLLs)
- Debug symbols in separate .pdb file (can be removed for distribution)

## Deployment Considerations

### Distribution Package

**Files to Include**:
```
RetroBrowser_Win98/
├── RetroBrowser_Win98.exe    # Main binary
├── README_Win98.txt           # Win98-specific instructions
└── proxy/                     # Python proxy (runs on host)
    ├── proxy.py
    ├── config.py
    └── requirements.txt
```

**Installation Instructions** (README_Win98.txt):
1. Copy RetroBrowser_Win98.exe to Win98 VM
2. Ensure Winsock 2.2 is installed (check for ws2_32.dll)
3. Configure network adapter (host-only)
4. Start proxy on host machine
5. Launch RetroBrowser_Win98.exe

### Compatibility Matrix

| OS Version | Status | Notes |
|------------|--------|-------|
| Windows 98 FE | ✅ Supported | Requires Winsock 2.2 update |
| Windows 98 SE | ✅ Fully Supported | Primary target |
| Windows ME | ✅ Supported | Same Win9x kernel |
| Windows 2000 | ✅ Compatible | Overqualified |
| Windows XP | ✅ Compatible | Overqualified |
| Windows 95 | ❌ Not Supported | Winsock 2.2 issues |
| Windows NT 4.0 | ⚠️ Untested | Should work theoretically |

## Future Enhancements

### Potential Improvements

1. **Automated VM Testing**:
   - Script to deploy and test on Win98 VM automatically
   - Capture screenshots for visual regression testing

2. **Performance Profiling**:
   - Add instrumentation to measure function timings
   - Identify bottlenecks on real Win98 hardware

3. **Alternative Compilers**:
   - Test with Open Watcom C++ (native Win98 compiler)
   - Compare binary size and performance

4. **Enhanced Compatibility Checker**:
   - Parse MSDN documentation automatically
   - Build comprehensive Win98 API database
   - Check for undocumented API usage

5. **Continuous Integration**:
   - Automated builds on commit
   - Automated compatibility checks
   - Automated Win98 VM testing (if feasible)
