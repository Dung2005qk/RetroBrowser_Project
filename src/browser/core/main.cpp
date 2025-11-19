// ============================================================================
// main.cpp - Central Orchestrator for Win98 Retro Browser
// ============================================================================
// PURPOSE: Glues UI, Network, Parser modules; manages application lifecycle,
//          data flow (URL → fetch → parse → display), and callback routing.
//          Single-threaded blocking model with UI feedback to mitigate freeze.
//
// DESIGN: No direct UI/network/parser logic here - delegates to modules via
//         public APIs only. Focuses on: init/cleanup, flow orchestration,
//         error handling, state management, and user action responses.
// ============================================================================

#include "stdafx.h"         // PCH: Windows APIs, STL, common macros
#include "ui/ui.h"          // UI module public API
#include "network/network.h" // Network module public API  
#include "parser/parser.h"  // Parser module public API

// ============================================================================
// GLOBAL STATE (Static Lifetime for RAII)
// ============================================================================
// Why static: Auto-init on startup, auto-cleanup on exit via destructors.
// NetworkManager calls WSAStartup in ctor, WSACleanup in dtor.
// HtmlParser is stateless but kept for consistency.

static Network::NetworkManager g_netMgr(PROXY_DEFAULT_HOST, PROXY_DEFAULT_PORT);
static Parser::HtmlParser g_parser;

// Current page state for relative URL resolution
static HWND g_hMainWnd = NULL;
static std::string g_currentURL;  // Last successfully loaded URL

// Optional: Navigation history (extensibility for back/forward)
#ifdef ENABLE_HISTORY
static std::vector<std::string> g_history;
#endif

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================
static std::string ResolveURL(const std::string& base, const std::string& href);

// ============================================================================
// IMAGE LOADING INFRASTRUCTURE
// ============================================================================
// Async image loading to prevent UI freeze. Each image is fetched and decoded
// in a separate worker thread, then results are posted to UI thread via message.

/**
 * @brief Container for image load request passed to worker thread.
 */
struct ImageLoadRequest
{
    HWND hwndTarget;        // Main window to receive result message
    std::string imageUrl;   // Full URL of image to load
    std::string baseUrl;    // Base page URL for relative path resolution
    
    ImageLoadRequest(HWND hwnd, const std::string& url, const std::string& base)
        : hwndTarget(hwnd), imageUrl(url), baseUrl(base) {}
};

/**
 * @brief Worker thread entry point for loading a single image.
 * @param lpParam Pointer to ImageLoadRequest (thread takes ownership, must delete)
 * @return Always 0
 */
