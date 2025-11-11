# Create Test BMP Images for RetroBrowser Testing
# This script creates simple colored BMP images using .NET Drawing

Add-Type -AssemblyName System.Drawing

$outputDir = "demo\images"
if (-not (Test-Path $outputDir)) {
    New-Item -ItemType Directory -Path $outputDir -Force | Out-Null
    Write-Host "Created directory: $outputDir"
}

function Create-TestBitmap {
    param(
        [string]$FilePath,
        [int]$Width = 200,
        [int]$Height = 150,
        [System.Drawing.Color]$BackColor,
        [string]$Text
    )
    
    try {
        # Create bitmap
        $bitmap = New-Object System.Drawing.Bitmap($Width, $Height)
        $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
        
        # Fill background
        $brush = New-Object System.Drawing.SolidBrush($BackColor)
        $graphics.FillRectangle($brush, 0, 0, $Width, $Height)
        
        # Draw text
        $font = New-Object System.Drawing.Font("Arial", 16, [System.Drawing.FontStyle]::Bold)
        $textBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::White)
        $format = New-Object System.Drawing.StringFormat
        $format.Alignment = [System.Drawing.StringAlignment]::Center
        $format.LineAlignment = [System.Drawing.StringAlignment]::Center
        $rect = New-Object System.Drawing.RectangleF(0, 0, $Width, $Height)
        $graphics.DrawString($Text, $font, $textBrush, $rect, $format)
        
        # Draw border
        $pen = New-Object System.Drawing.Pen([System.Drawing.Color]::Black, 3)
        $graphics.DrawRectangle($pen, 0, 0, $Width - 1, $Height - 1)
        
        # Save as BMP
        $bitmap.Save($FilePath, [System.Drawing.Imaging.ImageFormat]::Bmp)
        
        # Cleanup
        $graphics.Dispose()
        $bitmap.Dispose()
        $brush.Dispose()
        $textBrush.Dispose()
        $font.Dispose()
        $pen.Dispose()
        
        Write-Host "Created: $FilePath ($Width x $Height)"
        return $true
    }
    catch {
        Write-Host "Error creating $FilePath : $_" -ForegroundColor Red
        return $false
    }
}

# Create test images
Write-Host "`nCreating test BMP images...`n" -ForegroundColor Green

Create-TestBitmap -FilePath "$outputDir\test_image.bmp" `
                   -Width 300 -Height 200 `
                   -BackColor ([System.Drawing.Color]::Blue) `
                   -Text "Test Image 1"

Create-TestBitmap -FilePath "$outputDir\image1.bmp" `
                   -Width 200 -Height 150 `
                   -BackColor ([System.Drawing.Color]::Red) `
                   -Text "Image 1"

Create-TestBitmap -FilePath "$outputDir\image2.bmp" `
                   -Width 200 -Height 150 `
                   -BackColor ([System.Drawing.Color]::Green) `
                   -Text "Image 2"

Create-TestBitmap -FilePath "$outputDir\image3.bmp" `
                   -Width 200 -Height 150 `
                   -BackColor ([System.Drawing.Color]::Orange) `
                   -Text "Image 3"

Create-TestBitmap -FilePath "$outputDir\test.bmp" `
                   -Width 250 -Height 180 `
                   -BackColor ([System.Drawing.Color]::Purple) `
                   -Text "Relative Path"

Write-Host "`nTest images created successfully!" -ForegroundColor Green
Write-Host "`nYou can now test the browser with demo\image_test_local.html" -ForegroundColor Yellow
