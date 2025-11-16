# Implementation Plan

- [x] 1. Tạo Win98 API Compatibility Checker





  - Tạo Python script để scan source code và phát hiện API không tương thích với Windows 98
  - Script sẽ check tất cả .cpp và .h files trong src/browser
  - Output là report chi tiết về các API được sử dụng và compatibility status
  - _Requirements: 1.1, 1.2, 1.3, 1.4, 1.5, 5.1, 5.2, 5.3, 5.4, 5.5_

- [x] 1.1 Tạo Win98 API whitelist database


  - Tạo file JSON/Python dict chứa list các Win32 APIs tương thích với Win98
  - Categorize theo DLL (kernel32, user32, gdi32, ws2_32, comctl32)
  - Include function signatures và minimum OS version
  - _Requirements: 1.1, 1.2_


- [x] 1.2 Tạo incompatible API patterns list

  - List các APIs từ Windows 2000, XP, Vista trở lên
  - Include regex patterns để detect suspicious code
  - Document alternatives cho mỗi incompatible API
  - _Requirements: 1.3_


- [x] 1.3 Implement source code scanner

  - Parse C++ files để extract function calls
  - Match với whitelist và incompatible patterns
  - Track file location (file, line number) cho mỗi finding
  - Handle false positives (comments, strings)
  - _Requirements: 1.1, 1.2, 1.3_

- [x] 1.4 Implement compatibility report generator


  - Generate HTML report với color-coded severity levels
  - Include statistics (total APIs, compatible %, issues found)
  - Provide suggestions cho mỗi incompatible API
  - Export plain text version cho CI/CD
  - _Requirements: 1.3, 7.1, 7.2_

- [x] 2. Tạo optimized build_win98.bat script





  - Tạo build script mới riêng biệt từ build_quick.bat
  - Include tất cả Win98 compatibility flags
  - Add comprehensive error handling và validation
  - _Requirements: 2.1, 2.2, 2.3, 2.4, 2.5, 3.1, 3.2, 3.3, 3.4, 3.5, 4.1, 4.2, 4.3, 4.4, 4.5, 6.1, 6.2, 6.3, 6.4, 6.5_


- [x] 2.1 Setup environment validation

  - Check Visual Studio environment variables
  - Verify x86 toolchain is loaded (not x64)
  - Display clear error messages if environment incorrect
  - _Requirements: 3.1, 3.2, 3.3_


- [x] 2.2 Configure Win98-specific compiler flags

  - Set WINVER=0x0410, _WIN32_WINNT=0x0410, _WIN32_WINDOWS=0x0410
  - Use /MT for static CRT linking
  - Set /O2 for speed optimization (or /O1 for size)
  - Add /Oy- to keep frame pointers for debugging
  - Include /GZ for stack checks
  - _Requirements: 2.2, 2.3, 4.1, 4.2, 4.3, 4.4, 4.5_

- [x] 2.3 Configure Win98-specific linker flags

  - Set /SUBSYSTEM:WINDOWS,4.10 (critical for Win98 loader)
  - Set /MACHINE:X86
  - Use /INCREMENTAL:NO for smaller binary
  - Link with Win98-compatible libraries only
  - _Requirements: 2.2, 2.3_



- [x] 2.4 Add build output validation

  - Check ERRORLEVEL after compilation
  - Display binary size and location
  - Show success/failure message clearly
  - Pause at end for user to read output
  - _Requirements: 2.5, 3.5, 6.1, 6.2, 6.5_




- [x] 2.5 Add comprehensive comments and documentation

  - Comment mỗi compiler flag với explanation
  - Document Win98 limitations ở header
  - Include usage instructions
  - Note tested Win98 versions
  - _Requirements: 7.1, 7.2, 7.3, 7.4, 7.5_

- [x] 3. Tạo post-build validation script





  - Script để verify binary tương thích với Win98
  - Check PE header, dependencies, subsystem version
  - _Requirements: 2.5, 3.5_


