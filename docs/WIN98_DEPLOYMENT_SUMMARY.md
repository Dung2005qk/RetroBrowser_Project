# Windows 98 Deployment - Executive Summary

**Date:** 2025-11-17  
**Status:** ✅ Ready for Deployment  
**Build:** RetroBrowser_Win98.exe (834,560 bytes)

---

## Quick Answer to Your Questions

### 1️⃣ Files cần chuyển sang Win98?

**CHỈ CẦN 1 FILE DUY NHẤT:**

```
deploy/RetroBrowser_Win98.exe  (834,560 bytes)
```

**KHÔNG CẦN:**
- ❌ `libs/` folder - Đã static link vào exe
- ❌ `src/` folder - Source code không cần
- ❌ `.dll` files - Tất cả đã nhúng vào exe
- ❌ Config files - Không có file cấu hình external

**Giải thích:**
- Binary được build với `/MT` flag → Static CRT linking
- Tất cả dependencies đã được link vào exe
- Không đọc file config từ disk
- Không load external resources

---

### 2️⃣ Cấu trúc thư mục trên Win98?

**KHUYẾN NGHỊ (Đơn giản nhất):**

```
C:\
└── RetroBrowser\
    └── RetroBrowser.exe
```

**Lý do:**
- ✅ Đường dẫn ngắn, dễ nhớ
- ✅ Không có khoảng trắng (Win98 đôi khi có vấn đề)
- ✅ Dễ tạo shortcut
- ✅ Dễ uninstall (chỉ cần xóa folder)

**TRÁNH:**
```
❌ C:\Program Files\RetroBrowser\  - Có khoảng trắng
❌ C:\Documents and Settings\...   - Đường dẫn dài
❌ D:\My Documents\...              - Có khoảng trắng
```

**Optional (Nếu muốn có docs):**
```
C:\
└── RetroBrowser\
    ├── RetroBrowser.exe
    ├── README.txt
    └── docs\
        └── help.txt
```

---

### 3️⃣ Win98 cần chuẩn bị/cài đặt gì?

## A. Yêu Cầu Bắt Buộc

### ✅ Windows Version
- **Windows 98 Second Edition (SE)** - Khuyến nghị mạnh
- **Windows 98 First Edition (FE)** - Cần cài thêm Winsock 2.2
- **Windows ME** - Tương thích

**Kiểm tra:**
```
Right-click My Computer → Properties
Xem "System:" phải là "Microsoft Windows 98"
```

### ✅ Hardware Minimum
- **CPU:** 200MHz Pentium hoặc tương đương
- **RAM:** 64MB (khuyến nghị 128MB)
- **HDD:** 2GB với 50MB trống
- **Network:** Ethernet adapter

**Kiểm tra RAM:**
```
Right-click My Computer → Properties
Xem "RAM:" phải ≥ 64.0 MB
```

### ✅ Winsock 2.2 (QUAN TRỌNG!)

**Kiểm tra có Winsock 2.2 chưa:**
```
1. Mở C:\Windows\System\
2. Tìm file ws2_32.dll
3. Right-click → Properties → Version
4. Phải là version 4.10.1656 hoặc cao hơn
```

**Nếu KHÔNG có (Win98 FE):**
```
1. Download "Windows 98 Winsock 2 Update"
2. File: w98ws2setup.exe
3. Chạy installer
4. Restart Win98
```

**Win98 SE:** Đã có sẵn Winsock 2.2 ✓

### ✅ TCP/IP Protocol

**Kiểm tra:**
```
Control Panel → Network → Configuration tab
Phải có: "TCP/IP -> [Your Network Adapter]"
```

**Nếu chưa có:**
```
1. Click "Add" → Protocol → Add
2. Chọn "Microsoft" → "TCP/IP"
3. Click OK
4. Restart Win98
```

### ✅ Network Configuration

**VirtualBox:**
```
VM Settings → Network
- Adapter 1: Enable
- Attached to: Host-only Adapter
```

**VMware:**
```
VM Settings → Network Adapter
- Network connection: Host-only
```

**Test kết nối:**
```
Start → Run → command
C:\> ping 192.168.56.1
Phải thấy "Reply from 192.168.56.1"
```

## B. KHÔNG Cần Cài

### ❌ Visual Studio
- Không cần compiler trên Win98
- Build đã thực hiện trên host machine

### ❌ .NET Framework
- RetroBrowser không dùng .NET
- Native Win32 application

### ❌ DirectX
- Không dùng DirectX
- Chỉ dùng GDI (có sẵn)

### ❌ Internet Explorer Update
- Không phụ thuộc IE
- Có thể chạy độc lập

### ❌ Common Controls Update
- comctl32.dll có sẵn trên Win98
- Không cần update

---

## Complete Setup Checklist

### 📋 Trên Win98 VM:

- [ ] Windows 98 SE installed (hoặc FE + Winsock 2.2)
- [ ] RAM ≥ 64MB (check System Properties)
- [ ] TCP/IP protocol installed
- [ ] Network adapter working
- [ ] Can ping host machine (192.168.56.1)
- [ ] File ws2_32.dll exists in C:\Windows\System\
- [ ] Folder C:\RetroBrowser\ created
- [ ] RetroBrowser.exe copied (834,560 bytes)

