# Windows 98 VM Testing Instructions

**Purpose:** Test RetroBrowser_Win98.exe on actual Windows 98 VM  
**Prerequisites:** VirtualBox or VMware with Windows 98 SE installed  
**Estimated Time:** 30-45 minutes

---

## Prerequisites Checklist

Before starting, ensure you have:

- [ ] Windows 98 SE VM installed and running
- [ ] VM network configured (host-only adapter recommended)
- [ ] Winsock 2.2 installed on Win98 (included in SE, update needed for FE)
- [ ] At least 64MB RAM allocated to VM
- [ ] Guest Additions or VMware Tools installed (for file sharing)

---

## Step 1: Prepare Files for Transfer

### On Host Machine (Windows 10/11)

1. **Locate the Win98 binary:**
   ```
   C:\Users\LMC\OneDrive - THS\Desktop\RetroBrowser_Project\deploy\RetroBrowser_Win98.exe
   ```

2. **Create deployment package:**
   - Copy `RetroBrowser_Win98.exe` to a shared folder
   - Or create a floppy disk image (if VM uses floppy)
   - Or use network file sharing

3. **Start proxy server (for network testing):**
   ```batch
   cd C:\Users\LMC\OneDrive - THS\Desktop\RetroBrowser_Project
   python src/proxy/proxy.py
   ```
   - Proxy will listen on port 8080
   - Note your host machine's IP address

---

## Step 2: Transfer Binary to Win98 VM

### Method A: Shared Folder (Recommended)

1. In VirtualBox:
   - VM Settings → Shared Folders
   - Add folder: `C:\Users\LMC\OneDrive - THS\Desktop\RetroBrowser_Project\deploy`
   - Name: `RetroBrowser`
   - Auto-mount: Yes

2. In Win98 VM:
   - Open Network Neighborhood
   - Navigate to `\\VBOXSVR\RetroBrowser` (VirtualBox)
   - Copy `RetroBrowser_Win98.exe` to `C:\RetroBrowser\`

### Method B: Network Share

1. On host, share the deploy folder
2. In Win98, map network drive to `\\HOST_IP\deploy`
3. Copy exe to local drive

### Method C: Floppy/CD Image

1. Create ISO with binary
2. Mount ISO in VM
3. Copy from CD-ROM drive

---

## Step 3: Initial Launch Test

### Test 3.1: Basic Execution

1. **Navigate to binary location:**
   ```
   C:\RetroBrowser\RetroBrowser_Win98.exe
   ```

2. **Double-click to launch**

3. **Expected results:**
   - ✅ Application launches without errors
   - ✅ Main window appears
   - ✅ No "procedure not found" errors
   - ✅ No missing DLL errors

4. **If errors occur:**
   - Note exact error message
   - Check if Winsock 2.2 is installed: `C:\Windows\System\ws2_32.dll`
   - Verify all required DLLs exist

### Test 3.2: Window Display

1. **Check main window:**
   - [ ] Window title displays correctly
   - [ ] Menu bar visible
   - [ ] Toolbar visible
   - [ ] Status bar visible
   - [ ] Address bar visible

2. **Check UI responsiveness:**
   - [ ] Window can be moved
   - [ ] Window can be resized
   - [ ] Menus can be opened
   - [ ] Buttons respond to clicks

---

## Step 4: Network Connectivity Test

### Test 4.1: Configure Proxy

1. **In RetroBrowser, open Settings/Options**

2. **Configure proxy settings:**
   - Proxy enabled: Yes
   - Proxy host: `<HOST_IP>` (e.g., 192.168.56.1)
   - Proxy port: 8080

3. **Save settings**

### Test 4.2: Basic HTTP Request

1. **In address bar, enter:**
   ```
   http://example.com
   ```

2. **Expected results:**
   - ✅ Request goes through proxy
   - ✅ Page loads (or shows parsing)
   - ✅ No network errors
   - ✅ Status bar shows progress

3. **Check proxy server logs on host:**
   - Should see incoming connection from Win98 VM
   - Should see HTTP request for example.com

### Test 4.3: Multiple Requests

1. **Test several URLs:**
   - `http://example.com`
   - `http://info.cern.ch` (first website, simple HTML)
   - `http://motherfuckingwebsite.com` (minimal HTML)

2. **Verify:**
   - [ ] All requests complete
   - [ ] No crashes
   - [ ] Memory usage stable

---

## Step 5: Stability Tests

### Test 5.1: Memory Usage

1. **Before launching RetroBrowser:**
   - Note available memory (System Properties)

2. **After launching:**
   - Check memory usage
   - Should use < 10MB

3. **After loading pages:**
   - Memory should not continuously increase
   - No memory leaks

### Test 5.2: Extended Runtime

1. **Leave RetroBrowser running for 10 minutes**

2. **Perform various actions:**
   - Navigate to different pages
   - Open/close menus
   - Resize window
   - Minimize/restore

3. **Expected results:**
   - ✅ No crashes
   - ✅ No freezes
   - ✅ Stable memory usage
   - ✅ Responsive UI

