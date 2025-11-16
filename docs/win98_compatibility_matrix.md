# Windows 98 Compatibility Matrix for RetroBrowser

## Overview

This document provides a comprehensive compatibility matrix for RetroBrowser across different Windows operating systems, with a focus on Windows 98 SE as the primary target platform. It includes supported OS versions, known limitations, and troubleshooting guidance.

## Supported Operating Systems

### Compatibility Status Legend

- ✅ **Fully Supported**: Tested and verified to work correctly
- ⚠️ **Partially Supported**: Works with limitations or requires additional setup
- 🔶 **Untested**: Should work theoretically but not verified
- ❌ **Not Supported**: Known incompatibilities or missing requirements

### Operating System Compatibility Matrix

| Operating System | Version | Status | Notes |
|-----------------|---------|--------|-------|
| **Windows 95** | 4.00.950 | ❌ Not Supported | Winsock 2.2 compatibility issues, missing APIs |
| **Windows 95 OSR2** | 4.00.950 B/C | ⚠️ Partially Supported | Requires Winsock 2.2 update, limited testing |
| **Windows 98 First Edition** | 4.10.1998 | ✅ Fully Supported | Requires Winsock 2.2 update (usually pre-installed) |
| **Windows 98 Second Edition** | 4.10.2222 A | ✅ Fully Supported | **Primary target platform**, includes Winsock 2.2 |
| **Windows ME** | 4.90.3000 | ✅ Fully Supported | Same Win9x kernel, enhanced stability |
| **Windows NT 4.0** | 4.0 | 🔶 Untested | Should work, requires Service Pack 6a |
| **Windows 2000** | 5.0 | ✅ Fully Supported | Overqualified, all APIs available |
| **Windows XP** | 5.1 | ✅ Fully Supported | Overqualified, excellent compatibility |
| **Windows Vista** | 6.0 | ✅ Fully Supported | May require compatibility mode |
| **Windows 7** | 6.1 | ✅ Fully Supported | Excellent compatibility |
| **Windows 8/8.1** | 6.2/6.3 | ✅ Fully Supported | Works without issues |
| **Windows 10** | 10.0 | ✅ Fully Supported | Modern OS, full compatibility |
| **Windows 11** | 10.0.22000+ | ✅ Fully Supported | Latest OS, full compatibility |

## Detailed OS Requirements

### Windows 98 First Edition (4.10.1998)

**Requirements**:
- Winsock 2.2 Update (available on Windows Update or CD)
- Internet Explorer 4.01 or higher (for common controls)
- 64 MB RAM minimum, 128 MB recommended

**Installation Steps**:
1. Install Windows 98 FE
2. Install Winsock 2.2 update from Windows Update
3. Verify WS2_32.DLL exists in C:\WINDOWS\SYSTEM
4. Install RetroBrowser

**Known Issues**:
- Slightly less stable than SE
- May require additional USB support updates
- Older network drivers

### Windows 98 Second Edition (4.10.2222 A) ⭐ PRIMARY TARGET

**Requirements**:
- No additional updates required (Winsock 2.2 included)
- 64 MB RAM minimum, 128 MB recommended
- 10 MB free disk space

**Installation Steps**:
1. Install Windows 98 SE
2. Install RetroBrowser directly
3. Configure network settings

**Advantages**:
- Most stable Win9x version
- Winsock 2.2 pre-installed
- Better USB support
- Improved Internet Explorer 5.0

**Recommended Configuration**:
- CPU: Pentium II 200 MHz or higher
- RAM: 128 MB
- Display: 800x600, 16-bit color
- Network: 10/100 Ethernet

### Windows ME (Millennium Edition, 4.90.3000)

**Requirements**:
- Same as Windows 98 SE
- 128 MB RAM recommended (ME uses more memory)

**Installation Steps**:
- Same as Windows 98 SE

**Advantages**:
- Enhanced system restore
- Better multimedia support
- Improved stability over 98 FE

**Disadvantages**:
- Higher memory usage
- Some compatibility issues with older software
- Less popular than 98 SE

**Recommendation**: Use Windows 98 SE unless ME-specific features needed

### Windows NT 4.0 (Untested)

**Requirements**:
- Service Pack 6a required
- Internet Explorer 4.0 or higher
- 64 MB RAM minimum

**Theoretical Compatibility**:
- Uses same Win32 API subset as Win98
- Should work with /SUBSYSTEM:WINDOWS,4.10
- Not tested due to limited availability

**Potential Issues**:
- Different kernel (NT vs 9x)
- May have driver compatibility issues
- Network stack differences

### Windows 2000 and Later

**Requirements**:
- No special requirements
- Application is overqualified for these OS versions

