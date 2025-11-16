# Hướng Dẫn Deploy RetroBrowser lên Windows 98

**Ngôn ngữ:** Tiếng Việt  
**Phiên bản:** 1.0  
**Ngày:** 2025-11-17

---

## 1. Files Cần Chuyển Sang Win98

### ✅ Chỉ cần 1 file duy nhất!

```
deploy/RetroBrowser_Win98.exe  (834,560 bytes)
```

**Giải thích:**
- ✅ **Static linking:** Tất cả code đã được nhúng vào exe
- ✅ **Không cần MSVCRT.DLL:** CRT đã được static link
- ✅ **Không cần libs/:** Không có external resources
- ✅ **Không cần config files:** Không đọc file cấu hình ngoài

### ❌ KHÔNG cần các files sau:

- ❌ `libs/` folder - Đã được link vào exe
- ❌ `src/` folder - Source code không cần trên Win98
- ❌ `.dll` files - Tất cả đã static link
- ❌ Config files - Không có file cấu hình

### 📦 Optional (Khuyến nghị):

Nếu muốn có documentation trên Win98:

```
README_WIN98.txt           - Hướng dẫn nhanh
docs/troubleshooting.txt   - Xử lý lỗi
```

---

## 2. Cấu Trúc Thư Mục Trên Win98

### Cấu trúc đơn giản (Khuyến nghị):

```
C:\
└── RetroBrowser\
    └── RetroBrowser.exe  (đổi tên từ RetroBrowser_Win98.exe)
```

**Lý do:**
- Đường dẫn ngắn, dễ nhớ
- Tránh khoảng trắng trong path (Win98 đôi khi có vấn đề)
- Dễ tạo shortcut trên Desktop

### Cấu trúc đầy đủ (Nếu muốn có docs):

```
C:\
└── RetroBrowser\
    ├── RetroBrowser.exe
    ├── README.txt
    └── docs\
        ├── help.txt
        └── troubleshooting.txt
```

### ⚠️ Tránh các đường dẫn sau:

```
❌ C:\Program Files\RetroBrowser\  - Có khoảng trắng
❌ C:\Documents and Settings\...   - Đường dẫn dài
❌ D:\My Documents\...              - Có khoảng trắng
```

---

## 3. Môi Trường Win98 Cần Chuẩn Bị

### A. Yêu Cầu Hệ Thống (Bắt buộc)

#### ✅ Windows Version:
- **Windows 98 Second Edition (SE)** - Khuyến nghị
- **Windows 98 First Edition (FE)** - Cần cài thêm Winsock 2.2
- **Windows ME** - Tương thích

Kiểm tra version:
```
Right-click My Computer → Properties
Xem "System:" phải là "Microsoft Windows 98" hoặc "Windows Millennium"
```

#### ✅ Hardware Tối Thiểu:
- **CPU:** 200MHz Pentium hoặc tương đương
- **RAM:** 64MB (khuyến nghị 128MB)
- **HDD:** 2GB với 50MB trống
- **Network:** Ethernet adapter hoặc dial-up modem

#### ✅ Winsock 2.2 (Quan trọng!):

**Kiểm tra Winsock 2.2:**
```
1. Mở C:\Windows\System\
2. Tìm file ws2_32.dll
3. Right-click → Properties → Version
4. Phải là version 4.10.1656 hoặc cao hơn
```

**Nếu không có Winsock 2.2:**

**Windows 98 SE:** Đã có sẵn ✓

**Windows 98 FE:** Cần cài update:
1. Download "Windows 98 Winsock 2 Update" từ Microsoft
2. File: `w98ws2setup.exe`
3. Chạy installer
4. Restart Win98

