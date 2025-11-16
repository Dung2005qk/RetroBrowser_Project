# Windows 98 Testing Guide for RetroBrowser

## Overview

This guide provides step-by-step instructions for setting up a Windows 98 virtual machine and testing RetroBrowser to ensure full compatibility with Windows 98 SE hardware and software constraints.

## Prerequisites

- Host machine with virtualization support (Intel VT-x or AMD-V)
- At least 4GB RAM available for host + VM
- Windows 98 SE installation media (ISO or physical CD)
- VirtualBox 6.1+ or VMware Workstation/Player

## Part 1: Windows 98 VM Setup

### Option A: VirtualBox Setup

#### 1.1 Create New Virtual Machine

1. Open VirtualBox and click "New"
2. Configure VM settings:
   - **Name**: Windows 98 SE
   - **Type**: Microsoft Windows
   - **Version**: Windows 98
   - **Memory**: 128 MB (minimum) or 256 MB (recommended)
   - **Hard Disk**: Create new, VDI format, 2 GB

#### 1.2 Configure VM Settings

Before starting the VM, adjust these settings:

**System Settings:**
- **Motherboard Tab**:
  - Boot Order: Floppy, CD, Hard Disk
  - Chipset: PIIX3
  - Enable I/O APIC: Unchecked
  - Hardware Clock in UTC: Unchecked
  
- **Processor Tab**:
  - Processors: 1 CPU
  - Execution Cap: 100%
  
- **Acceleration Tab**:
  - Paravirtualization: None
  - Enable VT-x/AMD-V: Checked
  - Enable Nested Paging: Checked

**Display Settings:**
- Video Memory: 16 MB
- Graphics Controller: VBoxVGA
- Enable 3D Acceleration: Unchecked

**Storage Settings:**
- IDE Controller: PIIX4
- Attach Windows 98 SE ISO to CD/DVD drive

**Network Settings:**
- Adapter 1: Enable Network Adapter
- Attached to: Host-only Adapter
- Adapter Type: PCnet-PCI II (Am79C970A)
- Promiscuous Mode: Allow All

**Audio Settings:**
- Enable Audio: Checked
- Audio Controller: SB16
- Audio Driver: (host default)

#### 1.3 Install Windows 98 SE

1. Start the VM and boot from CD
2. Choose "Boot from CD-ROM" at startup menu
3. At the Windows 98 Setup screen:
   - Press Enter to continue
   - Choose "Format the hard disk" (FAT32)
   - Follow installation wizard
   - Skip network setup (configure later)
   - Install typical components

4. After installation, install VirtualBox Guest Additions:
   - Devices → Insert Guest Additions CD
   - Run setup from D:\SETUP.EXE
   - Reboot after installation

### Option B: VMware Setup

#### 1.1 Create New Virtual Machine

1. Open VMware and select "Create a New Virtual Machine"
2. Choose "Custom (advanced)" configuration
3. Configure settings:
   - **Guest OS**: Microsoft Windows → Windows 98
   - **Memory**: 128-256 MB
   - **Network**: Host-only networking
   - **Disk**: 2 GB, single file
   - **Disk Type**: IDE

#### 1.2 Edit VM Settings

- **Hardware Compatibility**: Workstation 8.x or earlier
- **Processors**: 1 core
- **Display**: Auto-detect, 16 MB video memory
- **Network Adapter**: Host-only, AMD PCNet32

#### 1.3 Install Windows 98 SE

Follow similar steps as VirtualBox installation above.

## Part 2: Network Configuration

### 2.1 Configure Host-Only Network

**On Host Machine (VirtualBox):**

1. File → Host Network Manager
2. Create or select vboxnet0
3. Configure IPv4:
   - Address: 192.168.56.1
   - Mask: 255.255.255.0
4. Enable DHCP Server:
   - Server Address: 192.168.56.100
   - Lower Bound: 192.168.56.101
   - Upper Bound: 192.168.56.254

**On Host Machine (VMware):**

1. Edit → Virtual Network Editor
2. Select VMnet1 (Host-only)
3. Configure:
   - Subnet IP: 192.168.137.0
   - Subnet Mask: 255.255.255.0
   - Use local DHCP: Enabled

### 2.2 Configure Windows 98 Network

1. Right-click "Network Neighborhood" → Properties
2. Ensure these components are installed:
   - Client for Microsoft Networks
   - AMD PCNET Family Ethernet Adapter
   - TCP/IP → AMD PCNET Family
   
