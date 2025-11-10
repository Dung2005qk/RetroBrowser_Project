#pragma once
// ============================================================================
// ui.h - Public API for Win98 Retro Browser UI Module
// ============================================================================
// PURPOSE: Stable interface contract between UI subsystem and other modules
//          (core/main.cpp, network, parser, renderer). Provides lifecycle
//          management, state updates, and callback-driven event routing for
//          decoupled architecture. Hides ALL implementation details (child
//          window handles, internal state, message handling logic) in ui.cpp.
//
// DESIGN PRINCIPLES:
//          - ENCAPSULATION: No global state exposure, opaque implementation
//            (UIState hidden, control IDs private, PIMPL idiom throughout)
//          - DECOUPLING: Callback table + custom messages for async comm
//            (separate OnNavigate/OnLinkClick for flexible event handling)
//          - TYPE SAFETY: STRICT handles, const correctness, explicit buffers
//          - BACKWARD COMPAT: Win98 APIs only, no post-2000 features
//          - SINGLE RESPONSIBILITY: UI manages windows/events, delegates logic
//          - MINIMAL DEPS: Only <windows.h>, independent of project headers
//
// USAGE: This header is self-contained (no stdafx.h dependency for flexibility).
//        Implementation files (ui.cpp) must include stdafx.h first per PCH rules.
//        Call UI_Init() with callback table before UI_CreateMainWindow(). Post
//        custom UIM_* messages from worker threads to notify UI of async events.
//        UI invokes callbacks on user actions. See ui.cpp for implementation.
// ============================================================================


// ============================================================================
// SECTION 1: REQUIRED DEPENDENCIES
// ============================================================================
// Minimal Win32 API types for public interface (HWND, TCHAR, etc.).
// Implementation files (ui.cpp) must include stdafx.h first per PCH rules.
// This header remains independent for maximum portability and recompilation.

#include <windows.h>            // Core Win32 types: HWND, BOOL, TCHAR, LPARAM, etc.


// ============================================================================
// SECTION 2: FORWARD DECLARATIONS
// ============================================================================
// Decouple from parser module by forward declaring data structures.
// Avoids #include "../parser/parser.h" to minimize dependencies and
// compilation cascades. Full definition only needed in ui.cpp.

struct ParsedPageData;      // Defined in parser/parser.h, opaque here
                             // Contains parsed HTML tree for rendering


// ============================================================================
// SECTION 3: CUSTOM WINDOW MESSAGES
// ============================================================================
// Async communication protocol from background threads (network, parser) to
// UI thread. Prevents race conditions and cross-thread resource access.
// Post these via PostMessage(UI_GetMainWindowHandle(), UIM_*, ...).
//
// MESSAGE CONTRACT:
// - wParam: Operation status/flags (BOOL success, int code, etc.)
// - lParam: Payload pointer (must be heap-allocated, UI frees after handling)
//           OR integer data for lightweight messages
// - Thread-safe: Only UI thread processes these in WndProc

/** @brief User initiated navigation via Go button or Enter key.
 *  @details wParam: Reserved (0)
 *           lParam: const TCHAR* URL from address bar (stack-safe copy)
 *  @note Triggers callback: UI_CALLBACKS::onNavigate */
#define UIM_NAVIGATE_REQUEST    (WM_APP + 1)

/** @brief Network module completed HTTP download.
 *  @details wParam: BOOL success (TRUE = valid HTML, FALSE = error)
 *           lParam: char* rawHTML (heap ptr, UI frees; NULL on failure)
 *  @note UI forwards to parser module via core logic */
#define UIM_DOWNLOAD_COMPLETE   (WM_APP + 2)

/** @brief Parser module finished HTML parsing.
 *  @details wParam: Reserved (0)
 *           lParam: ParsedPageData* pData (heap ptr, UI stores/frees)
 *  @note Triggers InvalidateRect for renderer to repaint */
#define UIM_PARSING_COMPLETE    (WM_APP + 3)

/** @brief Request immediate repaint of content area.
 *  @details wParam: BOOL bForceFull (TRUE = entire client, FALSE = dirty rect)
 *           lParam: Reserved (0)
 *  @note Posted by renderer or external modules after updates */
#define UIM_RENDER_REQUEST      (WM_APP + 4)