static DWORD WINAPI ImageLoadThreadProc(LPVOID lpParam)
{
    ImageLoadRequest* pRequest = (ImageLoadRequest*)lpParam;
    if (!pRequest) {
        return 0;
    }
    
    HBITMAP hBitmap = NULL;
    std::string imageUrl = pRequest->imageUrl;
    HWND hwndTarget = pRequest->hwndTarget;
    
    // Resolve relative URL if needed
    std::string fullUrl = ResolveURL(pRequest->baseUrl, imageUrl);
    
    // Download image data
    Network::HttpResponse resp = g_netMgr.FetchUrl(fullUrl);
    
    if (resp.status == Network::SUCCESS && resp.httpStatusCode == 200) {
        // For Win98 compatibility, we'll use a simplified approach:
        // Only support BMP format natively through Windows API.
        // JPEG/PNG support would require libjpeg/libpng integration
        // which adds significant complexity for VC++6/Win98 target.
        
        // Try to decode as BMP using Windows API
        // Save to temp file and load (Win98 doesn't have CreateDIBSection easily)
        char tempPath[MAX_PATH];
        char tempFile[MAX_PATH];
        
        if (GetTempPathA(MAX_PATH, tempPath) > 0) {
            if (GetTempFileNameA(tempPath, "img", 0, tempFile) != 0) {
                // Write image data to temp file using C FILE* (avoids SetFilePointerEx)
                FILE* fp = fopen(tempFile, "wb");
                if (fp != NULL) {
                    // Use &vector[0] instead of .data() for VC++6.0 compatibility
                    if (!resp.body.empty()) {
                        fwrite(&resp.body[0], 1, resp.body.size(), fp);
                    }
                    fclose(fp);
                    
                    // Try to load as bitmap
                    hBitmap = (HBITMAP)LoadImageA(NULL, tempFile, IMAGE_BITMAP,
                                                   0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
                    
                    // DEBUG: Log if LoadImage failed
                    if (!hBitmap) {
                        DWORD dwError = GetLastError();
                        char szDebug[512];
                        wsprintfA(szDebug, "LoadImageA failed for %s (size=%d bytes, error=%d)\n", 
                                 tempFile, (int)resp.body.size(), dwError);
                        OutputDebugStringA(szDebug);
                    }
                    
                    // Clean up temp file
                    DeleteFileA(tempFile);
                }
            }
        }
    }
    
    // Post result to UI thread (even if NULL to indicate failure)
    if (IsWindow(hwndTarget)) {
        // Allocate URL string on heap for message passing
        char* pUrlCopy = _strdup(fullUrl.c_str());
        PostMessage(hwndTarget, UIM_IMAGE_LOADED, (WPARAM)hBitmap, (LPARAM)pUrlCopy);
    } else {
        // Window closed before we finished - clean up bitmap
        if (hBitmap) {
            DeleteObject(hBitmap);
        }
    }
    
    // Clean up request
    delete pRequest;
    
    return 0;
}

/**
 * @brief Scan parsed page for images and trigger async loading for each.
 * @param blocks Parsed HTML blocks from parser
 * @note Non-blocking: spawns worker threads and returns immediately
 */
static void LoadImagesForPage(const std::vector<Parser::HtmlBlock>& blocks)
{
    if (!g_hMainWnd) {
        return;
    }
    
    // Iterate through blocks and find all images
    for (size_t i = 0; i < blocks.size(); ++i) {
        if (blocks[i].type == Parser::BLOCK_IMG) {
            // Extract image URL from src attribute
            std::map<std::string, std::string>::const_iterator srcIt = 
                blocks[i].attributes.find("src");
            
            if (srcIt != blocks[i].attributes.end() && !srcIt->second.empty()) {
                // Create request for worker thread
                ImageLoadRequest* pRequest = new ImageLoadRequest(
                    g_hMainWnd,
                    srcIt->second,
                    g_currentURL
                );
                
                // Spawn worker thread to load this image
                HANDLE hThread = CreateThread(NULL, 0, ImageLoadThreadProc, 
                                             pRequest, 0, NULL);
                
                if (hThread) {
                    // Detach thread - it will clean itself up
                    CloseHandle(hThread);
                } else {
                    // Thread creation failed - clean up request
                    delete pRequest;
                }
            }
        }
    }
}

// ============================================================================
// FORWARD DECLARATIONS - Callback Implementations
// ============================================================================
void OnNavigate(const TCHAR* pszUrl);
void OnLinkClick(const TCHAR* pszHref);
void OnResize(int nWidth, int nHeight);
void OnClose(void);

// ============================================================================
// HELPER FUNCTIONS - String Conversion & URL Utilities
// ============================================================================

/**
 * @brief Validate URL format and length for safety.
 * @note Silent validation - no error UI, caller decides action.
 */
static BOOL IsValidURL(const TCHAR* pszUrl)
{
    if (!pszUrl || !*pszUrl) return FALSE;
    if (lstrlen(pszUrl) >= MAX_URL_LENGTH) return FALSE;
    
    // Must start with http:// (proxy handles HTTPS backend)
    if (_tcsnicmp(pszUrl, _T("http://"), 7) != 0) return FALSE;
    
    return TRUE;
}

/**
 * @brief Convert TCHAR string to std::string for module APIs.
 * @note Win98 defaults to ANSI build (TCHAR=char), direct copy safe.
 *       Optimized: Use lstrcpyA for explicit ANSI copy, clearer intent.
 */
static std::string TCharToStdString(const TCHAR* pszTChar)
{
#ifdef UNICODE
    // Win98 rare case: convert wchar_t to char (not tested, fallback)
    char buffer[MAX_URL_LENGTH];
    WideCharToMultiByte(CP_ACP, 0, pszTChar, -1, buffer, MAX_URL_LENGTH, NULL, NULL);
    return std::string(buffer);
#else
    // Win98 typical: TCHAR = char, use explicit lstrcpyA for clarity
    // Avoids temporary string creation, more efficient in legacy environment
    char buffer[MAX_URL_LENGTH];
    lstrcpyA(buffer, pszTChar);
    return std::string(buffer);
#endif
}

/**
 * @brief Convert std::string to TCHAR for UI APIs (address bar, etc).
 * @note Allocates static buffer - NOT thread-safe but acceptable for 
 *       single-threaded Win98 design. Trade-off: simplicity vs. thread-safety.
 *       Alternative: return std::vector<TCHAR> or caller-provided buffer,
 *       but adds complexity for minimal benefit in current scope.
 */
static const TCHAR* StdStringToTChar(const std::string& str)
{
    static TCHAR buffer[MAX_URL_LENGTH];
#ifdef UNICODE
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, MAX_URL_LENGTH);
#else
    lstrcpyn(buffer, str.c_str(), MAX_URL_LENGTH);
#endif
    return buffer;
}