- [x] 3.1 Implement PE header checker

  - Use dumpbin hoặc Python pefile library
  - Verify subsystem version = 4.10
  - Check machine type = x86
  - Verify no .NET metadata
  - _Requirements: 2.5_


- [x] 3.2 Implement dependency checker

  - Extract DLL dependencies from PE
  - Verify all DLLs exist on Win98 (kernel32, user32, gdi32, ws2_32)
  - Flag any unexpected dependencies (MSVCR100.dll, etc.)
  - _Requirements: 5.5_



- [x] 3.3 Implement size and resource checker

  - Check binary size < 2MB (reasonable for Win98)
  - Verify no excessive resource usage
  - Check for debug symbols location (.pdb)
  - _Requirements: 3.5_

- [x] 4. Tạo Win98 compatibility documentation





  - Document toàn bộ Win98 compatibility considerations
  - Include testing guidelines cho Win98 VM
  - _Requirements: 7.1, 7.2, 7.3, 7.4, 7.5_

- [x] 4.1 Tạo Win98 testing guide


  - Document Win98 VM setup (VirtualBox/VMware)
  - Network configuration (host-only adapter)
  - Installation steps cho RetroBrowser
  - Test cases để verify functionality
  - _Requirements: 7.4_


- [x] 4.2 Tạo compatibility matrix document

  - List supported OS versions (98 FE, 98 SE, ME)
  - Document known limitations
  - Include troubleshooting section
  - _Requirements: 7.3, 7.5_


- [x] 4.3 Update main README with Win98 build instructions

  - Add section về build_win98.bat usage
  - Link to compatibility documentation
  - Include quick start guide
  - _Requirements: 7.1, 7.2_

- [x] 5. Run comprehensive compatibility check




  - Execute API checker trên toàn bộ codebase
  - Review findings và fix issues nếu có
  - Generate final compatibility report
  - _Requirements: 1.1, 1.2, 1.3, 1.4, 1.5_

- [x] 5.1 Scan all source files


  - Run checker trên src/browser/**/*.cpp và *.h
  - Collect all findings
  - Categorize by severity (ERROR, WARNING, INFO)
  - _Requirements: 1.1, 1.2_


- [x] 5.2 Review and triage findings

  - Investigate mỗi ERROR-level issue
  - Verify WARNING-level issues manually
  - Document INFO-level findings
  - _Requirements: 1.3_


- [x] 5.3 Fix incompatible code if found

  - Replace incompatible APIs với Win98 alternatives
  - Test fixes không break functionality
  - Update code comments
  - _Requirements: 1.3_


- [x] 5.4 Generate final compatibility report

  - Run checker lần cuối sau fixes
  - Generate HTML và text reports
  - Save to docs/win98_compatibility_report.md
  - _Requirements: 1.3, 7.1_

- [x] 6. Build and validate Win98 binary





  - Build using build_win98.bat
  - Run post-build validation
  - Verify binary ready for Win98 deployment
  - _Requirements: 2.1, 2.2, 2.3, 2.4, 2.5, 3.1, 3.2, 3.3, 3.4, 3.5_

- [x] 6.1 Execute build_win98.bat




  - Run build script
  - Monitor for errors
  - Verify successful compilation
  - _Requirements: 2.1, 2.2, 2.3, 2.4, 2.5_


- [x] 6.2 Run post-build validation

  - Execute PE header checks
  - Verify dependencies
  - Check binary size
  - _Requirements: 3.1, 3.2, 3.3, 3.4, 3.5_


- [x] 6.3 Compare with original build


  - Compare RetroBrowser_Win98.exe với RetroBrowser.exe
  - Verify size difference reasonable
  - Document any significant changes
  - _Requirements: 2.5, 3.5_



- [x] 6.4 Test on Win98 VM (if available)

  - Deploy binary to Win98 VM
  - Run basic functionality tests
  - Verify no "procedure not found" errors
  - Test network connectivity through proxy
  - _Requirements: 7.4_