/** @brief User clicked hyperlink in rendered content.
 *  @details wParam: Reserved (0)
 *           lParam: const TCHAR* href (heap ptr, UI frees after callback)
 *  @note Triggers callback: UI_CALLBACKS::onLinkClick */
#define UIM_LINK_CLICKED        (WM_APP + 5)

/** @brief Update status bar text from background operation.
 *  @details wParam: Reserved (0)
 *           lParam: const TCHAR* szStatus (heap ptr, UI frees after display)
 *  @note Thread-safe alternative to direct UI_SetStatusText call */
#define UIM_STATUS_UPDATE       (WM_APP + 6)


// ============================================================================
// SECTION 4: CALLBACK FUNCTION TYPES
// ============================================================================
// Function pointer signatures for UI to notify core module of user actions.
// Core populates UI_CALLBACKS struct and passes to UI_Init(). UI invokes
// these on main thread during message processing (no threading concerns).
//
// LIFETIME: Callback pointers must remain valid for app lifetime.
// THREADING: Always called on UI thread (safe to access module state).
// ERROR HANDLING: Callbacks should not throw (C code, no exceptions).

/** @brief User requested navigation (Go button, Enter, or link click).
 *  @param pszUrl Target URL from address bar or href attribute.
 *                Valid TCHAR string, null-terminated, lifetime: call scope.
 *  @note Core should validate URL, initiate network download, update UI state. */
typedef void (*OnNavigateCallback)(const TCHAR* pszUrl);

/** @brief User clicked hyperlink in rendered page content.
 *  @param pszHref Link target from <a href="..."> attribute.
 *                 Valid TCHAR string, null-terminated, lifetime: call scope.
 *  @note Core should update address bar and navigate (may call OnNavigateCallback). */
typedef void (*OnLinkClickCallback)(const TCHAR* pszHref);

/** @brief Main window resized by user (drag border, maximize, etc.).
 *  @param nWidth New client area width in pixels.
 *  @param nHeight New client area height in pixels.
 *  @note Core should notify renderer to reflow layout if needed. */
typedef void (*OnResizeCallback)(int nWidth, int nHeight);

/** @brief User requested application close (Alt+F4, X button, File->Exit).
 *  @note Core should cleanup resources, save state, call UI_Shutdown(), exit. */
typedef void (*OnCloseCallback)(void);


// ============================================================================
// SECTION 5: CALLBACK TABLE STRUCTURE
// ============================================================================
// Aggregates all callback pointers for single-pass initialization.
// Extensible design: add new callbacks without breaking API (check for NULL).

/** @brief Callback dispatch table for UI -> Core communication.
 *  @details Populate all fields before passing to UI_Init(). UI stores pointer
 *           internally (no copy), so struct must have static/global lifetime.
 *           Future extensions: onHistoryBack, onBookmark, onPrint, etc. */
struct UI_CALLBACKS
{
    OnNavigateCallback  pfnOnNavigate;      ///< Required: handle navigation requests
    OnLinkClickCallback pfnOnLinkClick;     ///< Required: handle hyperlink clicks
    OnResizeCallback    pfnOnResize;        ///< Optional: handle window resize (can be NULL)
    OnCloseCallback     pfnOnClose;         ///< Required: handle graceful shutdown

    // --- Future Extensibility (v2.0+) ---
    // OnHistoryBackCallback pfnOnHistoryBack;
    // OnBookmarkAddCallback pfnOnBookmarkAdd;
    // OnPrintCallback pfnOnPrint;
};


// ============================================================================
// SECTION 6: PUBLIC API - LIFECYCLE MANAGEMENT
// ============================================================================
// Module initialization, window creation, message loop, and cleanup.
// Call in strict order: Init -> CreateMainWindow -> RunMessageLoop -> Shutdown.

/** @brief Initialize UI module and register window class.
 *  @param hInstance Application instance handle from WinMain.
 *  @param pCallbacks Pointer to populated callback table (static lifetime).
 *  @return TRUE on success, FALSE if registration fails or invalid params.
 *  @note Must be called ONCE before UI_CreateMainWindow(). Stores callbacks
 *        internally for later dispatch. Does NOT create windows yet. */
BOOL UI_Init(HINSTANCE hInstance, const UI_CALLBACKS* pCallbacks);