**Tải từ đâu:**
- Microsoft Download Center (archive)
- WinWorld (https://winworldpc.com)
- Hoặc copy từ Win98 SE CD

### B. Network Configuration (Bắt buộc)

#### ✅ TCP/IP Protocol:

**Kiểm tra:**
```
1. Control Panel → Network
2. Trong "Configuration" tab, phải có:
   - "TCP/IP -> [Your Network Adapter]"
```

**Nếu chưa có:**
```
1. Click "Add" → Protocol → Add
2. Chọn "Microsoft" → "TCP/IP"
3. Click OK
4. Restart Win98
```

#### ✅ Network Adapter:

**VirtualBox:**
```
VM Settings → Network
- Adapter 1: Enable
- Attached to: Host-only Adapter
- Name: VirtualBox Host-Only Ethernet Adapter
```

**VMware:**
```
VM Settings → Network Adapter
- Network connection: Host-only
```

**Physical Hardware:**
```
- Ethernet card phải được nhận diện trong Device Manager
- Driver phải được cài đặt đúng
```

#### ✅ IP Configuration:

**Automatic (DHCP) - Khuyến nghị:**
```
Control Panel → Network → TCP/IP Properties
- IP Address tab: "Obtain an IP address automatically"
```

**Manual (Static IP):**
```
IP Address: 192.168.56.101
Subnet Mask: 255.255.255.0
Gateway: 192.168.56.1 (host machine IP)
```

**Kiểm tra kết nối:**
```
Start → Run → command
C:\> ping 192.168.56.1
Phải thấy "Reply from 192.168.56.1"
```

### C. Optional Components (Không bắt buộc)

#### Internet Explorer 5.0+:
- Không cần để chạy RetroBrowser
- Nhưng có thể hữu ích để so sánh rendering

#### Common Controls Update:
- RetroBrowser dùng comctl32.dll (có sẵn trên Win98)
- Không cần update

#### DirectX:
- Không cần (RetroBrowser không dùng DirectX)

---

## 4. Các Bước Deploy Chi Tiết

### Bước 1: Chuẩn Bị File Trên Host Machine

**Trên Windows 10/11:**

```batch
REM Tạo deployment package
cd C:\Users\LMC\OneDrive - THS\Desktop\RetroBrowser_Project
cmd /c prepare_win98_deployment.bat

REM File sẽ ở: deploy\win98_package\RetroBrowser.exe
```

### Bước 2: Transfer File Sang Win98

#### Phương Pháp A: Shared Folder (Khuyến nghị)

**VirtualBox:**
```
1. VM Settings → Shared Folders
2. Add new shared folder:
   - Folder Path: C:\Users\LMC\OneDrive - THS\Desktop\RetroBrowser_Project\deploy\win98_package
   - Folder Name: RetroBrowser
   - Auto-mount: Yes
   - Read-only: No

3. Trong Win98:
   - Mở Network Neighborhood
   - Tìm \\VBOXSVR\RetroBrowser
   - Copy RetroBrowser.exe sang C:\RetroBrowser\
```

**VMware:**
```
1. VM → Settings → Options → Shared Folders
2. Add shared folder (tương tự VirtualBox)
3. Trong Win98:
   - Mở Network Neighborhood
   - Tìm \\vmware-host\Shared Folders\RetroBrowser
   - Copy file
```

#### Phương Pháp B: Network Share

**Trên Host (Windows 10/11):**
```
1. Right-click deploy\win98_package folder
2. Properties → Sharing → Advanced Sharing
3. Share this folder: Yes
4. Permissions: Everyone → Full Control
5. Note share path: \\DESKTOP-XXX\win98_package
```

**Trong Win98:**
```
1. Start → Run
2. Gõ: \\192.168.56.1\win98_package
3. Copy RetroBrowser.exe sang C:\RetroBrowser\
```

#### Phương Pháp C: ISO/CD Image

**Tạo ISO trên host:**
```
1. Dùng ImgBurn hoặc CDBurnerXP
2. Add file RetroBrowser.exe
3. Create ISO: RetroBrowser.iso
```

**Mount trong VM:**
```
VirtualBox: Devices → Optical Drives → Choose disk image
VMware: VM → Settings → CD/DVD → Use ISO image file

Trong Win98:
- Mở D:\ (CD-ROM drive)
- Copy RetroBrowser.exe sang C:\RetroBrowser\
```

#### Phương Pháp D: Floppy Disk (Nếu file nhỏ)

**Lưu ý:** RetroBrowser.exe = 815KB, vừa đủ 1 floppy (1.44MB)

```
1. Trên host, copy file vào floppy image
2. Mount floppy trong VM
3. Copy từ A:\ sang C:\RetroBrowser\
```

### Bước 3: Tạo Thư Mục Trên Win98

```
1. Mở My Computer
2. Double-click C:\
3. File → New → Folder
4. Đặt tên: RetroBrowser
5. Copy RetroBrowser.exe vào folder này
```

### Bước 4: Tạo Shortcut (Optional)

```
1. Right-click C:\RetroBrowser\RetroBrowser.exe
2. Create Shortcut
3. Drag shortcut ra Desktop
4. Đổi tên: "Retro Browser"
```

### Bước 5: Verify Installation

**Kiểm tra file:**
```
1. Mở C:\RetroBrowser\
2. Right-click RetroBrowser.exe → Properties
3. Kiểm tra:
   - Size: 815 KB (834,560 bytes)
   - Type: Application
   - Version: (nếu có)
```

---

## 5. Cấu Hình Proxy Server

### Trên Host Machine (Windows 10/11):

**Bước 1: Tìm IP của host:**
```powershell
ipconfig | findstr "IPv4"
# Tìm dòng có 192.168.56.1 (host-only adapter)
```

**Bước 2: Start proxy server:**
```batch
cd C:\Users\LMC\OneDrive - THS\Desktop\RetroBrowser_Project
python src\proxy\proxy.py
```

**Output mong đợi:**
```
Proxy server listening on 0.0.0.0:8080
Ready to accept connections from Win98...
```

**Bước 3: Kiểm tra firewall:**
```
Windows Defender Firewall → Allow an app
Đảm bảo Python được allow cho Private networks
```

### Trên Win98:

**Proxy settings sẽ được config trong RetroBrowser UI khi chạy lần đầu**

---

## 6. Checklist Trước Khi Chạy

### ✅ Trên Win98:

- [ ] Windows 98 SE hoặc FE với Winsock 2.2
- [ ] RAM ≥ 64MB (check trong System Properties)
- [ ] TCP/IP protocol đã cài
- [ ] Network adapter hoạt động
- [ ] Có thể ping được host machine (192.168.56.1)
- [ ] File RetroBrowser.exe đã copy vào C:\RetroBrowser\
- [ ] File size = 834,560 bytes

### ✅ Trên Host Machine:

- [ ] Proxy server đang chạy (python src\proxy\proxy.py)
- [ ] Port 8080 không bị block bởi firewall
- [ ] Host-only network adapter đã enable
- [ ] IP của host là 192.168.56.1 (hoặc note lại IP khác)

---

## 7. Chạy Lần Đầu

### Bước 1: Launch Application

```
1. Double-click C:\RetroBrowser\RetroBrowser.exe
2. Hoặc: Start → Run → C:\RetroBrowser\RetroBrowser.exe
```

**Mong đợi:**
- Cửa sổ browser mở ra
- Không có error dialog
- UI hiển thị đầy đủ (menu, toolbar, address bar, status bar)

### Bước 2: Configure Proxy (Lần đầu)

```
1. Trong RetroBrowser, mở Settings/Options
2. Nhập:
   - Proxy Host: 192.168.56.1 (IP của host machine)
   - Proxy Port: 8080
3. Click Save/OK
```

### Bước 3: Test Navigation

```
1. Trong address bar, gõ: http://example.com
2. Click Go hoặc nhấn Enter
3. Chờ page load (5-10 giây)
```

**Mong đợi:**
- Status bar hiển thị "Connecting to proxy..."
- Sau đó "Downloading..."
- Cuối cùng "Done"
- Page content hiển thị trong render area

---

## 8. Troubleshooting

### Lỗi: "This program cannot be run in DOS mode"

**Nguyên nhân:** PE subsystem version không đúng

**Giải pháp:**
```
1. Verify file đúng: RetroBrowser_Win98.exe (không phải RetroBrowser.exe)
2. Check file size: 834,560 bytes
3. Rebuild nếu cần: cmd /c build_win98.bat
```

### Lỗi: "A required .DLL file was not found"

**Nguyên nhân:** Thiếu system DLL

**Giải pháp:**
```
1. Check ws2_32.dll: C:\Windows\System\ws2_32.dll
2. Nếu không có: Cài Winsock 2.2 update
3. Check comctl32.dll: C:\Windows\System\comctl32.dll
4. Nếu không có: Reinstall Windows 98
```

### Lỗi: "The procedure entry point could not be located"

**Nguyên nhân:** Dùng API không có trên Win98

**Giải pháp:**
```
1. Verify đang dùng RetroBrowser_Win98.exe (không phải bản thường)
2. Check build date: Phải là build mới nhất
3. Report bug nếu vẫn lỗi
```

### Lỗi: Network connection fails

**Nguyên nhân:** Proxy không accessible

**Giải pháp:**
```
1. Trên Win98, test ping:
   C:\> ping 192.168.56.1
   Phải có "Reply from..."

2. Trên host, check proxy running:
   Phải thấy "Proxy server listening on 0.0.0.0:8080"

3. Check firewall trên host:
   Allow Python port 8080

4. Verify proxy settings trong RetroBrowser:
   Host: 192.168.56.1
   Port: 8080
```

### Application crashes on startup

**Nguyên nhân:** Nhiều khả năng

**Giải pháp:**
```
1. Check available memory:
   Right-click My Computer → Properties
   Phải có ít nhất 32MB free RAM

2. Close other applications:
   Ctrl+Alt+Del → Close unnecessary programs

3. Reboot Win98:
   Start → Shut Down → Restart

4. Check Event Viewer (nếu có):
   Start → Programs → Administrative Tools → Event Viewer
```

---

## 9. Performance Tips

### Tối Ưu RAM:

```
1. Close unnecessary programs trước khi chạy RetroBrowser
2. Disable screen savers
3. Disable Active Desktop (nếu có)
```

### Tối Ưu Network:

```
1. Dùng host-only adapter (nhanh hơn NAT)
2. Đảm bảo proxy server chạy trên SSD (không phải HDD)
3. Close các network applications khác trên Win98
```

### Tối Ưu Display:

```
1. Set screen resolution: 800x600 hoặc 1024x768
2. Color depth: 16-bit (High Color) - đủ dùng, nhanh hơn 32-bit
3. Disable desktop effects
```

---

## 10. Uninstall

### Gỡ bỏ hoàn toàn:

```
1. Close RetroBrowser nếu đang chạy
2. Delete C:\RetroBrowser\ folder
3. Delete shortcuts trên Desktop
4. Delete registry entries (nếu có):
   Start → Run → regedit
   Tìm và xóa: HKEY_CURRENT_USER\Software\RetroBrowser
```

**Lưu ý:** RetroBrowser không cài vào registry, chỉ cần xóa folder.

---

## 11. Tóm Tắt Nhanh

### Minimum Setup:

```
1. Copy RetroBrowser_Win98.exe → C:\RetroBrowser\RetroBrowser.exe
2. Đảm bảo Winsock 2.2 đã cài (Win98 SE có sẵn)
3. Config network: TCP/IP + host-only adapter
4. Start proxy trên host: python src\proxy\proxy.py
5. Run RetroBrowser.exe
6. Config proxy: 192.168.56.1:8080
7. Navigate to http://example.com
```

### Thời gian ước tính:

- Setup Win98 VM: 30-60 phút (nếu chưa có)
- Transfer file: 2-5 phút
- Config network: 5-10 phút
- First run test: 2-3 phút

**Tổng:** ~45-80 phút (bao gồm setup VM)

---

## 12. Support

### Nếu gặp vấn đề:

1. Check `docs/win98_vm_testing_instructions.md` - Hướng dẫn chi tiết
2. Check `docs/troubleshooting.txt` - Xử lý lỗi thường gặp
3. Review build logs trong `deploy/` folder
4. Report issue với thông tin:
   - Windows version (98 FE/SE/ME)
   - RAM size
   - Error message chính xác
   - Screenshot nếu có

---

**Chúc bạn deploy thành công! 🎉**

**Build Version:** RetroBrowser_Win98.exe (834,560 bytes)  
**Build Date:** 2025-11-17  
**Subsystem:** 4.10 (Windows 98)  
**Validation:** PASSED ✅