### Test 5.3: Stress Test

1. **Rapidly perform actions:**
   - Click buttons quickly
   - Open multiple menus
   - Resize window repeatedly

2. **Expected results:**
   - ✅ No crashes
   - ✅ UI remains responsive
   - ✅ No error dialogs

---

## Step 6: Error Handling Tests

### Test 6.1: Invalid URL

1. **Enter invalid URL:**
   ```
   http://this-domain-does-not-exist-12345.com
   ```

2. **Expected results:**
   - ✅ Error message displayed
   - ✅ Application doesn't crash
   - ✅ Can continue using browser

### Test 6.2: Network Disconnection

1. **Disconnect VM network**

2. **Try to load a page**

3. **Expected results:**
   - ✅ Timeout error displayed
   - ✅ Application doesn't crash
   - ✅ Can reconnect and retry

### Test 6.3: Proxy Unavailable

1. **Stop proxy server on host**

2. **Try to load a page**

3. **Expected results:**
   - ✅ Connection error displayed
   - ✅ Application doesn't crash

---

## Step 7: Performance Tests

### Test 7.1: Launch Time

1. **Measure time from double-click to window display**

2. **Expected:**
   - < 5 seconds on 200MHz CPU
   - < 2 seconds on faster CPU

### Test 7.2: Page Load Time

1. **Load simple page (example.com)**

2. **Measure time from Enter to page display**

3. **Expected:**
   - < 10 seconds on 200MHz CPU with 56K modem simulation

### Test 7.3: UI Responsiveness

1. **Click menu items**

2. **Expected:**
   - Immediate response (< 100ms)
   - No lag or freezing

---

## Step 8: Compatibility Verification

### Test 8.1: DLL Dependencies

1. **Use Dependency Walker (if available on Win98):**
   ```
   depends.exe RetroBrowser_Win98.exe
   ```

2. **Verify all DLLs found:**
   - [ ] KERNEL32.dll
   - [ ] USER32.dll
   - [ ] GDI32.dll
   - [ ] COMCTL32.dll
   - [ ] WS2_32.dll

### Test 8.2: System Compatibility

1. **Check Windows version:**
   - Right-click My Computer → Properties
   - Should show Windows 98 (4.10.2222 A or similar)

2. **Verify binary runs on this version**

---

## Test Results Template

### Environment Information

```
Date: _______________
VM Software: VirtualBox / VMware / Other: _______________
Windows Version: _______________
CPU Speed: _______________
RAM: _______________
Network: Host-only / NAT / Bridged
```

### Test Results Summary

| Test | Status | Notes |
|------|--------|-------|
| Basic Launch | ✅ / ❌ | |
| Window Display | ✅ / ❌ | |
| Network Connectivity | ✅ / ❌ | |
| Proxy Connection | ✅ / ❌ | |
| Page Loading | ✅ / ❌ | |
| Memory Stability | ✅ / ❌ | |
| Extended Runtime | ✅ / ❌ | |
| Error Handling | ✅ / ❌ | |
| Performance | ✅ / ❌ | |

### Issues Found

```
1. _______________________________________________
2. _______________________________________________
3. _______________________________________________
```

### Screenshots

- [ ] Main window
- [ ] Page loaded
- [ ] Error dialog (if any)
- [ ] System properties

---

## Troubleshooting

### Issue: "This program cannot be run in DOS mode"

**Cause:** PE subsystem version not set correctly  
**Solution:** Rebuild with editbin patch, verify subsystem is 4.10

### Issue: "A required .DLL file, MSVCRT140.DLL, was not found"

**Cause:** Dynamic CRT linking instead of static  
**Solution:** Rebuild with `/MT` flag

### Issue: "The procedure entry point could not be located"

**Cause:** Using API not available on Win98  
**Solution:** Check code for Vista+ APIs, rebuild with WINVER=0x0410

### Issue: Network connection fails

**Cause:** Winsock 2.2 not installed or proxy not running  
**Solution:**
1. Install Winsock 2.2 update on Win98
2. Verify proxy server running on host
3. Check firewall settings on host

### Issue: Application crashes on startup

**Cause:** Various possibilities  
**Solution:**
1. Check Event Viewer (if available)
2. Run with debugger
3. Verify all DLL dependencies
4. Check available memory

---

## Success Criteria

The Win98 build is considered successful if:

- ✅ Application launches without errors
- ✅ UI displays correctly
- ✅ Network requests work through proxy
- ✅ No crashes during 10-minute runtime
- ✅ Memory usage remains stable
- ✅ All error conditions handled gracefully
- ✅ Performance acceptable on 200MHz CPU

---

## Reporting Results

After testing, update the following files:

1. **docs/win98_testing_results.md** - Detailed test results
2. **docs/win98_compatibility_report.md** - Update with VM test findings
3. **README.md** - Update compatibility status

Include:
- Test date and environment
- All test results (pass/fail)
- Screenshots
- Any issues found
- Performance measurements

---

**Note:** This is a manual testing process. Automated testing on Win98 is not feasible due to VM limitations and lack of modern testing tools on Win98.
