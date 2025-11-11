set -euo pipefail  # Exit on error, undefined var, pipe failure
IFS=$'\n\t'        # Safer word splitting (only newline and tab)

# ============================================================================
# CONFIGURATION & CONSTANTS
# ============================================================================

# --- Color Codes for Terminal Output (ANSI Escape Sequences) ---
readonly COLOR_RESET='\033[0m'
readonly COLOR_BOLD='\033[1m'
readonly COLOR_RED='\033[0;31m'
readonly COLOR_GREEN='\033[0;32m'
readonly COLOR_YELLOW='\033[0;33m'
readonly COLOR_BLUE='\033[0;34m'
readonly COLOR_CYAN='\033[0;36m'
readonly COLOR_MAGENTA='\033[0;35m'

# --- Project Paths (Relative to Repository Root) ---
readonly SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
readonly PROXY_SCRIPT="${PROJECT_ROOT}/src/proxy/proxy.py"
readonly PROXY_CONFIG="${PROJECT_ROOT}/src/proxy/config.py"
readonly BROWSER_EXE="${BROWSER_EXE:-${PROJECT_ROOT}/deploy/RetroBrowser.exe}"
readonly DEMO_HTML="${PROJECT_ROOT}/demo/image_test.html"
readonly LOG_DIR="${PROJECT_ROOT}/logs"
readonly LOG_FILE="${LOG_FILE:-${LOG_DIR}/demo_session_$(date +%Y%m%d_%H%M%S).log}"

# --- Network Configuration ---
readonly PROXY_HOST="${PROXY_HOST:-0.0.0.0}"
readonly PROXY_PORT="${PROXY_PORT:-8080}"
readonly PROXY_TIMEOUT="${PROXY_TIMEOUT:-10}"  # Seconds to wait for proxy startup

# --- Operational Flags ---
readonly DEBUG="${DEBUG:-0}"
readonly TEST_MODE="${TEST_MODE:-0}"  # 1 = proxy only, no browser launch

# --- Runtime State (Mutable) ---
PROXY_PID=""
SESSION_START_TIME=""
SESSION_END_TIME=""

# ============================================================================
# UTILITY FUNCTIONS - Logging, Output Formatting, Error Handling
# ============================================================================

# Print message with timestamp and color
log() {
    local level="$1"
    shift
    local message="$*"
    local timestamp="$(date '+%Y-%m-%d %H:%M:%S')"
    
    case "$level" in
        INFO)
            echo -e "${COLOR_CYAN}[INFO]${COLOR_RESET} ${timestamp} - ${message}" | tee -a "$LOG_FILE"
            ;;
        SUCCESS)
            echo -e "${COLOR_GREEN}[SUCCESS]${COLOR_RESET} ${timestamp} - ${message}" | tee -a "$LOG_FILE"
            ;;
        WARN)
            echo -e "${COLOR_YELLOW}[WARN]${COLOR_RESET} ${timestamp} - ${message}" | tee -a "$LOG_FILE"
            ;;
        ERROR)
            echo -e "${COLOR_RED}[ERROR]${COLOR_RESET} ${timestamp} - ${message}" | tee -a "$LOG_FILE" >&2
            ;;
        DEBUG)
            if [[ "$DEBUG" == "1" ]]; then
                echo -e "${COLOR_MAGENTA}[DEBUG]${COLOR_RESET} ${timestamp} - ${message}" | tee -a "$LOG_FILE"
            fi
            ;;
        HEADER)
            echo -e "\n${COLOR_BOLD}${COLOR_BLUE}=== ${message} ===${COLOR_RESET}\n" | tee -a "$LOG_FILE"
            ;;
        *)
            echo "$message" | tee -a "$LOG_FILE"
            ;;
    esac
}

# Print a horizontal separator line
print_separator() {
    echo "============================================================================" | tee -a "$LOG_FILE"
}

