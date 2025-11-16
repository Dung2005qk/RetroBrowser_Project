# Requirements Document

## Introduction

Dự án RetroBrowser hiện tại đã có file build_quick.bat để build cho Windows 98, nhưng cần đảm bảo tương thích hoàn toàn với Win98 bằng cách kiểm tra kỹ lưỡng từng file source code và tạo một file bat build riêng biệt tối ưu cho Win98. Mục tiêu là đảm bảo binary có thể chạy trên Windows 98 SE với phần cứng giới hạn (200MHz CPU, 64MB RAM).

## Glossary

- **Win98 System**: Windows 98 Second Edition operating system
- **Build Script**: File batch (.bat) để compile source code thành executable
- **Compatibility Flags**: Các compiler flags đảm bảo tương thích với Win98
- **Source Files**: Các file C++ trong thư mục src/browser
- **Win32 API**: Windows API version tương thích với Win98
- **WINVER**: Macro định nghĩa target Windows version (0x0410 cho Win98)

## Requirements

### Requirement 1

**User Story:** Là developer, tôi muốn kiểm tra tất cả source files để đảm bảo không sử dụng API không tương thích với Win98, để binary có thể chạy trên Win98 thực tế

#### Acceptance Criteria

1. WHEN kiểm tra source files, THE Win98 System SHALL xác định tất cả các Win32 API calls được sử dụng
2. THE Win98 System SHALL xác minh rằng tất cả API calls tồn tại trong Windows 98 SE
3. IF phát hiện API không tương thích, THEN THE Win98 System SHALL ghi nhận và đề xuất thay thế
4. THE Win98 System SHALL kiểm tra các header files không sử dụng features từ Windows 2000 trở lên
5. THE Win98 System SHALL xác nhận WINVER và _WIN32_WINNT được set đúng (0x0410)

### Requirement 2

**User Story:** Là developer, tôi muốn có một file bat build riêng tối ưu cho Win98, để dễ dàng build và maintain binary cho Win98

#### Acceptance Criteria

1. THE Build Script SHALL tạo file build_win98.bat riêng biệt từ build_quick.bat
2. THE Build Script SHALL sử dụng compiler flags tối ưu cho Win98 (/MACHINE:X86, /SUBSYSTEM:WINDOWS,4.10)
3. THE Build Script SHALL link với các libraries tương thích Win98 (kernel32.lib, user32.lib, gdi32.lib, ws2_32.lib)
4. THE Build Script SHALL set WINVER=0x0410 và _WIN32_WINNT=0x0410
5. THE Build Script SHALL tạo output binary có tên rõ ràng (RetroBrowser_Win98.exe)

### Requirement 3

**User Story:** Là developer, tôi muốn build script kiểm tra môi trường build, để đảm bảo compiler và tools phù hợp

#### Acceptance Criteria

1. WHEN chạy build script, THE Build Script SHALL kiểm tra Visual Studio environment được setup
2. THE Build Script SHALL xác minh x86 toolchain được load (không phải x64)
3. IF môi trường không đúng, THEN THE Build Script SHALL hiển thị error message rõ ràng
4. THE Build Script SHALL tạo thư mục obj và deploy nếu chưa tồn tại
5. THE Build Script SHALL hiển thị thông tin về binary size sau khi build thành công

### Requirement 4

**User Story:** Là developer, tôi muốn compiler flags được tối ưu cho Win98, để binary nhỏ gọn và chạy nhanh trên phần cứng yếu

#### Acceptance Criteria

1. THE Build Script SHALL sử dụng /MT flag để static link CRT (không cần MSVCRT.DLL)
2. THE Build Script SHALL sử dụng /O1 hoặc /O2 để optimize cho size hoặc speed
3. THE Build Script SHALL không sử dụng features C++11 trở lên
4. THE Build Script SHALL set /D _MBCS để support multi-byte character set
5. THE Build Script SHALL include debug symbols (/Zi) nhưng optimize code (/O2)

### Requirement 5

**User Story:** Là developer, tôi muốn kiểm tra các dependencies và libraries, để đảm bảo không có dependency không tương thích với Win98

#### Acceptance Criteria

1. THE Win98 System SHALL kiểm tra tất cả #include directives trong source files
2. THE Win98 System SHALL xác minh không sử dụng STL features không có trong VC6
3. THE Win98 System SHALL kiểm tra libjpeg library tương thích với Win98
4. THE Win98 System SHALL xác nhận Winsock 2.2 được sử dụng (không phải Winsock 1.1)
5. THE Win98 System SHALL kiểm tra không có dependency vào .NET Framework

### Requirement 6

**User Story:** Là developer, tôi muốn build script có error handling tốt, để dễ dàng debug khi build fail

#### Acceptance Criteria

1. WHEN build fail, THE Build Script SHALL hiển thị error message chi tiết
2. THE Build Script SHALL check ERRORLEVEL sau mỗi compilation step
3. THE Build Script SHALL giữ lại obj files khi build fail để debug
4. THE Build Script SHALL log compiler output vào file nếu cần
5. THE Build Script SHALL pause ở cuối để user có thể đọc messages

### Requirement 7

**User Story:** Là developer, tôi muốn documentation về Win98 compatibility, để team hiểu rõ các giới hạn và best practices

#### Acceptance Criteria

1. THE Build Script SHALL có comments giải thích mỗi compiler flag
2. THE Build Script SHALL document các Win98 limitations ở đầu file
3. THE Build Script SHALL list các tested Win98 versions (98 SE, 98 FE)
4. THE Build Script SHALL include instructions về cách test trên Win98 VM
5. THE Build Script SHALL note về minimum hardware requirements (RAM, CPU)