3. Select "TCP/IP → AMD PCNET" and click Properties:
   - **IP Address Tab**: Obtain automatically (DHCP)
   - **DNS Configuration Tab**: Disable DNS
   - **Gateway Tab**: (leave empty)
   - **WINS Configuration Tab**: Disable WINS

4. Click OK and reboot

5. After reboot, verify network:
   - Start → Run → command
   - Type: `ipconfig`
   - Should show IP like 192.168.56.xxx or 192.168.137.xxx

### 2.3 Test Host-VM Connectivity

**On Windows 98:**
```
ping 192.168.56.1
```
(or 192.168.137.1 for VMware)

Should receive replies. If not, check firewall on host.

## Part 3: Installing RetroBrowser on Windows 98

### 3.1 Transfer Binary to VM

**Method 1: Shared Folder (VirtualBox)**
1. VM Settings → Shared Folders
2. Add folder containing RetroBrowser_Win98.exe
3. In Windows 98, access via Network Neighborhood

**Method 2: Shared Folder (VMware)**
1. VM → Settings → Options → Shared Folders
2. Add folder and enable
3. Access via Network Neighborhood

**Method 3: Simple HTTP Server**
1. On host, in RetroBrowser deploy folder:
   ```bash
   python -m http.server 8080
   ```
2. On Windows 98, use Internet Explorer:
   - Navigate to http://192.168.56.1:8080
   - Download RetroBrowser_Win98.exe

### 3.2 Install Required Components

**Verify Winsock 2.2:**
1. Check for C:\WINDOWS\SYSTEM\WS2_32.DLL
2. If missing, install from Windows 98 CD:
   - Run SETUP.EXE
   - Add/Remove Components
   - Select "Internet Tools" → "Winsock 2 Update"

**Verify Visual C++ Runtime (if needed):**
- RetroBrowser uses static linking (/MT), so no MSVCRT.DLL needed
- If errors occur, check for MSVCRT.DLL in C:\WINDOWS\SYSTEM

### 3.3 Create Installation Directory

1. Create folder: C:\RetroBrowser
2. Copy RetroBrowser_Win98.exe to this folder
3. Create desktop shortcut (optional)

## Part 4: Setting Up the Proxy Server

### 4.1 Install Python on Host

RetroBrowser requires a proxy server running on the host machine.

1. Install Python 3.8+ on host
2. Navigate to RetroBrowser src/proxy directory
3. Install dependencies:
   ```bash
   pip install -r requirements.txt
   ```

### 4.2 Configure Proxy

Edit `src/proxy/config.py`:
```python
PROXY_HOST = '0.0.0.0'  # Listen on all interfaces
PROXY_PORT = 8888
```

### 4.3 Start Proxy Server

```bash
cd src/proxy
python proxy.py
```

Should see: "Proxy server listening on 0.0.0.0:8888"

### 4.4 Configure RetroBrowser to Use Proxy

On Windows 98, when launching RetroBrowser:
- Proxy address: 192.168.56.1:8888 (VirtualBox)
- Or: 192.168.137.1:8888 (VMware)

## Part 5: Test Cases

### Test Case 1: Application Launch

**Objective**: Verify binary starts without errors

**Steps**:
1. Double-click RetroBrowser_Win98.exe
2. Observe startup

**Expected Result**:
- Application window appears within 2-3 seconds
- No error dialogs about missing procedures or DLLs
- Window title shows "RetroBrowser"

**Failure Indicators**:
- "Procedure not found in ws2_32.dll" → Winsock 2.2 not installed
- "MSVCR100.dll not found" → CRT not statically linked (build error)
- Application crashes immediately → Incompatible API usage

### Test Case 2: UI Functionality

**Objective**: Verify user interface works correctly

**Steps**:
1. Launch RetroBrowser
2. Check address bar is visible and editable
3. Check navigation buttons (Back, Forward, Stop, Refresh)
4. Resize window
5. Minimize and restore window

**Expected Result**:
- All UI elements render correctly
- Address bar accepts text input
- Buttons are clickable (even if disabled)
- Window resizes smoothly
- No visual artifacts

### Test Case 3: Network Connectivity

**Objective**: Verify proxy connection works