/**
 * @brief Extract page title from parsed blocks (first H1 or fallback).
 * @return Title string or "Untitled Page" if not found.
 */
static std::string ExtractPageTitle(const std::vector<Parser::HtmlBlock>& blocks)
{
    // Search for first H1 heading as title
    for (size_t i = 0; i < blocks.size(); ++i) {
        if (blocks[i].type == Parser::BLOCK_H1 && !blocks[i].content.empty()) {
            return blocks[i].content;
        }
    }
    return "Untitled Page";
}

/**
 * @brief Resolve relative URL against current base URL (simple implementation).
 * @note Handles: relative paths, absolute URLs (pass-through), fragments.
 *       Does NOT handle: complex ../../ paths, query params merging.
 */
static std::string ResolveURL(const std::string& base, const std::string& href)
{
    // Absolute URL: starts with http:// or https://
    if (href.find("http://") == 0 || href.find("https://") == 0) {
        return href;
    }
    
    // Fragment-only (#anchor): append to current URL
    if (!href.empty() && href[0] == '#') {
        return base + href;
    }
    
    // Absolute path (starts with /): use base domain only
    if (!href.empty() && href[0] == '/') {
        // Extract protocol + domain from base
        size_t protocolEnd = base.find("://");
        if (protocolEnd != std::string::npos) {
            size_t domainEnd = base.find('/', protocolEnd + 3);
            if (domainEnd != std::string::npos) {
                // Base has path: extract domain only
                return base.substr(0, domainEnd) + href;
            } else {
                // Base is just domain: append path
                return base + href;
            }
        }
        // Fallback: shouldn't happen with valid base
        return base + href;
    }
    
    // Relative path: extract base domain + path
    std::string resolved = base;
    
    // Find last slash to get base path
    size_t lastSlash = resolved.rfind('/');
    if (lastSlash != std::string::npos && lastSlash > 7) { // After http://
        resolved = resolved.substr(0, lastSlash + 1);
    } else {
        resolved += '/';
    }
    
    // Remove leading ./ from relative path
    std::string cleanHref = href;
    if (cleanHref.find("./") == 0) {
        cleanHref = cleanHref.substr(2);
    }
    
    return resolved + cleanHref;
}

/**
 * @brief Convert integer to string (no std::to_string in VC++6.0).
 */
static std::string IntToString(int value)
{
    char buffer[16];
    _itoa(value, buffer, 10);
    return std::string(buffer);
}

// ============================================================================
// CALLBACK IMPLEMENTATIONS - Core Application Logic
// ============================================================================

/**
 * @brief Handle navigation request from user (Go button, Enter, or link click).
 * @details Complete flow: validate → show loading → fetch → parse → display.
 *          All errors handled gracefully with UI feedback and state reset.
 */