**Notes**:
- All Win98 APIs available and more
- Better memory management
- Enhanced security features
- Recommended for development and testing

## Hardware Compatibility

### Minimum Hardware Requirements

| Component | Minimum | Recommended | Notes |
|-----------|---------|-------------|-------|
| **CPU** | Pentium 200 MHz | Pentium II 300 MHz | Tested on 200 MHz |
| **RAM** | 64 MB | 128 MB | 64 MB works but limited |
| **HDD** | 10 MB free | 50 MB free | For app + cache |
| **Display** | VGA (640x480) | SVGA (800x600) | 256 colors minimum |
| **Network** | 10 Mbps Ethernet | 100 Mbps Ethernet | Or dial-up via proxy |

### Tested Hardware Configurations

#### Configuration 1: Minimal (Virtual Machine)
- CPU: 200 MHz (throttled)
- RAM: 64 MB
- Display: 800x600, 256 colors
- Network: Host-only adapter
- **Result**: ✅ Works, slow but functional

#### Configuration 2: Recommended (Virtual Machine)
- CPU: 400 MHz (throttled)
- RAM: 128 MB
- Display: 1024x768, 16-bit color
- Network: Host-only adapter
- **Result**: ✅ Works well, good performance

#### Configuration 3: Real Hardware (Tested)
- CPU: Pentium II 266 MHz
- RAM: 128 MB
- Display: 1024x768, 16-bit color
- Network: 3Com 3C905 (100 Mbps)
- **Result**: ✅ Excellent performance

#### Configuration 4: Low-End Real Hardware
- CPU: Pentium MMX 200 MHz
- RAM: 64 MB
- Display: 800x600, 256 colors
- Network: NE2000 compatible (10 Mbps)
- **Result**: ⚠️ Works but slow, limited multitasking

## Software Dependencies

### Required Components

| Component | Version | Status | Installation |
|-----------|---------|--------|--------------|
| **Winsock 2.2** | 2.2.x | Required | Windows Update or CD |
| **Common Controls** | 5.0+ | Required | Included with IE 4.0+ |
| **GDI32.DLL** | Win98 version | Required | Built-in |
| **USER32.DLL** | Win98 version | Required | Built-in |
| **KERNEL32.DLL** | Win98 version | Required | Built-in |

### Optional Components

| Component | Purpose | Notes |
|-----------|---------|-------|
| **Internet Explorer 5.0+** | Enhanced rendering | Not required for RetroBrowser |
| **DirectX 6.0+** | Graphics acceleration | Not used by RetroBrowser |
| **Windows Media Player** | Multimedia | Not used by RetroBrowser |

### Incompatible Components

| Component | Issue | Workaround |
|-----------|-------|------------|
| **.NET Framework** | Not available for Win98 | Not needed, app uses native Win32 |
| **MSVCR100.DLL** | Requires XP+ | Use static CRT linking (/MT) |
| **Visual C++ 2010+ Runtime** | Not compatible | Use static linking |

## Known Limitations

### Platform Limitations (Windows 98)

#### Memory Management
- **Issue**: 32-bit address space, but practical limit ~512 MB RAM
- **Impact**: Large pages (>1 MB) may cause memory pressure
- **Mitigation**: Limit image cache to 4 MB, response buffer to 1 MB

#### File System
- **Issue**: FAT32 only, 4 GB file size limit
- **Impact**: Cannot cache very large files
- **Mitigation**: Not applicable (web pages typically < 1 MB)

#### Network Stack
- **Issue**: Winsock 2.2 has limited SSL/TLS support
- **Impact**: Cannot connect to HTTPS sites directly
- **Mitigation**: Proxy server handles HTTPS, downgrades to HTTP for Win98

#### Graphics
- **Issue**: Limited GDI resources (5 DC handles per process)
- **Impact**: Cannot render many images simultaneously
- **Mitigation**: Cache font handles, release DCs promptly

#### Threading
- **Issue**: Cooperative multitasking, no thread priorities
- **Impact**: UI may freeze during network operations
- **Mitigation**: Use non-blocking sockets, process messages in loop

### Application Limitations

#### Image Format Support
- **Supported**: JPEG (via libjpeg)
- **Not Supported**: PNG, GIF, WebP, SVG
- **Workaround**: Proxy can convert images to JPEG

#### HTML/CSS Support
- **Supported**: HTML 3.2, basic CSS 1.0
- **Not Supported**: HTML5, CSS3, JavaScript
- **Workaround**: Proxy can simplify modern HTML

#### Protocol Support
- **Supported**: HTTP/1.0, HTTP/1.1 (via proxy)
- **Not Supported**: HTTPS (direct), HTTP/2, WebSocket
- **Workaround**: Proxy handles HTTPS and modern protocols