**Steps**:
1. Ensure proxy is running on host
2. Launch RetroBrowser
3. Enter URL: http://example.com
4. Click Go or press Enter

**Expected Result**:
- Status bar shows "Connecting..."
- Connection succeeds within 5 seconds
- Page content loads
- No network timeout errors

**Failure Indicators**:
- "Cannot connect to proxy" → Check network configuration
- Timeout after 30 seconds → Firewall blocking port 8888
- "Winsock error" → Winsock 2.2 issue

### Test Case 4: HTML Rendering

**Objective**: Verify HTML parser and renderer work

**Steps**:
1. Load simple test page: http://example.com
2. Verify text displays
3. Load page with formatting: http://info.cern.ch
4. Check bold, italic, headings render

**Expected Result**:
- Text content visible and readable
- Basic formatting applied (bold, italic)
- Headings use larger font
- Links are underlined
- No rendering crashes

### Test Case 5: Image Loading

**Objective**: Verify JPEG image support

**Steps**:
1. Navigate to page with images
2. Wait for images to load
3. Scroll to view all images

**Expected Result**:
- JPEG images decode and display
- No "broken image" icons
- Images scale appropriately
- No memory errors

**Known Limitation**:
- PNG images may not display (JPEG only currently)

### Test Case 6: Navigation

**Objective**: Verify browsing history works

**Steps**:
1. Load page A
2. Click link to page B
3. Click Back button
4. Click Forward button
5. Enter new URL (page C)
6. Click Back twice

**Expected Result**:
- Back button returns to previous page
- Forward button works after Back
- History stack maintains correct order
- Address bar updates with each navigation

### Test Case 7: Stability Test

**Objective**: Verify no memory leaks or crashes

**Steps**:
1. Launch RetroBrowser
2. Browse 10-15 different pages
3. Scroll through pages
4. Resize window multiple times
5. Leave running for 30 minutes

**Expected Result**:
- No crashes or freezes
- Memory usage stays under 32 MB
- UI remains responsive
- No "Out of memory" errors

**Monitor**:
- Use Ctrl+Alt+Del → Close Program to check if app is "Not Responding"
- Check available memory in System Properties

### Test Case 8: Error Handling

**Objective**: Verify graceful error handling

**Steps**:
1. Enter invalid URL: "notaurl"
2. Enter unreachable URL: "http://192.168.56.99"
3. Stop proxy server mid-load
4. Enter very long URL (500+ characters)

**Expected Result**:
- Invalid URL shows error message
- Unreachable URL times out gracefully
- Proxy disconnect shows connection error
- Long URL handled without crash

### Test Case 9: Performance Test

**Objective**: Measure page load performance

**Steps**:
1. Clear any cache
2. Load http://example.com
3. Time from Enter to page fully rendered
4. Repeat 3 times, average results

**Expected Result**:
- Simple page loads in < 5 seconds
- Complex page loads in < 15 seconds
- No UI freezing during load

**Acceptable Performance** (on 200MHz CPU):
- Text-only page: 2-5 seconds
- Page with images: 5-15 seconds
- Large page (100KB+): 15-30 seconds

### Test Case 10: Resource Limits

**Objective**: Test behavior under low resources

**Steps**:
1. Configure VM with 64 MB RAM
2. Launch RetroBrowser
3. Load multiple pages
4. Monitor for memory errors

**Expected Result**:
- Application launches successfully
- Basic browsing works
- May show "Low memory" warnings from Windows
- Should not crash, but may slow down

## Part 6: Troubleshooting

### Issue: "Procedure not found in WS2_32.DLL"

**Cause**: Winsock 2.2 not installed or incompatible API used

**Solution**:
1. Install Winsock 2.2 update from Windows 98 CD
2. If persists, run compatibility checker on source code
3. Verify build used correct WINVER flags

### Issue: Application crashes on startup

**Cause**: Incompatible API or wrong subsystem version

**Solution**:
1. Check PE header: `dumpbin /headers RetroBrowser_Win98.exe`
2. Verify subsystem version is 4.10
3. Rebuild with build_win98.bat

### Issue: Cannot connect to proxy

**Cause**: Network misconfiguration or firewall

**Solution**:
1. Verify VM has IP address: `ipconfig`
2. Ping host from VM: `ping 192.168.56.1`
3. Check host firewall allows port 8888
4. Verify proxy is running: `netstat -an | findstr 8888`

### Issue: Images don't display