# Fatal error: log and exit with code
die() {
    local exit_code="$1"
    shift
    log ERROR "$*"
    log ERROR "Exiting with code $exit_code"
    cleanup
    exit "$exit_code"
}

# ============================================================================
# PREREQUISITE VALIDATION - Check Dependencies Before Starting
# ============================================================================

check_prerequisites() {
    log HEADER "Prerequisite Validation"
    
    local errors=0
    
    # 1. Check Python Installation
    if ! command -v python3 &>/dev/null && ! command -v python &>/dev/null; then
        log ERROR "Python 3 not found in PATH. Install Python 3.7+ and retry."
        ((errors++))
    else
        local python_cmd="python3"
        command -v python3 &>/dev/null || python_cmd="python"
        local python_version="$($python_cmd --version 2>&1 | awk '{print $2}')"
        log SUCCESS "Python detected: $python_version"
    fi
    
    # 2. Check Python Dependencies
    log INFO "Validating Python packages (requests, beautifulsoup4)..."
    local python_cmd="python3"
    command -v python3 &>/dev/null || python_cmd="python"
    
    if ! $python_cmd -c "import requests, bs4" 2>/dev/null; then
        log ERROR "Required Python packages missing. Install with:"
        log ERROR "    pip install requests beautifulsoup4"
        log ERROR "Or run: pip install -r src/proxy/requirements.txt"
        ((errors++))
    else
        log SUCCESS "Python dependencies satisfied"
    fi
    
    # 3. Check Proxy Script Exists
    if [[ ! -f "$PROXY_SCRIPT" ]]; then
        log ERROR "Proxy script not found: $PROXY_SCRIPT"
        ((errors++))
    else
        log SUCCESS "Proxy script found: $PROXY_SCRIPT"
    fi
    
    # 4. Check Browser Executable Exists
    if [[ ! -f "$BROWSER_EXE" ]]; then
        log ERROR "Browser executable not found: $BROWSER_EXE"
        log ERROR "Build the project first: Ctrl+Shift+B in VS Code or run cl.exe manually"
        ((errors++))
    else
        local exe_size="$(stat -c%s "$BROWSER_EXE" 2>/dev/null || stat -f%z "$BROWSER_EXE" 2>/dev/null)"
        local exe_size_kb="$((exe_size / 1024))"
        log SUCCESS "Browser executable found: $BROWSER_EXE (${exe_size_kb} KB)"
    fi
    
    # 5. Check Demo HTML Exists
    if [[ ! -f "$DEMO_HTML" ]]; then
        log WARN "Demo HTML not found: $DEMO_HTML (will use default URL)"
    else
        log SUCCESS "Demo HTML found: $DEMO_HTML"
    fi
    
    # 6. Check Port Availability
    if command -v netstat &>/dev/null; then
        if netstat -an | grep -q ":${PROXY_PORT}.*LISTEN"; then
            log ERROR "Port $PROXY_PORT already in use. Stop conflicting service or set PROXY_PORT to a different value."
            ((errors++))
        else
            log SUCCESS "Port $PROXY_PORT is available"
        fi
    elif command -v lsof &>/dev/null; then
        if lsof -i :"$PROXY_PORT" &>/dev/null; then
            log ERROR "Port $PROXY_PORT already in use. Stop conflicting service or set PROXY_PORT to a different value."
            ((errors++))
        else
            log SUCCESS "Port $PROXY_PORT is available"
        fi
    else
        log WARN "Cannot check port availability (netstat/lsof not found). Proceeding anyway..."
    fi
    
    # 7. Create Log Directory
    if [[ ! -d "$LOG_DIR" ]]; then
        mkdir -p "$LOG_DIR" && log SUCCESS "Created log directory: $LOG_DIR" || {
            log ERROR "Failed to create log directory: $LOG_DIR"
            ((errors++))
        }
    fi
    
    # Fail if any prerequisite check failed
    if [[ $errors -gt 0 ]]; then
        die 1 "Prerequisite validation failed with $errors error(s). Fix issues above and retry."
    fi
    
    log SUCCESS "All prerequisites satisfied. Ready to start demo."
    print_separator
}