/** @brief Create main browser window and child controls.
 *  @param nCmdShow Initial window display state (SW_SHOW, SW_MAXIMIZE, etc.).
 *  @return Main window HWND on success, NULL on creation failure.
 *  @note Creates address bar (Edit), Go button, status bar, content area.
 *        Window is initially empty (no page loaded). Call after UI_Init(). */
HWND UI_CreateMainWindow(int nCmdShow);

/** @brief Run standard Win32 message loop until WM_QUIT.
 *  @return Exit code from WM_QUIT wParam (typically PostQuitMessage arg).
 *  @note Blocking call: does not return until application closes.
 *        Dispatches messages to UI_WndProc. Call after UI_CreateMainWindow(). */
int UI_RunMessageLoop(void);

/** @brief Cleanup UI resources and unregister window class.
 *  @note Call once before application exit, after message loop returns.
 *        Destroys windows (if not already closed), frees allocated memory,
 *        releases parsed page data. Safe to call multiple times (idempotent). */
void UI_Shutdown(void);


// ============================================================================
// SECTION 7: PUBLIC API - STATE MUTATORS
// ============================================================================
// Thread-safe functions to update UI state from core/modules.
// Safe to call from UI thread only (post UIM_* messages from worker threads).

/** @brief Set main window title bar text.
 *  @param pszTitle Title to display (typically page title from <title> tag).
 *                  Null-terminated TCHAR string. NULL resets to default app name.
 *  @note Common pattern: UI_SetWindowTitle(parsedData->pageTitle) after parsing.
 *        Win98 title bar truncates at ~260 chars automatically. */
void UI_SetWindowTitle(const TCHAR* pszTitle);

/** @brief Update status bar text (bottom of window).
 *  @param pszText Status message to display (e.g., "Loading...", "Done").
 *                 Null-terminated TCHAR string. NULL clears status bar.
 *  @note Truncates to status bar width (~200 chars). Does not persist across
 *        navigation. Safe to call repeatedly (no flicker suppression). */
void UI_SetStatusText(const TCHAR* pszText);

/** @brief Set address bar text (URL input field).
 *  @param pszUrl URL to display. Null-terminated TCHAR string. NULL clears field.
 *  @note Does NOT trigger navigation (use UIM_NAVIGATE_REQUEST for that).
 *        Typically called after successful navigation to update display URL.
 *        Safe from UI thread only. */
void UI_SetAddressBar(const TCHAR* pszUrl);

/** @brief Toggle loading state visual indicators.
 *  @param bLoading TRUE = show busy cursor + "Loading..." status,
 *                  FALSE = restore normal cursor + clear status.
 *  @note Call TRUE when network starts download, FALSE when complete/error.
 *        Does not disable address bar (user can cancel/navigate away). */
void UI_SetLoading(BOOL bLoading);

/** @brief Store parsed page data and trigger repaint.
 *  @param pData Pointer to ParsedPageData from parser module (heap-allocated).
 *               UI takes ownership, frees on next navigation or shutdown.
 *               NULL clears current page (blank content area).
 *  @note Invalidates content area to trigger WM_PAINT -> renderer draw.
 *        Does not update address bar (caller's responsibility). */
void UI_OnPageLoaded(const ParsedPageData* pData);


// ============================================================================
// SECTION 8: PUBLIC API - STATE ACCESSORS
// ============================================================================
// Query current UI state for core logic decisions.

/** @brief Get main window handle for message posting.
 *  @return HWND of main window, or NULL if not yet created (before UI_CreateMainWindow).
 *  @note Use for PostMessage(UI_GetMainWindowHandle(), UIM_*, ...) from threads.
 *        Do NOT use for SendMessage (blocks worker threads). */
HWND UI_GetMainWindowHandle(void);

/** @brief Get render area window handle for Renderer module.
 *  @return HWND of content area for direct rendering (may be main window client
 *          or dedicated child window depending on implementation).
 *  @note Renderer should use this handle for GetDC/BeginPaint operations.
 *        Abstraction allows future migration to child window without API change.
 *        Returns NULL if window not yet created. */
HWND UI_GetRenderAreaHWND(void);