**Cause**: JPEG decoder issue or memory limitation

**Solution**:
1. Check image format (only JPEG supported)
2. Verify libjpeg linked correctly
3. Check available memory (need ~4MB for image cache)
4. Try smaller images first

### Issue: Slow performance

**Cause**: VM resource constraints or inefficient code

**Solution**:
1. Increase VM RAM to 256 MB
2. Disable unnecessary Windows 98 services
3. Use /O2 optimization in build
4. Profile code to find bottlenecks

### Issue: Memory errors after extended use

**Cause**: Memory leak in application

**Solution**:
1. Monitor memory usage over time
2. Check for unreleased GDI objects
3. Review network buffer management
4. Use debug build to track allocations

## Part 7: Performance Benchmarking

### Benchmark Setup

**Test Hardware Profile**:
- CPU: 200 MHz (VM throttled)
- RAM: 128 MB
- Network: 10 Mbps simulated

**Test Pages**:
1. Minimal: Plain text, < 1 KB
2. Simple: Text + formatting, ~5 KB
3. Standard: Text + images, ~50 KB
4. Complex: Multiple images, ~200 KB

### Metrics to Collect

1. **Startup Time**: Launch to window visible
2. **Connection Time**: Enter URL to first byte
3. **Parse Time**: First byte to DOM ready
4. **Render Time**: DOM ready to screen paint
5. **Total Load Time**: Enter URL to fully rendered
6. **Memory Usage**: Before and after page load

### Recording Results

Create log file: C:\RetroBrowser\benchmark.txt

```
Test Date: [date]
VM: VirtualBox 6.1
OS: Windows 98 SE (4.10.2222 A)
CPU: 200 MHz
RAM: 128 MB

Test 1: Minimal Page (http://example.com)
- Startup: 2.1s
- Connection: 0.8s
- Parse: 0.2s
- Render: 0.3s
- Total: 3.4s
- Memory: 8 MB → 10 MB

[Continue for each test page...]
```

## Part 8: Regression Testing

### When to Run Regression Tests

- After any code changes to network, parser, or renderer
- After updating build flags
- Before releasing new version
- When adding new features

### Regression Test Suite

Run all 10 test cases from Part 5 and verify:
- No new failures introduced
- Performance not degraded > 10%
- Memory usage not increased > 20%
- All existing functionality still works

### Automated Testing (Optional)

For advanced users, consider:
1. Scripting VM snapshots for quick resets
2. Using AutoIt or similar to automate UI testing
3. Capturing screenshots for visual comparison
4. Logging results to file for analysis

## Appendix A: Windows 98 SE Specifications

**Minimum Requirements**:
- CPU: 486DX 66 MHz
- RAM: 16 MB
- HDD: 500 MB
- Display: VGA

**Recommended Requirements**:
- CPU: Pentium 200 MHz
- RAM: 64 MB
- HDD: 2 GB
- Display: SVGA

**RetroBrowser Target**:
- CPU: Pentium 200 MHz
- RAM: 128 MB (64 MB minimum)
- HDD: 10 MB for application
- Display: 800x600, 256 colors minimum

## Appendix B: Useful Windows 98 Commands

```
ipconfig          - Show network configuration
ping [host]       - Test network connectivity
netstat -an       - Show network connections
mem               - Show memory usage
scandisk          - Check disk for errors
defrag            - Defragment hard drive
msconfig          - System configuration utility
regedit           - Registry editor
```

## Appendix C: VM Snapshot Strategy

**Recommended Snapshots**:

1. **Fresh Install**: Clean Windows 98 SE after installation
2. **Configured**: Network configured, Winsock 2.2 installed
3. **With RetroBrowser**: Application installed and tested
4. **Baseline**: Known good state for regression testing

**Usage**:
- Restore "Baseline" before each test session
- Create new snapshot after successful tests
- Keep "Fresh Install" for major reconfigurations

## Appendix D: Additional Resources

- Windows 98 SE Documentation: Microsoft Archive
- VirtualBox Manual: https://www.virtualbox.org/manual/
- VMware Documentation: https://docs.vmware.com/
- Win32 API Reference (Win98): MSDN Library (archived)
- Winsock 2.2 Specification: Microsoft Docs

---

**Document Version**: 1.0  
**Last Updated**: 2025-11-17  
**Maintained By**: RetroBrowser Development Team