# ============================================================================
# PROXY MANAGEMENT - Start, Monitor, Stop Python Proxy Server
# ============================================================================

start_proxy() {
    log HEADER "Starting Python Proxy Server"
    
    local python_cmd="python3"
    command -v python3 &>/dev/null || python_cmd="python"
    
    # Set environment variables for proxy configuration
    export PROXY_HOST="$PROXY_HOST"
    export PROXY_PORT="$PROXY_PORT"
    
    log INFO "Launching proxy: $python_cmd $PROXY_SCRIPT"
    log INFO "Configuration: ${PROXY_HOST}:${PROXY_PORT}"
    
    # Start proxy as background daemon, redirect output to log
    $python_cmd "$PROXY_SCRIPT" >> "$LOG_FILE" 2>&1 &
    PROXY_PID=$!
    
    log INFO "Proxy started with PID: $PROXY_PID"
    
    # Wait for proxy to be ready (check for listening port)
    log INFO "Waiting for proxy to be ready (timeout: ${PROXY_TIMEOUT}s)..."
    
    local elapsed=0
    local ready=0
    
    while [[ $elapsed -lt $PROXY_TIMEOUT ]]; do
        # Check if process is still alive
        if ! kill -0 "$PROXY_PID" 2>/dev/null; then
            log ERROR "Proxy process died unexpectedly. Check log: $LOG_FILE"
            die 2 "Proxy startup failed"
        fi
        
        # Check if port is listening (cross-platform approach)
        if command -v netstat &>/dev/null; then
            if netstat -an 2>/dev/null | grep -q ":${PROXY_PORT}.*LISTEN"; then
                ready=1
                break
            fi
        elif command -v lsof &>/dev/null; then
            if lsof -i :"$PROXY_PORT" &>/dev/null; then
                ready=1
                break
            fi
        else
            # Fallback: try connecting via curl/wget
            if command -v curl &>/dev/null; then
                if curl -s --max-time 1 "http://localhost:${PROXY_PORT}/" &>/dev/null; then
                    ready=1
                    break
                fi
            fi
        fi
        
        sleep 1
        ((elapsed++))
        log DEBUG "Waiting for proxy... ($elapsed/$PROXY_TIMEOUT)"
    done
    
    if [[ $ready -eq 0 ]]; then
        log ERROR "Proxy did not respond within ${PROXY_TIMEOUT}s timeout"
        log ERROR "Last 20 lines of log:"
        tail -n 20 "$LOG_FILE" | while IFS= read -r line; do
            echo "  $line"
        done
        die 4 "Proxy startup timeout"
    fi
    
    log SUCCESS "Proxy is ready and listening on ${PROXY_HOST}:${PROXY_PORT}"
    print_separator
}

stop_proxy() {
    if [[ -n "$PROXY_PID" ]]; then
        log INFO "Stopping proxy (PID: $PROXY_PID)..."
        
        # Try graceful shutdown first (SIGTERM)
        if kill -0 "$PROXY_PID" 2>/dev/null; then
            kill -TERM "$PROXY_PID" 2>/dev/null || true
            
            # Wait up to 5 seconds for graceful shutdown
            local wait_count=0
            while kill -0 "$PROXY_PID" 2>/dev/null && [[ $wait_count -lt 5 ]]; do
                sleep 1
                ((wait_count++))
            done
            
            # Force kill if still alive
            if kill -0 "$PROXY_PID" 2>/dev/null; then
                log WARN "Proxy didn't stop gracefully, forcing kill..."
                kill -KILL "$PROXY_PID" 2>/dev/null || true
            fi
            
            log SUCCESS "Proxy stopped"
        else
            log DEBUG "Proxy already stopped"
        fi
        
        PROXY_PID=""
    fi
}