void OnNavigate(const TCHAR* pszUrl)
{
    // --- Input Validation ---
    // Why: Prevent crashes from null/empty/oversized URLs; silent ignore
    if (!IsValidURL(pszUrl)) {
        return; // Invalid URL: no action, no error (per spec)
    }
    
    // --- Start Loading State ---
    // Why: Mitigate UI freeze perception during blocking network call
    UI_SetLoading(TRUE);
    SetCursor(LoadCursor(NULL, IDC_WAIT));
    UI_SetStatusText(_T("Connecting to proxy..."));
    
    // Clear previous page before fetch (immediate visual feedback)
    UI_OnPageLoaded(NULL);
    
    // --- Convert & Store URL ---
    std::string url = TCharToStdString(pszUrl);
    g_currentURL = url; // Save for relative link resolution
    
#ifdef ENABLE_HISTORY
    // Optional: Record in history for back/forward navigation
    if (!g_history.empty() && g_history.back() != url) {
        g_history.push_back(url);
    } else if (g_history.empty()) {
        g_history.push_back(url);
    }
#endif
    
    // --- Network Fetch (BLOCKING CALL) ---
    // Why: Single-threaded design avoids Win98 threading complexity
    UI_SetStatusText(_T("Downloading..."));
    Network::HttpResponse resp = g_netMgr.FetchUrl(url);
    
    // Declare parseResult at the beginning to avoid C2362 error with goto
    Parser::ParseResult parseResult;
    
    // --- Handle Network Errors ---
    if (resp.status != Network::SUCCESS) {
        std::string errorMsg = "Network Error: " + resp.errorMessage;
        UI_ShowError(StdStringToTChar(errorMsg));
        UI_SetWindowTitle(_T("Error - Retro Browser")); // Reset title on error
        goto cleanup; // Unified cleanup path
    }
    
    // --- Handle HTTP Errors (non-200 status) ---
    if (resp.httpStatusCode != 200) {
        std::string httpError = "HTTP " + IntToString(resp.httpStatusCode);
        
        // Provide user-friendly messages for common codes
        if (resp.httpStatusCode == 404) httpError += " - Page Not Found";
        else if (resp.httpStatusCode == 500) httpError += " - Server Error";
        else if (resp.httpStatusCode == 403) httpError += " - Access Denied";
        
        UI_ShowError(StdStringToTChar(httpError));
        UI_SetWindowTitle(_T("Error - Retro Browser")); // Reset title on error
        goto cleanup;
    }
    
    // --- Parse HTML (BLOCKING CALL) ---
    UI_SetStatusText(_T("Parsing HTML..."));
    
    // DEBUG: Log HTML size received from proxy
    // DEBUG_LOGF removed for VC++6.0 compatibility
    
    parseResult = g_parser.Parse(resp.body);
    
    // DEBUG: Log parse results
    // DEBUG_LOGF removed for VC++6.0 compatibility
    
    // --- Handle Parse Errors ---
    if (parseResult.status != Parser::PARSE_SUCCESS) {
        std::string parseError = "Parse Error: " + parseResult.errorMessage;
        UI_ShowError(StdStringToTChar(parseError));
        UI_SetWindowTitle(_T("Error - Retro Browser")); // Reset title on error
        goto cleanup;
    }
    
    // --- Success Path: Display Page ---
    {
        // Transfer ownership: heap-allocate ParseResult, UI will free
        // CRITICAL: Must allocate ParseResult* (not vector*) because UI_OnPageLoaded
        // expects ParsedPageData* which is typedef of ParseResult*
        // Previous bug: Allocated vector*, cast to ParseResult* -> type mismatch crash!
        Parser::ParseResult* pPageData = new Parser::ParseResult(parseResult);
        
        // Cast to opaque ParsedPageData* (cleaner than C-style cast in API design)
        UI_OnPageLoaded(reinterpret_cast<const ParsedPageData*>(pPageData));
        
        // Update window title from page content
        std::string title = ExtractPageTitle(parseResult.blocks);
        UI_SetWindowTitle(StdStringToTChar(title));
        
        // Update address bar (may differ from input if redirected)
        UI_SetAddressBar(pszUrl);
        
        // --- Start Async Image Loading ---
        // Spawn worker threads to download images in background
        // Images will appear progressively as they load (non-blocking)
        LoadImagesForPage(parseResult.blocks);
        
        UI_SetStatusText(_T("Done"));
    }
    
cleanup:
    // --- Cleanup: Always Reset Loading State ---
    // Why: Ensure cursor/UI restored on all exit paths (success/error)
    UI_SetLoading(FALSE);
    SetCursor(LoadCursor(NULL, IDC_ARROW));
}

/**
 * @brief Handle hyperlink click in rendered content.
 * @details Resolves relative URLs, updates address bar, reuses OnNavigate.
 */
void OnLinkClick(const TCHAR* pszHref)
{
    // --- Input Validation ---
    if (!pszHref || !*pszHref) {
        return; // Empty href: no action
    }
    
    // --- Resolve Relative URL ---
    std::string href = TCharToStdString(pszHref);
    std::string fullURL;
    
    if (!g_currentURL.empty()) {
        // Resolve against current page base
        fullURL = ResolveURL(g_currentURL, href);
    } else {
        // No base URL: use href as-is (should be absolute)
        fullURL = href;
    }
    
    // --- Update Address Bar ---
    // Why: User sees full URL before navigation starts
    const TCHAR* pszFullURL = StdStringToTChar(fullURL);
    UI_SetAddressBar(pszFullURL);
    
    // --- Reuse Navigation Logic ---
    // Why: Uniform flow for all navigation (button, Enter, link)
    OnNavigate(pszFullURL);
}

/**
 * @brief Handle window resize event.
 * @note Simple implementation: trigger repaint for potential reflow.
 */
void OnResize(int nWidth, int nHeight)
{
    // Optional: Notify renderer of new dimensions for layout reflow
    // Current scope: Just invalidate to repaint content
    UI_InvalidateRender();
}