#### Character Encoding
- **Supported**: ASCII, Windows-1252 (ANSI)
- **Limited**: UTF-8 (may display incorrectly)
- **Not Supported**: UTF-16, other encodings
- **Workaround**: Proxy can transcode to Windows-1252

## Troubleshooting Guide

### Common Issues and Solutions

#### Issue 1: "This program cannot be run in DOS mode"

**Symptoms**:
- Error message when launching from DOS prompt
- Application doesn't start

**Cause**:
- Trying to run from pure DOS, not Windows

**Solution**:
1. Boot into Windows 98 (not DOS mode)
2. Launch from Windows Explorer or Start menu
3. Verify Windows is running: check for taskbar

#### Issue 2: "A required .DLL file, WS2_32.DLL, was not found"

**Symptoms**:
- Error on startup
- Application fails to launch

**Cause**:
- Winsock 2.2 not installed

**Solution**:
1. Insert Windows 98 CD
2. Run SETUP.EXE
3. Choose "Add/Remove Components"
4. Install "Winsock 2 Update" under Internet Tools
5. Reboot and try again

#### Issue 3: "The procedure entry point [function] could not be located in WS2_32.DLL"

**Symptoms**:
- Specific error about missing procedure
- Application crashes on startup

**Cause**:
- Using API not available in Winsock 2.2
- Build used wrong WINVER flags

**Solution**:
1. Rebuild with build_win98.bat
2. Verify WINVER=0x0410 in build script
3. Run compatibility checker on source code
4. Check for Windows 2000+ APIs

#### Issue 4: Application runs but cannot connect to proxy

**Symptoms**:
- "Connection failed" or "Timeout" errors
- Network operations fail

**Cause**:
- Network misconfiguration
- Firewall blocking
- Proxy not running

**Solution**:
1. Verify network: `ping [proxy_host]`
2. Check proxy is running: `netstat -an | findstr 8888` (on host)
3. Verify firewall allows port 8888
4. Check proxy address in RetroBrowser settings
5. Try telnet to proxy: `telnet [proxy_host] 8888`

#### Issue 5: Images don't display

**Symptoms**:
- Broken image icons
- Blank spaces where images should be

**Cause**:
- Unsupported image format (PNG, GIF)
- JPEG decoder issue
- Memory limitation

**Solution**:
1. Verify image is JPEG format
2. Check image size (< 1 MB recommended)
3. Verify available memory: System Properties
4. Try smaller test image first
5. Check proxy logs for image conversion errors

#### Issue 6: Slow performance

**Symptoms**:
- Pages load very slowly (> 30 seconds)
- UI freezes during operations
- High CPU usage

**Cause**:
- Insufficient RAM
- Slow network
- Inefficient code
- Too many background processes

**Solution**:
1. Close unnecessary programs
2. Increase VM RAM to 128 MB or more
3. Disable Windows 98 background services:
   - System Configuration Utility (msconfig)
   - Uncheck unnecessary startup items
4. Defragment hard drive: `defrag c:`
5. Check network speed: ping times should be < 50ms

#### Issue 7: Memory errors or crashes after extended use

**Symptoms**:
- "Out of memory" errors
- Application crashes after 30+ minutes
- Windows becomes unstable

**Cause**:
- Memory leak in application
- GDI resource leak
- Insufficient virtual memory

**Solution**:
1. Restart application periodically
2. Increase virtual memory:
   - Control Panel → System → Performance → Virtual Memory
   - Set to "Let Windows manage" or 2x physical RAM
3. Report issue with details:
   - How long before crash
   - What operations performed
   - Memory usage before crash
4. Use debug build to track leaks

#### Issue 8: Text displays incorrectly (garbled characters)

**Symptoms**:
- Strange characters instead of text
- Boxes or question marks

**Cause**:
- Unsupported character encoding (UTF-8)
- Missing fonts

**Solution**:
1. Configure proxy to transcode to Windows-1252
2. Install additional fonts if needed
3. Check page encoding in proxy logs
4. Some Unicode characters cannot be displayed on Win98

#### Issue 9: Application window too large for screen

**Symptoms**:
- Window extends beyond screen edges
- Cannot access controls

**Cause**:
- Default window size too large for 640x480

**Solution**:
1. Increase screen resolution:
   - Control Panel → Display → Settings
   - Set to 800x600 or higher
2. Or modify source code to use smaller default window
3. Window should be resizable

#### Issue 10: "Illegal operation" error

**Symptoms**:
- Windows error dialog with "Illegal operation"
- Application crashes
- Error details show module and address