# ============================================================================
# BROWSER MANAGEMENT - Launch Win32 Browser Executable
# ============================================================================

launch_browser() {
    log HEADER "Launching Browser"
    
    # Determine default URL (prefer demo HTML if exists)
    local default_url="http://localhost:${PROXY_PORT}/https://example.com/"
    if [[ -f "$DEMO_HTML" ]]; then
        # Convert Windows path to file:// URL for browser
        local html_path="$(cygpath -w "$DEMO_HTML" 2>/dev/null || echo "$DEMO_HTML")"
        default_url="file:///${html_path// /%20}"
        log INFO "Using demo page: $DEMO_HTML"
    fi
    
    log INFO "Browser executable: $BROWSER_EXE"
    log INFO "Default URL: $default_url"
    
    # Launch browser in foreground (blocks until browser exits)
    # Use cmd.exe on Windows/Cygwin for proper .exe execution
    if command -v cygstart &>/dev/null; then
        # Cygwin environment
        log INFO "Launching via cygstart (Cygwin)..."
        cygstart "$BROWSER_EXE" || die 3 "Failed to launch browser"
    elif [[ "$(uname -s)" == MINGW* || "$(uname -s)" == MSYS* ]]; then
        # Git Bash / MSYS2 environment
        log INFO "Launching via cmd.exe (Git Bash/MSYS2)..."
        cmd.exe /c start "" "$(cygpath -w "$BROWSER_EXE" 2>/dev/null || echo "$BROWSER_EXE")" || die 3 "Failed to launch browser"
    elif command -v wine &>/dev/null; then
        # Linux/macOS with Wine
        log INFO "Launching via Wine (Linux/macOS)..."
        wine "$BROWSER_EXE" || die 3 "Failed to launch browser"
    else
        log ERROR "Cannot determine how to launch Windows executable on this platform"
        log ERROR "Platform: $(uname -s)"
        die 3 "Unsupported platform for browser launch"
    fi
    
    log SUCCESS "Browser launched successfully"
    log INFO "Browser will run in foreground. Close browser window to exit demo."
    print_separator
}

# ============================================================================
# CLEANUP & SHUTDOWN - Graceful Resource Cleanup
# ============================================================================

cleanup() {
    log HEADER "Cleanup & Shutdown"
    
    # Stop proxy if running
    stop_proxy
    
    # Record session end time
    SESSION_END_TIME="$(date +%s)"
    
    # Generate session summary
    if [[ -n "$SESSION_START_TIME" ]]; then
        local duration=$((SESSION_END_TIME - SESSION_START_TIME))
        log INFO "Session duration: ${duration}s"
    fi
    
    log SUCCESS "Cleanup completed"
    print_separator
}

# Trap signals for graceful shutdown
trap cleanup EXIT
trap 'die 130 "Interrupted by user (Ctrl+C)"' INT TERM

# ============================================================================
# MAIN EXECUTION FLOW
# ============================================================================

main() {
    SESSION_START_TIME="$(date +%s)"
    
    # Print banner
    print_separator
    log HEADER "Win98 Retro Browser - Demo Launcher"
    log "" "Project: BTL HDH - Retro Browser with Async Image Loading"
    log "" "Date: $(date '+%Y-%m-%d %H:%M:%S')"
    log "" "Log: $LOG_FILE"
    print_separator
    
    # Step 1: Validate prerequisites
    check_prerequisites
    
    # Step 2: Start proxy server
    start_proxy
    
    # Step 3: Launch browser (or skip if TEST_MODE)
    if [[ "$TEST_MODE" == "1" ]]; then
        log WARN "TEST_MODE enabled - proxy running without browser"
        log INFO "Press Ctrl+C to stop proxy and exit"
        
        # Keep script alive until interrupted
        while true; do
            sleep 1
        done
    else
        launch_browser
    fi
    
    # Step 4: Browser closed, cleanup
    log SUCCESS "Demo completed successfully"
}

# Execute main function
main "$@"