/** @brief Retrieve current address bar text into buffer.
 *  @param pszBuffer Destination buffer for URL text (TCHAR array).
 *  @param nBufferSize Size of buffer in TCHARs (including null terminator).
 *  @note Safe against buffer overflow: truncates to nBufferSize-1 + null.
 *        Returns empty string if address bar empty or invalid params.
 *        Typical usage: TCHAR url[MAX_URL_LENGTH]; UI_GetAddressBarText(url, MAX_URL_LENGTH); */
void UI_GetAddressBarText(TCHAR* pszBuffer, int nBufferSize);


// ============================================================================
// SECTION 9: PUBLIC API - UTILITY FUNCTIONS
// ============================================================================
// Helper functions for common UI operations.

/** @brief Display modal error dialog with OK button.
 *  @param pszMessage Error message to show. Null-terminated TCHAR string.
 *                    NULL displays generic "An error occurred" message.
 *  @note Blocks until user dismisses (MB_OK | MB_ICONERROR style).
 *        Safe to call from UI thread only. For thread-safe errors, post
 *        UIM_STATUS_UPDATE or custom error message. */
void UI_ShowError(const TCHAR* pszMessage);

/** @brief Request immediate repaint of content rendering area.
 *  @note Calls InvalidateRect on content area, triggering WM_PAINT.
 *        Renderer module handles actual drawing during paint. Use after
 *        external state changes (zoom, theme, etc.) that don't auto-invalidate. */
void UI_InvalidateRender(void);


// ============================================================================
// SECTION 10: WINDOW PROCEDURE
// ============================================================================
// Main message dispatcher registered with WNDCLASSEX.

/** @brief Main window procedure for browser window.
 *  @param hWnd Window handle receiving message.
 *  @param uMsg Message identifier (WM_*, UIM_*, etc.).
 *  @param wParam Message-specific parameter (depends on uMsg).
 *  @param lParam Message-specific parameter (depends on uMsg).
 *  @return Message-specific return value (0 for handled, DefWindowProc for unhandled).
 *  @note Registered via UI_Init(). Routes UI events to callbacks, processes
 *        custom UIM_* messages, delegates painting to renderer. Do NOT call
 *        directly (invoked by message loop). */
LRESULT CALLBACK UI_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


// ============================================================================
// SECTION 11: EXTENSIBILITY HOOKS (FUTURE FEATURES)
// ============================================================================
// Placeholder declarations for planned features in future versions.
// Currently unimplemented (no definitions in ui.cpp).

// --- Navigation History (v1.1) ---
// typedef void (*OnHistoryBackCallback)(void);
// typedef void (*OnHistoryForwardCallback)(void);
// void UI_EnableHistoryButtons(BOOL bBackEnabled, BOOL bForwardEnabled);

// --- Bookmarks (v1.2) ---
// typedef void (*OnBookmarkAddCallback)(const TCHAR* pszUrl, const TCHAR* pszTitle);
// void UI_ShowBookmarksDialog(void);

// --- Page Search (v1.3) ---
// typedef void (*OnFindTextCallback)(const TCHAR* pszQuery, BOOL bNext);
// void UI_ShowFindDialog(void);

// --- Content Context Menu (v1.4) ---
// typedef void (*OnContextMenuCallback)(int x, int y, const TCHAR* pszLinkHref);
// void UI_ShowContentContextMenu(int x, int y, HMENU hMenu);


// ============================================================================
// END OF PUBLIC API
// ============================================================================
// IMPLEMENTATION NOTES (for ui.cpp):
// - Store UI state in static/file-scope struct (no extern globals)
// - Child window handles (hAddressBar, hGoButton, hStatusBar) are private
// - ParsedPageData stored as static pointer, freed on navigation/shutdown
// - Callbacks invoked via stored UI_CALLBACKS* pointer
// - WndProc uses switch/case for messages, delegates to helper functions
// - All heap allocations from UIM_* lParam must be freed after handling
// - Use TextOut/DrawText for content rendering via renderer module
// - Implement double-buffering if flicker occurs on Win98 (use CreateCompatibleDC)
//
// TESTING CHECKLIST:
// - Verify no leaks with heap allocations (use _CrtDumpMemoryLeaks in debug)
// - Test null callback pointers (crash prevention)
// - Validate buffer overflows in UI_GetAddressBarText
// - Check thread-safety of PostMessage vs direct calls
// - Confirm Win98 compatibility (no XP+ APIs via Dependency Walker)
// ============================================================================