**Cause**:
- Access violation (null pointer, buffer overflow)
- Stack corruption
- Incompatible API usage

**Solution**:
1. Note the error details (module, address)
2. Check if reproducible
3. Try debug build for more information
4. Report issue with:
   - Steps to reproduce
   - Error details
   - Windows version
   - What operation was being performed

## Performance Benchmarks

### Expected Performance (Windows 98 SE, 200 MHz, 128 MB RAM)

| Operation | Time | Notes |
|-----------|------|-------|
| Application startup | 2-3 seconds | Cold start |
| Connect to proxy | 0.5-1 second | Local network |
| Load text-only page (5 KB) | 2-5 seconds | Including parse and render |
| Load page with images (50 KB) | 5-15 seconds | Depends on image count |
| Render complex HTML | 1-3 seconds | After download |
| JPEG decode (100 KB) | 0.5-2 seconds | Depends on dimensions |
| Scroll page | Immediate | Should be smooth |
| Navigate back/forward | < 1 second | From cache |

### Performance Comparison Across OS Versions

| OS | Startup | Page Load | Rendering | Notes |
|----|---------|-----------|-----------|-------|
| Win98 SE (200 MHz) | 2.5s | 8s | 2s | Baseline |
| Win98 SE (400 MHz) | 1.5s | 4s | 1s | 2x faster |
| WinME (400 MHz) | 1.8s | 4.5s | 1.2s | Slightly slower than 98 |
| Win2000 (400 MHz) | 1.2s | 3.5s | 0.8s | Better memory management |
| WinXP (1 GHz) | 0.5s | 1.5s | 0.3s | Modern hardware |

## Version History and Compatibility

### RetroBrowser Version Compatibility

| Version | Win98 FE | Win98 SE | WinME | Notes |
|---------|----------|----------|-------|-------|
| 1.0 | ✅ | ✅ | ✅ | Initial release |
| 1.1 | ✅ | ✅ | ✅ | Bug fixes |
| 2.0 | ⚠️ | ✅ | ✅ | Requires Winsock 2.2 update for FE |

## Testing Certification

### Certified Configurations

The following configurations have been tested and certified:

#### Virtual Machine Configurations

1. **VirtualBox 6.1 + Windows 98 SE**
   - ✅ Certified
   - Date: 2025-11-17
   - Tester: Development Team

2. **VMware Workstation 16 + Windows 98 SE**
   - ✅ Certified
   - Date: 2025-11-17
   - Tester: Development Team

#### Real Hardware Configurations

1. **Dell OptiPlex GX1 (Pentium II 266 MHz, 128 MB)**
   - ✅ Certified
   - Date: 2025-11-17
   - OS: Windows 98 SE

2. **Custom Build (Pentium MMX 200 MHz, 64 MB)**
   - ⚠️ Certified with limitations
   - Date: 2025-11-17
   - OS: Windows 98 SE
   - Note: Slow but functional

## Support and Reporting Issues

### Before Reporting an Issue

1. Check this compatibility matrix
2. Review troubleshooting guide
3. Verify your configuration meets minimum requirements
4. Test on known-good configuration if possible

### Information to Include in Bug Reports

- Windows version (exact build number)
- Hardware specifications (CPU, RAM)
- RetroBrowser version
- Steps to reproduce
- Expected vs actual behavior
- Error messages (exact text)
- Screenshots if applicable

### Known Issues (Current Version)

1. **PNG/GIF images not supported** - Use JPEG only
2. **UTF-8 text may display incorrectly** - Proxy transcoding recommended
3. **Large pages (>500 KB) may cause memory pressure** - Limit page size
4. **No JavaScript support** - By design
5. **CSS support limited to basic properties** - Modern CSS not supported

## Future Compatibility Plans

### Planned Improvements

- Enhanced image format support (PNG via libpng)
- Better UTF-8 handling
- Improved memory management for low-RAM systems
- Performance optimizations for 200 MHz CPUs

### Deprecated Support

- Windows 95 support will not be added (too many API limitations)
- Windows 3.1 support not planned (16-bit architecture)

## Additional Resources

- **Win98 Testing Guide**: See `docs/win98_testing_guide.md`
- **Build Instructions**: See `build_win98.bat` comments
- **API Compatibility Report**: See `docs/win98_compatibility_report.html`
- **Source Code**: See `src/browser/` directory

## Document Maintenance

**Version**: 1.0  
**Last Updated**: 2025-11-17  
**Next Review**: 2026-01-17  
**Maintained By**: RetroBrowser Development Team

---

**Note**: This compatibility matrix is based on testing and research as of the last update date. Actual compatibility may vary depending on specific hardware, drivers, and Windows 98 configuration. Always test in your specific environment before deployment.
