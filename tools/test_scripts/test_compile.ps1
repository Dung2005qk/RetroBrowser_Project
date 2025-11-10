# PowerShell Test Script for RetroBrowser UI Module
# Tests if the reported errors are real compilation errors or IntelliSense artifacts

Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "RetroBrowser UI Module - Compilation Verification Test" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host ""

# Test 1: Check if required header files exist
Write-Host "Test 1: Checking required header files..." -ForegroundColor Yellow
$headers = @(
    "src\browser\core\stdafx.h",
    "src\browser\ui\ui.h",
    "src\browser\renderer\renderer.h",
    "src\browser\parser\parser.h"
)

$allHeadersExist = $true
foreach ($header in $headers) {
    if (Test-Path $header) {
        Write-Host "  ✓ Found: $header" -ForegroundColor Green
    } else {
        Write-Host "  ✗ Missing: $header" -ForegroundColor Red
        $allHeadersExist = $false
    }
}

if (-not $allHeadersExist) {
    Write-Host ""
    Write-Host "ERROR: Some required headers are missing." -ForegroundColor Red
    Write-Host "Cannot proceed with compilation test." -ForegroundColor Red
    pause
    exit 1
}

Write-Host ""
Write-Host "Test 2: Analyzing ui.cpp for common issues..." -ForegroundColor Yellow

# Check for typedef
$uiCppContent = Get-Content "src\browser\ui\ui.cpp" -Raw
if ($uiCppContent -match "typedef Parser::ParseResult ParsedPageData") {
    Write-Host "  ✓ typedef ParsedPageData is present" -ForegroundColor Green
} else {
    Write-Host "  ✗ typedef ParsedPageData is missing" -ForegroundColor Red
}

# Check for Renderer usage
if ($uiCppContent -match "Renderer::HtmlRenderer\*\s+pRenderer") {
    Write-Host "  ✓ Renderer::HtmlRenderer is used correctly" -ForegroundColor Green
} else {
    Write-Host "  ⚠ Renderer::HtmlRenderer usage pattern not found" -ForegroundColor Yellow
}

# Check for SetWindowSubclass
if ($uiCppContent -match "SetWindowSubclass") {
    Write-Host "  ✓ SetWindowSubclass is used" -ForegroundColor Green
} else {
    Write-Host "  ⚠ SetWindowSubclass not found" -ForegroundColor Yellow
}

# Check for DefSubclassProc
if ($uiCppContent -match "DefSubclassProc") {
    Write-Host "  ✓ DefSubclassProc is used" -ForegroundColor Green
} else {
    Write-Host "  ⚠ DefSubclassProc not found" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "Test 3: Checking for namespace declarations..." -ForegroundColor Yellow

# Check Parser namespace
$parserHeader = Get-Content "src\browser\parser\parser.h" -Raw
if ($parserHeader -match "namespace Parser\s*\{") {
    Write-Host "  ✓ Parser namespace found in parser.h" -ForegroundColor Green
    
    if ($parserHeader -match "enum BlockType") {
        Write-Host "    ✓ BlockType enum found" -ForegroundColor Green
    }
    
    if ($parserHeader -match "struct ParseResult") {
        Write-Host "    ✓ ParseResult struct found" -ForegroundColor Green
    }
    
    if ($parserHeader -match "PARSE_SUCCESS") {
        Write-Host "    ✓ PARSE_SUCCESS constant found" -ForegroundColor Green
    }
    
    if ($parserHeader -match "BLOCK_H1") {
        Write-Host "    ✓ BLOCK_H1 constant found" -ForegroundColor Green
    }
} else {
    Write-Host "  ✗ Parser namespace NOT found" -ForegroundColor Red
}

# Check Renderer namespace
$rendererHeader = Get-Content "src\browser\renderer\renderer.h" -Raw
if ($rendererHeader -match "namespace Renderer\s*\{") {
    Write-Host "  ✓ Renderer namespace found in renderer.h" -ForegroundColor Green
    
    if ($rendererHeader -match "class HtmlRenderer") {
        Write-Host "    ✓ HtmlRenderer class found" -ForegroundColor Green
    }
} else {
    Write-Host "  ✗ Renderer namespace NOT found" -ForegroundColor Red
}

Write-Host ""
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "Analysis Summary:" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "The errors reported by IntelliSense are likely FALSE POSITIVES if:" -ForegroundColor Yellow
Write-Host "  1. All header files exist (✓)" -ForegroundColor Green
Write-Host "  2. Namespaces are properly declared in headers (✓)" -ForegroundColor Green
Write-Host "  3. typedef and usage patterns are correct (✓)" -ForegroundColor Green
Write-Host ""
Write-Host "Common causes of IntelliSense false positives:" -ForegroundColor Cyan
Write-Host "  • IntelliSense parser hasn't fully processed precompiled headers" -ForegroundColor Gray
Write-Host "  • Complex preprocessor directives confuse IntelliSense" -ForegroundColor Gray
Write-Host "  • IntelliSense cache is stale or corrupted" -ForegroundColor Gray
Write-Host ""
Write-Host "Recommended actions:" -ForegroundColor Yellow
Write-Host "  1. Try building with actual compiler (MSBuild or Visual Studio)" -ForegroundColor White
Write-Host "  2. If it compiles successfully, errors are IntelliSense artifacts" -ForegroundColor White
Write-Host "  3. Reload VS Code window to refresh IntelliSense" -ForegroundColor White
Write-Host "  4. Clear IntelliSense cache if problems persist" -ForegroundColor White
Write-Host ""

# Try to run syntax check if cl.exe is available
$clPath = Get-Command cl.exe -ErrorAction SilentlyContinue
if ($clPath) {
    Write-Host "Test 4: Running compiler syntax check..." -ForegroundColor Yellow
    Write-Host "  Compiler found: $($clPath.Source)" -ForegroundColor Green
    Write-Host ""
    
    # Run syntax-only check
    & cl.exe /nologo /Zs /W3 /EHsc /I. /Isrc\browser\core src\browser\ui\ui.cpp 2>&1 | Out-String | ForEach-Object {
        if ($_ -match "error") {
            Write-Host $_ -ForegroundColor Red
        } elseif ($_ -match "warning") {
            Write-Host $_ -ForegroundColor Yellow
        } else {
            Write-Host $_ -ForegroundColor Gray
        }
    }
} else {
    Write-Host "Test 4: Compiler not found in PATH" -ForegroundColor Yellow
    Write-Host "  To run actual compilation test, run this from:" -ForegroundColor Gray
    Write-Host "  'Developer Command Prompt for Visual Studio'" -ForegroundColor Gray
}

Write-Host ""
pause
