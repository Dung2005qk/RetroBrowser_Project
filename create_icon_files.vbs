' ============================================================================
' create_icon_files.vbs - Tạo file icon và cursor đơn giản cho Win98
' ============================================================================
' Chạy script này trên Win98: cscript create_icon_files.vbs
' Hoặc double-click để chạy với wscript
' ============================================================================

Set fso = CreateObject("Scripting.FileSystemObject")

' Tạo thư mục res nếu chưa có
resPath = "src\browser\res"
If Not fso.FolderExists(resPath) Then
    fso.CreateFolder(resPath)
End If

' ============================================================================
' TẠO FILE APP.ICO (16x16, 16 colors, Windows 3.x format)
' ============================================================================
' Icon format: ICONDIR header + ICONDIRENTRY + DIB bitmap
' Đây là icon đơn giản nhất: 16x16, 4-bit color (16 colors)

iconFile = resPath & "\app.ico"
Set f = fso.CreateTextFile(iconFile, True)
f.Close

' Mở file ở chế độ binary
Set stream = CreateObject("ADODB.Stream")
stream.Type = 1 ' adTypeBinary
stream.Open

' ICONDIR header (6 bytes)
stream.Write ChrB(0) & ChrB(0)  ' Reserved (must be 0)
stream.Write ChrB(1) & ChrB(0)  ' Type (1 = icon)
stream.Write ChrB(1) & ChrB(0)  ' Count (1 image)

' ICONDIRENTRY (16 bytes)
stream.Write ChrB(16)           ' Width (16 pixels)
stream.Write ChrB(16)           ' Height (16 pixels)
stream.Write ChrB(16)           ' Color count (16 colors)
stream.Write ChrB(0)            ' Reserved
stream.Write ChrB(1) & ChrB(0)  ' Color planes
stream.Write ChrB(4) & ChrB(0)  ' Bits per pixel (4-bit)
stream.Write ChrB(&H28) & ChrB(&H01) & ChrB(&H00) & ChrB(&H00)  ' Size of image data (296 bytes)
stream.Write ChrB(&H16) & ChrB(&H00) & ChrB(&H00) & ChrB(&H00)  ' Offset to image data (22 bytes)

' BITMAPINFOHEADER (40 bytes)
stream.Write ChrB(&H28) & ChrB(&H00) & ChrB(&H00) & ChrB(&H00)  ' Header size (40)
stream.Write ChrB(&H10) & ChrB(&H00) & ChrB(&H00) & ChrB(&H00)  ' Width (16)
stream.Write ChrB(&H20) & ChrB(&H00) & ChrB(&H00) & ChrB(&H00)  ' Height (32 = 16*2 for XOR+AND masks)
stream.Write ChrB(&H01) & ChrB(&H00)  ' Planes (1)
stream.Write ChrB(&H04) & ChrB(&H00)  ' Bits per pixel (4)
stream.Write ChrB(&H00) & ChrB(&H00) & ChrB(&H00) & ChrB(&H00)  ' Compression (0 = none)
stream.Write ChrB(&H00) & ChrB(&H00) & ChrB(&H00) & ChrB(&H00)  ' Image size (0 = uncompressed)
stream.Write ChrB(&H00) & ChrB(&H00) & ChrB(&H00) & ChrB(&H00)  ' X pixels per meter
stream.Write ChrB(&H00) & ChrB(&H00) & ChrB(&H00) & ChrB(&H00)  ' Y pixels per meter
stream.Write ChrB(&H10) & ChrB(&H00) & ChrB(&H00) & ChrB(&H00)  ' Colors used (16)
stream.Write ChrB(&H00) & ChrB(&H00) & ChrB(&H00) & ChrB(&H00)  ' Important colors (0 = all)

' Color palette (16 colors * 4 bytes = 64 bytes) - Standard VGA palette
Dim palette(15)
palette(0) = Array(&H00, &H00, &H00, &H00)  ' Black
palette(1) = Array(&H80, &H00, &H00, &H00)  ' Dark Red
palette(2) = Array(&H00, &H80, &H00, &H00)  ' Dark Green
palette(3) = Array(&H80, &H80, &H00, &H00)  ' Dark Yellow
palette(4) = Array(&H00, &H00, &H80, &H00)  ' Dark Blue
palette(5) = Array(&H80, &H00, &H80, &H00)  ' Dark Magenta
palette(6) = Array(&H00, &H80, &H80, &H00)  ' Dark Cyan
palette(7) = Array(&HC0, &HC0, &HC0, &H00)  ' Light Gray
palette(8) = Array(&H80, &H80, &H80, &H00)  ' Dark Gray
palette(9) = Array(&HFF, &H00, &H00, &H00)  ' Red
palette(10) = Array(&H00, &HFF, &H00, &H00) ' Green
palette(11) = Array(&HFF, &HFF, &H00, &H00) ' Yellow
palette(12) = Array(&H00, &H00, &HFF, &H00) ' Blue
palette(13) = Array(&HFF, &H00, &HFF, &H00) ' Magenta
palette(14) = Array(&H00, &HFF, &HFF, &H00) ' Cyan
palette(15) = Array(&HFF, &HFF, &HFF, &H00) ' White