### 📋 Trên Host Machine:

- [ ] Python installed
- [ ] Proxy server ready: src\proxy\proxy.py
- [ ] Port 8080 not blocked by firewall
- [ ] Host-only network adapter enabled
- [ ] IP address is 192.168.56.1 (or noted)

---

## Step-by-Step Deployment

### Step 1: Prepare File (Host Machine)

```batch
cd C:\Users\LMC\OneDrive - THS\Desktop\RetroBrowser_Project
cmd /c prepare_win98_deployment.bat
```

Output: `deploy\win98_package\RetroBrowser.exe`

### Step 2: Transfer to Win98

**Method A - Shared Folder (Recommended):**
```
VirtualBox:
1. VM Settings → Shared Folders
2. Add: deploy\win98_package → Name: RetroBrowser
3. In Win98: \\VBOXSVR\RetroBrowser
4. Copy RetroBrowser.exe to C:\RetroBrowser\
```

**Method B - Network Share:**
```
1. Share deploy\win98_package on host
2. In Win98: \\192.168.56.1\win98_package
3. Copy file
```

**Method C - ISO:**
```
1. Create ISO with RetroBrowser.exe
2. Mount in VM
3. Copy from D:\ to C:\RetroBrowser\
```

### Step 3: Create Folder on Win98

```
1. Open My Computer
2. Double-click C:\
3. File → New → Folder
4. Name: RetroBrowser
5. Copy RetroBrowser.exe here
```

### Step 4: Start Proxy (Host Machine)

```batch
cd C:\Users\LMC\OneDrive - THS\Desktop\RetroBrowser_Project
python src\proxy\proxy.py
```

Expected output:
```
Proxy server listening on 0.0.0.0:8080
Ready to accept connections...
```

### Step 5: Run RetroBrowser (Win98)

```
1. Double-click C:\RetroBrowser\RetroBrowser.exe
2. Configure proxy:
   - Host: 192.168.56.1
   - Port: 8080
3. Test: http://example.com
```

---

## Troubleshooting Quick Reference

| Error | Cause | Solution |
|-------|-------|----------|
| "Cannot run in DOS mode" | Wrong subsystem version | Use RetroBrowser_Win98.exe (not RetroBrowser.exe) |
| "DLL not found" | Missing Winsock 2.2 | Install Winsock 2.2 update (Win98 FE) |
| "Procedure not found" | Wrong build | Verify using Win98 build (834,560 bytes) |
| Network fails | Proxy not accessible | Check ping, proxy running, firewall |
| Crashes on start | Low memory | Close other apps, reboot Win98 |

---

## Time Estimates

| Task | Time |
|------|------|
| Setup Win98 VM (if needed) | 30-60 min |
| Install Winsock 2.2 (if needed) | 5-10 min |
| Configure network | 5-10 min |
| Transfer file | 2-5 min |
| First run test | 2-3 min |
| **Total** | **45-80 min** |

---

## Documentation Reference

| Document | Purpose |
|----------|---------|
| `docs/win98_deployment_guide_vi.md` | Chi tiết đầy đủ (Tiếng Việt) |
| `docs/QUICK_START_WIN98.txt` | Hướng dẫn nhanh |
| `docs/win98_deployment_diagram.txt` | Sơ đồ kiến trúc |
| `docs/win98_vm_testing_instructions.md` | Testing guide (English) |
| `docs/win98_testing_guide.md` | Comprehensive testing |

---

## Summary

### ✅ What You Need to Transfer:
**1 file only:** `RetroBrowser_Win98.exe` (834,560 bytes)

### ✅ Where to Put It:
**Simple:** `C:\RetroBrowser\RetroBrowser.exe`

### ✅ What Win98 Needs:
1. **Windows 98 SE** (or FE + Winsock 2.2 update)
2. **64MB RAM** minimum
3. **TCP/IP protocol** installed
4. **Network adapter** working
5. **Winsock 2.2** (ws2_32.dll version 4.10.1656+)

### ✅ What Win98 Does NOT Need:
- ❌ Visual Studio
- ❌ .NET Framework
- ❌ DirectX
- ❌ libs/ folder
- ❌ Any .dll files
- ❌ Config files

---

## Build Information

```
File:           RetroBrowser_Win98.exe
Size:           834,560 bytes (815 KB)
Build Date:     2025-11-17
Compiler:       Visual Studio 2022 (v14.42)
Target:         Windows 98 SE (4.10.2222 A)
Subsystem:      4.10 (Windows 98)
CRT Linking:    Static (/MT)
Dependencies:   5 Win98 system DLLs only
Validation:     PASSED ✅
```

---

## Success Criteria

✅ Application launches without errors  
✅ No "procedure not found" errors  
✅ No missing DLL errors  
✅ UI displays correctly  
✅ Network requests work through proxy  
✅ Can load http://example.com  
✅ No crashes during 10-minute runtime  
✅ Memory usage stable (< 10MB)  

---

**Ready for deployment! 🚀**

For detailed instructions, see: `docs/win98_deployment_guide_vi.md`