/**
 * @brief Handle application close request.
 * @note Cleanup performed by static destructors; just post quit message.
 */
void OnClose(void)
{
    // Why: Graceful shutdown via standard message loop exit
    PostQuitMessage(0);
}

// ============================================================================
// APPLICATION ENTRY POINT
// ============================================================================

/**
 * @brief WinMain - Standard Win32 GUI application entry point.
 * @details Three phases: Init (setup modules), Loop (message pump), Shutdown.
 *          Errors in init phase display MessageBox and abort with error code.
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                   LPSTR lpCmdLine, int nCmdShow)
{
    // Suppress unreferenced parameter warnings
    (void)hPrevInstance;
    (void)lpCmdLine;
    
    // --- Enable Memory Leak Detection (Debug Only) ---
    // Why: Automatically detect memory leaks at program exit in VC++6.0
    //      Critical for tracking manual new/delete in C++ Win98 environment
#ifdef _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    
    // ========== PHASE 1: INITIALIZATION ==========
    
    // --- Check Network Module ---
    // Why: App is useless without network; fail fast with clear message
    if (!g_netMgr.IsInitialized()) {
        MessageBox(NULL, 
                   _T("Failed to initialize Winsock 2.2.\n\n")
                   _T("Please check:\n")
                   _T("- Network adapter is enabled\n")
                   _T("- TCP/IP protocol is installed\n")
                   _T("- ws2_32.dll is present in System32"),
                   _T("Fatal Error - Network Initialization"),
                   MB_ICONERROR | MB_OK);
        return 1;
    }
    
    // --- Setup Callback Table ---
    // Why: Decouples UI module from main.cpp implementation details
    UI_CALLBACKS callbacks;
    callbacks.pfnOnNavigate  = OnNavigate;
    callbacks.pfnOnLinkClick = OnLinkClick;
    callbacks.pfnOnResize    = OnResize;
    callbacks.pfnOnClose     = OnClose;
    
    // --- Initialize UI Module ---
    // Why: Register window class and prepare UI subsystem
    if (!UI_Init(hInstance, &callbacks)) {
        MessageBox(NULL, 
                   _T("Failed to initialize UI module.\n\n")
                   _T("Window class registration failed."),
                   _T("Fatal Error - UI Initialization"),
                   MB_ICONERROR | MB_OK);
        return 2;
    }
    
    // --- Create Main Window ---
    g_hMainWnd = UI_CreateMainWindow(nCmdShow);
    if (!g_hMainWnd) {
        MessageBox(NULL,
                   _T("Failed to create main browser window.\n\n")
                   _T("CreateWindowEx returned NULL."),
                   _T("Fatal Error - Window Creation"),
                   MB_ICONERROR | MB_OK);
        UI_Shutdown(); // Cleanup registered class
        return 3;
    }
    
    // ========== PHASE 2: MESSAGE LOOP (BLOCKING) ==========
    
    // Why: Standard Win32 message pump; runs until WM_QUIT posted
    int exitCode = UI_RunMessageLoop();
    
    // ========== PHASE 3: SHUTDOWN ==========
    
    // Cleanup UI resources (free page data, destroy controls, unregister class)
    // Why: Explicit UI shutdown before static destructors for controlled cleanup
    UI_Shutdown();
    
    // Note: g_netMgr and g_parser destructors run automatically here
    // NetworkManager dtor calls WSACleanup(), releases Winsock resources
    
    return exitCode;
}

// ============================================================================
// END OF FILE
// ============================================================================
// MAINTENANCE NOTES:
// - Add features via new callbacks in UI_CALLBACKS struct (back/forward/print)
// - Extend history with g_history vector (already ifdef-ready)
// - URL validation can be enhanced (regex patterns, punycode support)
// - Error messages can be localized (resource strings instead of hardcoded)
// - Threading can be added: move FetchUrl to worker thread, PostMessage result
//
// TESTING CHECKLIST:
// [ ] Launch without proxy running → Network init error dialog
// [ ] Navigate to invalid URL (no http://) → Silent ignore
// [ ] Navigate to valid URL → Success flow with loading feedback
// [ ] Network error (proxy crash) → Clear error message + cursor reset
// [ ] HTTP 404 response → User-friendly error with code
// [ ] Parse error (malformed HTML) → Error display + state reset
// [ ] Click relative link (/page.html) → Resolves correctly
// [ ] Click absolute link (http://other.com) → Navigates directly
// [ ] Resize window → Content repaints
// [ ] Close window → Clean exit, no leaks (check Task Manager)
// ============================================================================