# ============================================================================
# fix_clear_calls.ps1 - Replace .clear() with VC++6.0 compatible code
# ============================================================================

Write-Host "Fixing .clear() calls for VC++6.0 compatibility..." -ForegroundColor Yellow
Write-Host ""

$files = @(
    "src\browser\parser\parser.cpp",
    "src\browser\network\network.cpp"
)

foreach ($file in $files) {
    if (Test-Path $file) {
        Write-Host "Processing: $file" -ForegroundColor Cyan
        
        $content = Get-Content $file -Raw
        $originalContent = $content
        
        # Replace string.clear() with string = ""
        $content = $content -replace '(\w+)\.clear\(\);', '$1 = "";'
        
        # Replace map.clear() with map = map<>()
        $content = $content -replace '(attributes)\.clear\(\);', '$1 = std::map<std::string, std::string>();'
        $content = $content -replace '(containerAttrs)\.clear\(\);', '$1 = std::map<std::string, std::string>();'
        
        if ($content -ne $originalContent) {
            Set-Content $file -Value $content -NoNewline
            Write-Host "  ✓ Fixed .clear() calls" -ForegroundColor Green
        } else {
            Write-Host "  - No changes needed" -ForegroundColor Gray
        }
    } else {
        Write-Host "  ✗ File not found: $file" -ForegroundColor Red
    }
}

Write-Host ""
Write-Host "Done!" -ForegroundColor Green