For i = 0 To 15
    stream.Write ChrB(palette(i)(0)) & ChrB(palette(i)(1)) & ChrB(palette(i)(2)) & ChrB(palette(i)(3))
Next

' XOR mask (16x16 pixels, 4-bit = 128 bytes)
' Tạo icon đơn giản: viền xanh, nền trắng
For row = 0 To 15
    For col = 0 To 7  ' 2 pixels per byte (4-bit)
        If row = 0 Or row = 15 Or col = 0 Or col = 7 Then
            stream.Write ChrB(&H44)  ' Blue border (color 4)
        Else
            stream.Write ChrB(&HFF)  ' White interior (color 15)
        End If
    Next
Next

' AND mask (16x16 pixels, 1-bit = 32 bytes)
' All zeros = opaque (không trong suốt)
For i = 1 To 32
    stream.Write ChrB(&H00)
Next

stream.SaveToFile iconFile, 2 ' adSaveCreateOverWrite
stream.Close

WScript.Echo "Created: " & iconFile

' ============================================================================
' TẠO FILE HAND.CUR (32x32, monochrome cursor)
' ============================================================================

cursorFile = resPath & "\hand.cur"

Set stream2 = CreateObject("ADODB.Stream")
stream2.Type = 1
stream2.Open

' CURSORDIR header (6 bytes)
stream2.Write ChrB(0) & ChrB(0)  ' Reserved
stream2.Write ChrB(2) & ChrB(0)  ' Type (2 = cursor)
stream2.Write ChrB(1) & ChrB(0)  ' Count (1 image)

' CURSORDIRENTRY (16 bytes)
stream2.Write ChrB(32)           ' Width (32 pixels)
stream2.Write ChrB(32)           ' Height (32 pixels)
stream2.Write ChrB(0)            ' Reserved
stream2.Write ChrB(0)            ' Reserved
stream2.Write ChrB(16) & ChrB(0) ' Hotspot X (16 = center)
stream2.Write ChrB(16) & ChrB(0) ' Hotspot Y (16 = center)
stream2.Write ChrB(&H30) & ChrB(&H01) & ChrB(&H00) & ChrB(&H00)  ' Size (304 bytes)
stream2.Write ChrB(&H16) & ChrB(&H00) & ChrB(&H00) & ChrB(&H00)  ' Offset (22 bytes)

' BITMAPINFOHEADER (40 bytes)
stream2.Write ChrB(&H28) & ChrB(&H00) & ChrB(&H00) & ChrB(&H00)  ' Header size
stream2.Write ChrB(&H20) & ChrB(&H00) & ChrB(&H00) & ChrB(&H00)  ' Width (32)
stream2.Write ChrB(&H40) & ChrB(&H00) & ChrB(&H00) & ChrB(&H00)  ' Height (64 = 32*2)
stream2.Write ChrB(&H01) & ChrB(&H00)  ' Planes
stream2.Write ChrB(&H01) & ChrB(&H00)  ' Bits per pixel (1 = monochrome)
stream2.Write ChrB(&H00) & ChrB(&H00) & ChrB(&H00) & ChrB(&H00)  ' Compression
stream2.Write ChrB(&H00) & ChrB(&H00) & ChrB(&H00) & ChrB(&H00)  ' Image size
stream2.Write ChrB(&H00) & ChrB(&H00) & ChrB(&H00) & ChrB(&H00)  ' X ppm
stream2.Write ChrB(&H00) & ChrB(&H00) & ChrB(&H00) & ChrB(&H00)  ' Y ppm
stream2.Write ChrB(&H00) & ChrB(&H00) & ChrB(&H00) & ChrB(&H00)  ' Colors used
stream2.Write ChrB(&H00) & ChrB(&H00) & ChrB(&H00) & ChrB(&H00)  ' Important colors

' Color palette (2 colors * 4 bytes = 8 bytes)
stream2.Write ChrB(&H00) & ChrB(&H00) & ChrB(&H00) & ChrB(&H00)  ' Black
stream2.Write ChrB(&HFF) & ChrB(&HFF) & ChrB(&HFF) & ChrB(&H00)  ' White

' XOR mask (32x32 pixels, 1-bit = 128 bytes) - Hand shape
' Simplified hand cursor (all white for now)
For i = 1 To 128
    stream2.Write ChrB(&HFF)
Next

' AND mask (32x32 pixels, 1-bit = 128 bytes) - Transparency
For i = 1 To 128
    stream2.Write ChrB(&H00)
Next

stream2.SaveToFile cursorFile, 2
stream2.Close

WScript.Echo "Created: " & cursorFile
WScript.Echo ""
WScript.Echo "SUCCESS! Icon and cursor files created."
WScript.Echo "Now you can build the project in Visual Studio."
