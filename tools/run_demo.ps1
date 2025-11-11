# ============================================================================
# run_demo.ps1 - PowerShell Demo Launcher for Win98 Retro Browser
# ============================================================================
# Windows-native alternative to run_demo.sh for users without Git Bash/WSL
# Provides identical functionality with PowerShell-idiomatic implementation
# ============================================================================

param(
    [switch]$TestMode,      # Run proxy only without launching browser
    [switch]$Debug,         # Enable verbose debug logging
    [int]$ProxyPort = 8080, # Custom proxy port (default: 8080)
    [string]$ProxyHost = "0.0.0.0"  # Custom proxy bind address
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

# ============================================================================
# CONFIGURATION
# ============================================================================

$Script:ProjectRoot = Split-Path -Parent $PSScriptRoot
$Script:ProxyScript = Join-Path $ProjectRoot "src\proxy\proxy.py"
$Script:BrowserExe = Join-Path $ProjectRoot "deploy\RetroBrowser.exe"
$Script:LogDir = Join-Path $ProjectRoot "logs"
$Script:LogFile = Join-Path $LogDir "demo_session_$(Get-Date -Format 'yyyyMMdd_HHmmss').log"
$Script:ProxyProcess = $null
$Script:SessionStart = Get-Date

# ============================================================================
# LOGGING FUNCTIONS
# ============================================================================

function Write-Log {
    param(
        [string]$Level,
        [string]$Message
    )
    
    $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    $logEntry = "[$Level] $timestamp - $Message"
    
    switch ($Level) {
        "INFO"    { Write-Host $logEntry -ForegroundColor Cyan }
        "SUCCESS" { Write-Host $logEntry -ForegroundColor Green }
        "WARN"    { Write-Host $logEntry -ForegroundColor Yellow }
        "ERROR"   { Write-Host $logEntry -ForegroundColor Red }
        "DEBUG"   { if ($Debug) { Write-Host $logEntry -ForegroundColor Magenta } }
        "HEADER"  { Write-Host "`n=== $Message ===" -ForegroundColor Blue; Write-Host }
    }
    
    Add-Content -Path $Script:LogFile -Value $logEntry -ErrorAction SilentlyContinue
}

function Write-Separator {
    $line = "=" * 80
    Write-Host $line
    Add-Content -Path $Script:LogFile -Value $line -ErrorAction SilentlyContinue
}

# ============================================================================
# PREREQUISITE CHECKS
# ============================================================================

function Test-Prerequisites {
    Write-Log "HEADER" "Prerequisite Validation"
    
    $errors = 0
    
    # Check Python
    try {
        $pythonVersion = & python --version 2>&1
        Write-Log "SUCCESS" "Python detected: $pythonVersion"
    } catch {
        Write-Log "ERROR" "Python not found. Install Python 3.7+ from python.org"
        $errors++
    }
    
    # Check Python packages
    Write-Log "INFO" "Validating Python packages..."
    try {
        & python -c "import requests, bs4" 2>$null
        Write-Log "SUCCESS" "Python dependencies satisfied"
    } catch {
        Write-Log "ERROR" "Missing packages. Run: pip install requests beautifulsoup4"
        $errors++
    }
    
    # Check proxy script
    if (Test-Path $Script:ProxyScript) {
        Write-Log "SUCCESS" "Proxy script found: $Script:ProxyScript"
    } else {
        Write-Log "ERROR" "Proxy script not found: $Script:ProxyScript"
        $errors++
    }
    
    # Check browser exe
    if (Test-Path $Script:BrowserExe) {
        $size = (Get-Item $Script:BrowserExe).Length / 1KB
        Write-Log "SUCCESS" "Browser executable found: $Script:BrowserExe ($([math]::Round($size, 2)) KB)"
    } else {
        Write-Log "ERROR" "Browser not found: $Script:BrowserExe. Build project first (Ctrl+Shift+B)"
        $errors++
    }
    
    # Check port availability
    $portInUse = Get-NetTCPConnection -LocalPort $ProxyPort -ErrorAction SilentlyContinue
    if ($portInUse) {
        Write-Log "ERROR" "Port $ProxyPort already in use. Choose different port with -ProxyPort parameter"
        $errors++
    } else {
        Write-Log "SUCCESS" "Port $ProxyPort is available"
    }
    
    # Create log directory
    if (-not (Test-Path $Script:LogDir)) {
        New-Item -ItemType Directory -Path $Script:LogDir | Out-Null
        Write-Log "SUCCESS" "Created log directory: $Script:LogDir"
    }
    
    if ($errors -gt 0) {
        Write-Log "ERROR" "Prerequisite validation failed with $errors error(s)"
        exit 1
    }
    
    Write-Log "SUCCESS" "All prerequisites satisfied"
    Write-Separator
}

# ============================================================================
# PROXY MANAGEMENT
# ============================================================================

function Start-ProxyServer {
    Write-Log "HEADER" "Starting Python Proxy Server"
    
    $env:PROXY_HOST = $ProxyHost
    $env:PROXY_PORT = $ProxyPort
    
    Write-Log "INFO" "Launching proxy on ${ProxyHost}:${ProxyPort}"
    
    # Start proxy process with output redirection to log file
    $Script:ProxyProcess = Start-Process -FilePath "python" `
        -ArgumentList "`"$Script:ProxyScript`"" `
        -NoNewWindow `
        -PassThru `
        -RedirectStandardOutput "$Script:LogFile.proxy.out" `
        -RedirectStandardError "$Script:LogFile.proxy.err"
    
    Write-Log "INFO" "Proxy started with PID: $($Script:ProxyProcess.Id)"
    
    # Wait for proxy to be ready
    Write-Log "INFO" "Waiting for proxy to be ready (timeout: 10s)..."
    
    $timeout = 10
    $elapsed = 0
    $ready = $false
    
    while ($elapsed -lt $timeout) {
        if ($Script:ProxyProcess.HasExited) {
            Write-Log "ERROR" "Proxy process exited unexpectedly"
            Write-Log "ERROR" "Check log: $Script:LogFile"
            exit 2
        }
        
        try {
            $connection = Test-NetConnection -ComputerName localhost -Port $ProxyPort -InformationLevel Quiet -WarningAction SilentlyContinue
            if ($connection) {
                $ready = $true
                break
            }
        } catch {
            # Port not ready yet
        }
        
        Start-Sleep -Seconds 1
        $elapsed++
        Write-Log "DEBUG" "Waiting for proxy... ($elapsed/$timeout)"
    }
    
    if (-not $ready) {
        Write-Log "ERROR" "Proxy did not respond within ${timeout}s timeout"
        Write-Log "ERROR" "Last 20 lines of log:"
        Get-Content $Script:LogFile -Tail 20 | ForEach-Object { Write-Host "  $_" }
        exit 4
    }
    
    Write-Log "SUCCESS" "Proxy is ready and listening on ${ProxyHost}:${ProxyPort}"
    Write-Separator
}

function Stop-ProxyServer {
    if ($Script:ProxyProcess -and -not $Script:ProxyProcess.HasExited) {
        Write-Log "INFO" "Stopping proxy (PID: $($Script:ProxyProcess.Id))..."
        
        try {
            $Script:ProxyProcess.Kill()
            $Script:ProxyProcess.WaitForExit(5000)
            Write-Log "SUCCESS" "Proxy stopped"
        } catch {
            Write-Log "WARN" "Failed to stop proxy gracefully: $_"
        }
    }
}

# ============================================================================
# BROWSER LAUNCH
# ============================================================================

function Start-Browser {
    Write-Log "HEADER" "Launching Browser"
    
    Write-Log "INFO" "Browser executable: $Script:BrowserExe"
    Write-Log "INFO" "Proxy available at: http://localhost:${ProxyPort}/"
    
    try {
        Start-Process -FilePath $Script:BrowserExe -Wait
        Write-Log "SUCCESS" "Browser closed"
    } catch {
        Write-Log "ERROR" "Failed to launch browser: $_"
        exit 3
    }
    
    Write-Separator
}

# ============================================================================
# CLEANUP
# ============================================================================

function Invoke-Cleanup {
    Write-Log "HEADER" "Cleanup & Shutdown"
    
    Stop-ProxyServer
    
    $duration = (Get-Date) - $Script:SessionStart
    Write-Log "INFO" "Session duration: $([math]::Round($duration.TotalSeconds))s"
    
    Write-Log "SUCCESS" "Cleanup completed"
    Write-Separator
}

# ============================================================================
# MAIN EXECUTION
# ============================================================================

function Main {
    try {
        # Print banner
        Write-Separator
        Write-Log "HEADER" "Win98 Retro Browser - Demo Launcher (PowerShell)"
        Write-Host "Project: BTL HDH - Retro Browser with Async Image Loading"
        Write-Host "Date: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')"
        Write-Host "Log: $Script:LogFile"
        Write-Separator
        
        # Step 1: Validate prerequisites
        Test-Prerequisites
        
        # Step 2: Start proxy
        Start-ProxyServer
        
        # Step 3: Launch browser or wait (test mode)
        if ($TestMode) {
            Write-Log "WARN" "TEST_MODE enabled - proxy running without browser"
            Write-Log "INFO" "Press Ctrl+C to stop proxy and exit"
            
            while ($true) {
                Start-Sleep -Seconds 1
            }
        } else {
            Start-Browser
        }
        
        Write-Log "SUCCESS" "Demo completed successfully"
        
    } catch {
        Write-Log "ERROR" "Unexpected error: $_"
        Write-Log "ERROR" $_.ScriptStackTrace
        exit 1
    } finally {
        Invoke-Cleanup
    }
}

# Handle Ctrl+C gracefully
$null = Register-EngineEvent -SourceIdentifier PowerShell.Exiting -Action {
    Invoke-Cleanup
}

# Execute main
Main
