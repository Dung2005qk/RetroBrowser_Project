**BTL Hệ Điều Hành**

---

<div align="center">

# 🌐 RetroBrowser Project
### Windows 98 Retro Web Browser with Modern Connectivity

*A Comprehensive Operating Systems Course Project*

[![Win98 Compatible](https://img.shields.io/badge/Windows%2098-Compatible-blue.svg)](https://en.wikipedia.org/wiki/Windows_98)
[![C++98](https://img.shields.io/badge/C%2B%2B-98-00599C.svg)](https://en.cppreference.com/w/cpp/98)
[![Python 3.x](https://img.shields.io/badge/Python-3.x-3776AB.svg)](https://www.python.org/)
[![Test Coverage](https://img.shields.io/badge/Tests-24%2F24%20Passing-success.svg)](tools/test_scripts/)

[Tổng Quan](#-tổng-quan) •
[Kiến Trúc](#️-kiến-trúc-hệ-thống) •
[Tính Năng](#-tính-năng-nổi-bật) •
[Cài Đặt](#️-cài-đặt--triển-khai) •
[Sử Dụng](#-sử-dụng) •
[Testing](#-testing)

</div>

---

## 📋 Mục Lục

- [Tổng Quan](#-tổng-quan)
  - [Giới Thiệu Dự Án](#giới-thiệu-dự-án)
  - [Mục Tiêu Học Thuật](#mục-tiêu-học-thuật)
  - [Phạm Vi & Giới Hạn](#phạm-vi--giới-hạn)
- [Kiến Trúc Hệ Thống](#️-kiến-trúc-hệ-thống)
  - [Sơ Đồ Tổng Quan](#sơ-đồ-tổng-quan)
  - [Luồng Xử Lý](#luồng-xử-lý)
  - [Các Module Chính](#các-module-chính)
- [Tính Năng Nổi Bật](#-tính-năng-nổi-bật)
- [Cấu Trúc Thư Mục](#-cấu-trúc-thư-mục)
- [Cài Đặt & Triển Khai](#️-cài-đặt--triển-khai)
- [Sử Dụng](#-sử-dụng)
- [Tài Liệu Chi Tiết](#-tài-liệu-chi-tiết)
- [Testing](#-testing)
- [Known Issues & Future](#-known-issues--future-work)
- [Credits](#-credits)

---

## 🎯 Tổng Quan

### Giới Thiệu Dự Án

**RetroBrowser** là một trình duyệt web đầy đủ chức năng được thiết kế để chạy trên **Windows 98**, kết hợp giữa kiến trúc cổ điển (Win32 API, GDI, Winsock 2.2) và khả năng truy cập web hiện đại (HTTPS, HTTP/2) thông qua một proxy server Python thông minh.

Dự án này là minh chứng cho việc áp dụng các khái niệm Hệ Điều Hành vào thực tế:
- **Process Management**: Multi-threaded image loading
- **Memory Management**: RAII pattern, resource lifecycle
- **Inter-Process Communication**: Browser ↔ Proxy qua TCP/IP
- **File I/O**: Cache management, log handling
- **Concurrency**: Async image downloads, event-driven architecture
- **API Programming**: Win32 API, Winsock, GDI mastery

### Mục Tiêu Học Thuật

#### 1. **Win32 API Mastery**
- Window management (CreateWindowEx, message loop)
- Event handling (WM_PAINT, WM_SIZE, WM_COMMAND)
- GDI rendering (TextOut, BitBlt, double-buffering)
- Controls (Edit, Button, Static) và layout

#### 2. **Network Programming (Winsock 2.2)**
- Socket initialization (WSAStartup)
- TCP connection management
- HTTP/1.1 protocol implementation
- Blocking I/O model
- Error handling (timeout, connection refused)

#### 3. **Concurrency & Threading**
- Worker threads cho async image loading
- Thread synchronization (PostMessage)
- Resource sharing (image cache)
- Deadlock prevention

#### 4. **Memory Management**
- RAII pattern (Resource Acquisition Is Initialization)
- Manual memory management (new/delete)
- GDI object lifecycle (HFONT, HBITMAP, HDC)
- Memory leak prevention

#### 5. **Parser Design**
- Finite State Machine (FSM)
- Single-pass parsing
- Error recovery
- String processing

### Phạm Vi & Giới Hạn

#### ✅ Trong Phạm Vi (Implemented)
- HTTP GET requests thông qua proxy
- HTML parsing cơ bản (H1-H6, P, A, IMG)
- Text rendering với multiple fonts
- Hyperlink navigation (clickable links)
- Asynchronous image loading (BMP format)
- Progressive rendering
- Scrolling support
- Error handling (404, timeout, network errors)

#### ⚠️ Giới Hạn (Out of Scope)
- **No CSS support**: Chỉ basic inline styles
- **No JavaScript**: Security và compatibility reasons
- **No POST/Forms**: Chỉ GET requests
- **No HTTPS direct**: Proxy handles TLS
- **Limited image formats**: BMP only (Win98 native)
- **No tabs**: Single window design
- **No bookmarks/history UI**: Có history stack nhưng no UI

#### 🎯 Lý Do Thiết Kế
1. **Time constraint**: 8 tuần development
2. **Win98 limitations**: 200MHz CPU, 64MB RAM
3. **Educational focus**: OS concepts over feature completeness
4. **Stability priority**: Simple = Stable on legacy hardware

---

## 🏗️ Kiến Trúc Hệ Thống

### Sơ Đồ Tổng Quan

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         WIN98 VIRTUAL MACHINE                            │
│  ┌───────────────────────────────────────────────────────────────────┐  │
│  │                    RetroBrowser.exe (C++)                         │  │
│  │  ┌─────────────┐  ┌──────────────┐  ┌────────────┐  ┌─────────┐ │  │
│  │  │   UI Module │──│ Main/Core    │──│   Network  │──│  Parser │ │  │
│  │  │  (Win32 GUI)│  │ (Orchestrator)│  │  (Winsock) │  │  (FSM)  │ │  │
│  │  └──────┬──────┘  └──────┬───────┘  └─────┬──────┘  └────┬────┘ │  │
│  │         │                 │                 │               │      │  │
│  │         │                 └─────────┬───────┘               │      │  │
│  │         │                           │                       │      │  │
│  │         │                           ▼                       ▼      │  │
│  │         │                    ┌─────────────┐         ┌──────────┐ │  │
│  │         └───────────────────►│  Renderer   │◄────────│  Blocks  │ │  │
│  │                               │  (GDI 2D)   │         │  (Data)  │ │  │
│  │                               └─────────────┘         └──────────┘ │  │
│  └───────────────────────────────────┬───────────────────────────────┘  │
│                                      │ HTTP/1.0 over TCP                │
└──────────────────────────────────────┼──────────────────────────────────┘
                                       │
                                       │ Host-Only Network
                                       │
┌──────────────────────────────────────┼──────────────────────────────────┐
│                         HOST MACHINE (Modern OS)                         │
│  ┌───────────────────────────────────▼───────────────────────────────┐  │
│  │                    Python Proxy Server                            │  │
│  │  ┌──────────────┐  ┌──────────────┐  ┌─────────────────────────┐ │  │
│  │  │   Socket     │──│  Sanitizer   │──│   HTTP/HTTPS Client     │ │  │
│  │  │  Listener    │  │  (BS4/Regex) │  │   (Requests Library)    │ │  │
│  │  │  (8080)      │  │              │  │                         │ │  │
│  │  └──────────────┘  └──────────────┘  └────────────┬────────────┘ │  │
│  └───────────────────────────────────────────────────┼──────────────┘  │
│                                                       │ HTTPS/HTTP2     │
└───────────────────────────────────────────────────────┼─────────────────┘
                                                        │
                                                        ▼
                                              ╔═══════════════╗
                                              ║  Modern Web   ║
                                              ║  (Internet)   ║
                                              ╚═══════════════╝
```

### Luồng Xử Lý

#### 1. **User Input Flow** 
```
User types URL → Address Bar (Edit Control)
               ↓
User clicks "Go" → WM_COMMAND message
               ↓
UI Module → OnNavigate callback
               ↓
Main/Core validates URL → NetworkManager
```

#### 2. **Network Flow**
```
NetworkManager::FetchUrl(url)
               ↓
Create TCP socket (Winsock)
               ↓
Connect to proxy (127.0.0.1:8080)
               ↓
Send: GET /http://example.com HTTP/1.0
               ↓
Proxy fetches via HTTPS
               ↓
Proxy sanitizes HTML (remove scripts/styles)
               ↓
NetworkManager receives response
               ↓
Return HttpResponse struct
```

#### 3. **Parsing Flow**
```
HtmlParser::Parse(html_vector)
               ↓
FSM single-pass through HTML
               ↓
Build flat block list:
  [{type: H1, content: "Title"},
   {type: P, content: "Text"},
   {type: IMG, src: "image.bmp"}]
               ↓
Return ParseResult
```

#### 4. **Rendering Flow**
```
Renderer::SetContent(blocks)
               ↓
Phase 1 - Calculate Layout:
  Measure text, compute positions
               ↓
Phase 2 - Paint on WM_PAINT:
  Draw to offscreen DC
  BitBlt to screen (flicker-free)
```

### Các Module Chính

#### 📦 **1. Core/Main Module** (`main.cpp` - 579 dòng)
**Nhiệm vụ**: Orchestrator - điều phối tất cả modules

**Chức năng**:
- WinMain entry point
- Callback routing (OnNavigate, OnLinkClick)
- State management (current URL, history)
- Image loading orchestration
- Error handling dialogs

#### 🖥️ **2. UI Module** (`ui.h` 355 dòng, `ui.cpp` 660 dòng)
**Nhiệm vụ**: Win32 GUI management

**Chức năng**:
- Window creation (CreateWindowEx)
- Controls: Address bar, Go button, Status bar
- Message loop (GetMessage/DispatchMessage)
- Event handling (WM_PAINT, WM_SIZE, WM_COMMAND)
- Custom messages (UIM_IMAGE_LOADED, etc.)

#### 🌐 **3. Network Module** (`network.h` 444 dòng, `network.cpp` 760 dòng)
**Nhiệm vụ**: HTTP communication qua Winsock

**Chức năng**:
- Winsock lifecycle (WSAStartup/WSACleanup)
- HTTP/1.0 client implementation
- Blocking socket I/O
- Response parsing
- Binary-safe data handling
- Error code mapping

#### 📝 **4. Parser Module** (`parser.h` 854 dòng, `parser.cpp` 1007 dòng)
**Nhiệm vụ**: HTML → Structured blocks

**Chức năng**:
- Finite State Machine (11 states)
- Single-pass O(n) parsing
- Forgiving error recovery
- Flat block list output (no DOM tree)
- Entity decoding (&amp; → &)
- Tag/attribute extraction

#### 🎨 **5. Renderer Module** (`renderer.h` 273 dòng, `renderer.cpp` 958 dòng)
**Nhiệm vụ**: GDI rendering engine

**Chức năng**:
- Two-phase (Layout + Paint) architecture
- Double-buffering (flicker-free)
- Font management (H1/H2/P/A caching)
- Image cache (LRU eviction)
- Scrolling support
- Click detection for links

---

## ✨ Tính Năng Nổi Bật

### 🚀 Core Features

✅ **Modern Web Access on Win98** - Browse HTTPS sites từ Win98 VM  
✅ **Asynchronous Image Loading** - Non-blocking, multi-threaded  
✅ **Intelligent Sanitization** - XSS prevention, script removal  
✅ **Professional UI/UX** - Flicker-free, smooth scrolling  
✅ **Robust Error Handling** - Clear messages, graceful degradation  

### 🔧 Technical Highlights

🎯 **Memory Management** - RAII pattern, no leaks, bounded caches  
⚡ **Performance** - Layout caching, clipping, font reuse  
📐 **Modular Design** - Clear separation of concerns  
🔒 **Type Safety** - STRICT handle checking  
📝 **Code Quality** - Comprehensive comments, explicit error codes  

---

## 📂 Cấu Trúc Thư Mục

```
RetroBrowser_Project/
│
├── 📄 README.md                 # Comprehensive documentation (this file)
├── 📄 LICENSE                  # Academic license
│
├── 📁 src/                     # Source code
│   ├── 📁 browser/             # C++ browser (4,817 lines)
│   │   ├── 📁 core/            # Main + PCH (579 + 263 lines)
│   │   │   ├── main.cpp        # Orchestrator, callbacks, image threads
│   │   │   ├── stdafx.h        # Precompiled header
│   │   │   └── stdafx.cpp      # PCH implementation
│   │   │
│   │   ├── 📁 ui/              # Win32 GUI (1,015 lines)
│   │   │   ├── ui.h            # Public API, callbacks (355)
│   │   │   └── ui.cpp          # Windows, controls, events (660)
│   │   │
│   │   ├── 📁 network/         # HTTP client (1,204 lines)
│   │   │   ├── network.h       # API, HttpResponse (444)
│   │   │   └── network.cpp     # Winsock, HTTP parsing (760)
│   │   │
│   │   ├── 📁 parser/          # HTML parser (1,861 lines)
│   │   │   ├── parser.h        # BlockType, API (854)
│   │   │   └── parser.cpp      # FSM, tokenizer (1007)
│   │   │
│   │   └── 📁 renderer/        # GDI engine (1,231 lines)
│   │       ├── renderer.h      # Two-phase API (273)
│   │       └── renderer.cpp    # Layout, paint, cache (958)
│   │
│   └── 📁 proxy/               # Python proxy (1,046 lines)
│       ├── proxy.py            # Server logic (488)
│       ├── config.py           # Configuration (558)
│       └── requirements.txt    # Dependencies
│
├── 📁 tools/                   # Development tools
│   └── 📁 test_scripts/        # Comprehensive test suite
│       ├── test_comprehensive.py  # 24 tests (1,400+ lines)
│       ├── TEST_DOCUMENTATION.md  # Test guide
│       └── README.md           # Quick reference
│
├── 📁 deploy/                  # Build output
│   ├── RetroBrowser.exe        # Compiled binary
│   └── RetroBrowser.pdb        # Debug symbols
│
├── 📁 demo/                    # Test pages
│   ├── image_test.html         # Image demo
│   └── 📁 images/              # Sample BMPs
│
├── 📁 docs/                    # Documentation
│   ├── BaoCao_BaiTapLon.pdf    # Report (Vietnamese)
│   └── architecture_diagram.png
│
└── 📁 libs/                    # External libraries
    └── 📁 libjpeg/             # JPEG support (future)
```

### 📊 Code Statistics

| Component | Files | Lines | Purpose |
|-----------|-------|-------|---------|
| Core | 3 | 842 | Orchestration |
| UI | 2 | 1,015 | GUI management |
| Network | 2 | 1,204 | HTTP client |
| Parser | 2 | 1,861 | HTML → Blocks |
| Renderer | 2 | 1,231 | GDI rendering |
| Proxy | 2 | 1,046 | HTTPS bridge |
| Tests | 8 | 1,600+ | Quality assurance |
| **TOTAL** | **21** | **~8,800** | Production-ready |

---

## 🛠️ Cài Đặt & Triển Khai

### Prerequisites

#### Host Machine
- **OS**: Windows 10/11, Linux, or macOS
- **Python**: 3.7+ with `requests beautifulsoup4`
- **Network**: Internet connection

#### Win98 VM
- **Hypervisor**: VirtualBox or VMware
- **OS**: Windows 98 SE
- **RAM**: 128 MB recommended
- **Network**: Host-Only Adapter

### Quick Start

#### 1. Clone Repository
```bash
git clone https://github.com/Dung2005qk/RetroBrowser_Project.git
cd RetroBrowser_Project
```

#### 2. Install Python Dependencies
```bash
pip install -r src/proxy/requirements.txt
```

#### 3. Start Proxy Server
```bash
python src/proxy/proxy.py

# Output:
# ============================================================
#   Win98 Retro Browser - Intelligent Proxy Starting Up
# ============================================================
# Listening on: 0.0.0.0:8080
# ...
```

#### 4. Build Browser (if needed)
```bash
# Use VS Code task (Ctrl+Shift+B)
# Or compile manually with VC++ 6.0
# Binary available in deploy/RetroBrowser.exe
```

#### 5. Run in Win98 VM
```
1. Copy deploy/RetroBrowser.exe to VM
2. Launch RetroBrowser.exe
3. Enter URL: http://example.com
4. Click "Go" button
```

### Detailed Build Instructions

#### Method 1: VS Code Task (Recommended)
```
1. Open project in VS Code
2. Press Ctrl+Shift+B
3. Select "Build RetroBrowser"
4. Output: deploy/RetroBrowser.exe
```

#### Method 2: Command Line
```cmd
cl.exe /nologo /Fe:deploy\RetroBrowser.exe /Fo:obj\ ^
  /EHsc /MT /Zi /Od /W3 ^
  /D WIN32 /D _WINDOWS /D WINVER=0x0410 ^
  /I src\browser /I src\browser\core ^
  src\browser\core\*.cpp ^
  src\browser\ui\*.cpp ^
  src\browser\network\*.cpp ^
  src\browser\parser\*.cpp ^
  src\browser\renderer\*.cpp ^
  /link /SUBSYSTEM:WINDOWS /MACHINE:X86 ^
  kernel32.lib user32.lib gdi32.lib ws2_32.lib
```

---

## 🚀 Sử Dụng

### Basic Usage

1. **Start proxy** on host: `python src/proxy/proxy.py`
2. **Launch browser** in Win98 VM
3. **Enter URL** in address bar
4. **Click "Go"** or press Enter
5. **Browse**: Click links, scroll pages

### Example URLs

#### ✅ Recommended (Lightweight)
- http://example.com
- http://info.cern.ch
- http://lite.cnn.com
- http://text.npr.org
- https://simple.wikipedia.org

#### ⚠️ Complex (May be slow)
- https://wikipedia.org
- https://news.ycombinator.com

#### ❌ Not Supported
- https://youtube.com (video)
- https://facebook.com (blocked)
- https://google.com (JS-heavy)

### Keyboard Shortcuts

| Key | Action |
|-----|--------|
| Enter | Navigate |
| Tab | Cycle controls |
| Mouse Wheel | Scroll |
| Ctrl+A | Select all (address bar) |
| Alt+F4 | Close |

### Troubleshooting

**"Network Error"**
- Check proxy is running
- Verify VM can ping host IP
- Check firewall allows port 8080

**Page not loading**
- Try simpler URL (example.com)
- Check proxy logs for errors
- Verify internet connection

**Images not showing**
- Only BMP format supported
- Check image URL in logs
- Verify size < 1MB

---

## 📚 Tài Liệu Chi Tiết

### Module Documentation

Each module has detailed documentation in header files:

- **UI Module**: `src/browser/ui/ui.h` - Win32 API usage, callbacks
- **Network**: `src/browser/network/network.h` - Winsock, HTTP protocol
- **Parser**: `src/browser/parser/parser.h` - FSM design, block types
- **Renderer**: `src/browser/renderer/renderer.h` - GDI rendering, layout
- **Proxy**: `src/proxy/config.py` - Configuration options

### Additional Resources

- **Project Report**: `docs/BaoCao_BaiTapLon.pdf` (Vietnamese)
- **Test Documentation**: `tools/test_scripts/TEST_DOCUMENTATION.md`
- **Test Summary**: `TESTING_SUMMARY.md` (24/24 tests passing)
- **Demo README**: `READMEdemo.md` (original overview)

---

## 🧪 Testing

### Comprehensive Test Suite

**File**: `tools/test_scripts/test_comprehensive.py`  
**Coverage**: 24 test cases, 100% passing

#### Test Categories

1. **Basic Functionality** (3 tests)
   - Proxy connectivity
   - Simple HTTP requests
   - HTTP version handling

2. **Edge Cases** (4 tests)
   - Empty/malformed requests
   - Very long URLs
   - Special characters

3. **Security** (4 tests)
   - Script/style removal
   - Dangerous attribute filtering
   - Invalid URL schemes

4. **Content Validation** (3 tests)
   - HTML structure
   - Headers
   - Content-Length

5. **Performance** (5 tests)
   - Response time
   - Sequential requests
   - Concurrent handling
   - Large responses
   - Timeouts

6. **Boundary Conditions** (3 tests)
   - Zero-length responses
   - Unicode URLs
   - Connection reuse

7. **End-to-End** (2 tests)
   - Full cycle
   - Multiple URL formats

### Run Tests

```bash
# Start proxy
python src/proxy/proxy.py

# Run tests (separate terminal)
python tools/test_scripts/test_comprehensive.py

# Output:
# ══════════════════════════════════════════════════════════════
# TEST SUMMARY
# ══════════════════════════════════════════════════════════════
# Total Tests: 24
# Passed: 24 ✅
# Failed: 0
# Success Rate: 100%
# 🎉 ALL TESTS PASSED! 🎉
```

### Test Results

- **Response Time**: 0.43s average
- **Throughput**: 2.4 requests/second
- **Concurrency**: 8-10 simultaneous connections
- **Success Rate**: 100% (24/24 tests)

---

## 🐛 Known Issues & Future Work

### Known Limitations

1. **Image Format**: Only BMP supported (Win98 native)
2. **CSS/JavaScript**: Not supported (by design)
3. **POST Requests**: Only GET implemented
4. **HTTPS Direct**: Proxy handles TLS
5. **Performance**: Slow on complex pages (200MHz CPU)

### Future Enhancements

- [ ] JPEG/PNG support via libjpeg/libpng
- [ ] Thread pool for image loading
- [ ] Connection pooling (HTTP Keep-Alive)
- [ ] Disk cache for images
- [ ] History UI (back/forward buttons)
- [ ] Bookmark management
- [ ] Download manager
- [ ] Print support

---

## 👥 Credits

### Development Team

**Project**: BTL Hệ Điều Hành - Operating Systems Course  
**Institution**: [PTIT]  
**Term**: [D23]

### Technologies Used

- **C++98**: Legacy compatibility
- **Win32 API**: GUI, GDI, Winsock
- **Python 3.x**: Proxy server
- **Requests**: HTTP/HTTPS client
- **BeautifulSoup4**: HTML sanitization
- **Visual C++ 6.0**: Compiler
- **VirtualBox**: Virtualization

### References

- Petzold, Charles. *Programming Windows* (5th Edition)
- Microsoft Win32 API Documentation (MSDN)
- RFC 7230-7235: HTTP/1.1 Specification
- Winsock 2.2 Reference
- GDI Programming Guide

---

## 📜 License

This is an academic project for educational purposes.

**License**: Academic Use Only  
**Copyright**: © 2025 RetroBrowser Project Team  
**Disclaimer**: Not for commercial use

---

<div align="center">

**⭐ Star this repo if you find it useful!**

[![GitHub](https://img.shields.io/badge/GitHub-Dung2005qk%2FRetroBrowser__Project-blue?logo=github)](https://github.com/Dung2005qk/RetroBrowser_Project)

*Made with ❤️ for Operating Systems Course*

</div>
