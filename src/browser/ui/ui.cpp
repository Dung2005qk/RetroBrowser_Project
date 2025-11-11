// ============================================================================
// ui.cpp - Implementation of the UI Module for Win98 Retro Browser
//
// PURPOSE: Manages the main window, child controls, user events, and acts as
// the central dispatcher between the user and browser core modules.
// It implements a responsive, flicker-free, retro-themed UI consistent
// with the Windows 98 aesthetic.
// ============================================================================

#include "../core/stdafx.h"       // Precompiled header, MUST be first.
#include "ui.h"                   // Our public API contract.
#include "../renderer/renderer.h" // For painting delegation and hit-testing.
#include "../parser/parser.h"     // For ParseResult structure.
#include <commctrl.h>

#pragma comment(lib, "comctl32.lib")

// --- Constants ---
// Use control IDs from resource.h (already included via stdafx.h)
#define IDC_ADDRESS_BAR      IDC_ADDRESS_EDIT    // 400
#define IDC_STATUS_BAR       IDC_STATUS_STATIC   // 402
// IDC_GO_BUTTON already defined in resource.h as 401
#define UI_PADDING           5
#define ADDRESS_BAR_HEIGHT   24
#define GO_BUTTON_WIDTH      60
#define STATUS_BAR_HEIGHT    22

// --- Scrolling Constants ---
#define SCROLL_LINE_AMOUNT   20  // Pixels per scroll line
#define MOUSE_WHEEL_LINES    3   // Lines to scroll per wheel notch

// --- Internal State ---
// Encapsulates all module-level state variables into a single struct for clarity.
static struct UIState {
    HINSTANCE                   hInstance;
    HWND                        hMainWnd;
    HWND                        hAddressBar;
    HWND                        hGoButton;
    HWND                        hStatusBar;
    HWND                        hRenderArea; // In this design, it's the same as hMainWnd
    const UI_CALLBACKS*         pCallbacks;
    ParsedPageData*             pCurrentPage;  // Typedef to Parser::ParseResult
    Renderer::HtmlRenderer*     pRenderer;     // Renderer instance - full encapsulation
    HFONT                       hDefaultFont;
    BOOL                        isLoading;
} g_State = {};

// --- Helper Function Forward Declarations ---
static void CreateControls(HWND hParent);
static void RepositionControls(HWND hParent);
static void HandleNavigate(HWND hWnd);
static RECT GetRenderAreaRect(void);
static void ExtractAndSetPageTitle(const ParsedPageData* pData);
static LRESULT CALLBACK AddressSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static WNDPROC g_pfnOldAddressProc = NULL;

// ============================================================================
// SECTION 1: PUBLIC API IMPLEMENTATION
// ============================================================================

BOOL UI_Init(HINSTANCE hInstance, const UI_CALLBACKS* pCallbacks) {
    // Store instance handle and callbacks, validate parameters.
    if (!hInstance || !pCallbacks) return FALSE;
    g_State.hInstance = hInstance;
    g_State.pCallbacks = pCallbacks;

    // Register the main window class with Win98 aesthetics.
    WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
    wcex.style         = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc   = UI_WndProc;
    wcex.hInstance     = hInstance;
    wcex.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON));
    wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = _T("Win98RetroBrowserWndClass");
    wcex.hIconSm       = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_APP_ICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    
    return RegisterClassEx(&wcex);
}

HWND UI_CreateMainWindow(int nCmdShow) {
    // Create the main window with WS_CLIPCHILDREN to prevent flicker.
    g_State.hMainWnd = CreateWindowEx(
        0, _T("Win98RetroBrowserWndClass"), _T(APP_NAME),
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, NULL, g_State.hInstance, NULL);

    if (!g_State.hMainWnd) {
        DEBUG_LOG("CreateWindowEx failed!");
        return NULL;
    }

    // In our single-window design, the render area is the main window itself.
    g_State.hRenderArea = g_State.hMainWnd;

    ShowWindow(g_State.hMainWnd, nCmdShow);
    UpdateWindow(g_State.hMainWnd);
    
    return g_State.hMainWnd;
}

int UI_RunMessageLoop(void) {
    // Standard Win32 message loop.
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

void UI_Shutdown(void) {
    // Clean up all resources in proper order.
    SAFE_DELETE(g_State.pCurrentPage);
    SAFE_DELETE(g_State.pRenderer);  // Clean up renderer
    if (g_State.hDefaultFont) DeleteObject(g_State.hDefaultFont);
    UnregisterClass(_T("Win98RetroBrowserWndClass"), g_State.hInstance);
    
    // Zero out the state struct for safety.
    memset(&g_State, 0, sizeof(UIState));
}

void UI_SetWindowTitle(const TCHAR* pszTitle) {
    // Set main window title, fallback to app name if title is null.
    SetWindowText(g_State.hMainWnd, pszTitle && *pszTitle ? pszTitle : _T(APP_NAME));
}

void UI_SetStatusText(const TCHAR* pszText) {
    // Update the status bar text (thread-safe as SendMessage is).
    SendMessage(g_State.hStatusBar, SB_SETTEXT, 0, (LPARAM)(pszText ? pszText : _T("")));
}

void UI_SetAddressBar(const TCHAR* pszUrl) {
    // Set the address bar text and select all for easy re-typing.
    SetWindowText(g_State.hAddressBar, pszUrl);
    if (pszUrl && *pszUrl) {
        SendMessage(g_State.hAddressBar, EM_SETSEL, 0, -1);
    }
}

void UI_SetLoading(BOOL bLoading) {
    // Manage UI state during loading process for better UX.
    if (g_State.isLoading == bLoading) return;
    g_State.isLoading = bLoading;
    
    SetCursor(LoadCursor(NULL, bLoading ? IDC_WAIT : IDC_ARROW));
    UI_SetStatusText(bLoading ? _T("Loading page...") : _T("Done"));
    
    EnableWindow(g_State.hAddressBar, !bLoading);
    EnableWindow(g_State.hGoButton, !bLoading);
    
    if (!bLoading) SetFocus(g_State.hAddressBar);
}

void UI_OnPageLoaded(const ParsedPageData* pData) {
    // Take ownership of new page data, release old, and trigger repaint.
    SAFE_DELETE(g_State.pCurrentPage);
    g_State.pCurrentPage = (ParsedPageData*)pData;

    // Extract and set page title using helper function for better separation
    ExtractAndSetPageTitle(g_State.pCurrentPage);
    
    // Integrate with renderer: set content and calculate layout
    if (g_State.pCurrentPage && g_State.pRenderer) {
        // Set new content to renderer
        g_State.pRenderer->SetContent(g_State.pCurrentPage->blocks);
        
        // Calculate layout based on current window size
        RECT rcRender = GetRenderAreaRect();
        g_State.pRenderer->CalculateLayout(g_State.hMainWnd, rcRender);
        
        // Reset scroll position to top for new page
        g_State.pRenderer->OnScroll(0);
        
        // Update scrollbar information using render area dimensions
        int contentHeight = g_State.pRenderer->GetTotalContentHeight();
        int viewHeight = rcRender.bottom - rcRender.top;
        
        if (contentHeight <= viewHeight) {
            // Content fits in viewport - hide scrollbar
            ShowScrollBar(g_State.hMainWnd, SB_VERT, FALSE);
        } else {
            // Content requires scrolling - show and configure scrollbar
            ShowScrollBar(g_State.hMainWnd, SB_VERT, TRUE);
            SCROLLINFO si = { sizeof(SCROLLINFO) };
            si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
            si.nMin = 0;
            si.nMax = contentHeight - 1; // 0-indexed as per MSDN
            si.nPage = viewHeight;
            si.nPos = 0; // Reset to top
            SetScrollInfo(g_State.hMainWnd, SB_VERT, &si, TRUE);
        }
    }
    
    UI_InvalidateRender();
}

HWND UI_GetMainWindowHandle(void) { return g_State.hMainWnd; }
HWND UI_GetRenderAreaHWND(void) { return g_State.hRenderArea; }

void UI_GetAddressBarText(TCHAR* pszBuffer, int nBufferSize) {
    // Safely retrieve text from address bar.
    GetWindowText(g_State.hAddressBar, pszBuffer, nBufferSize);
    // Ensure null termination even if GetWindowText truncates.
    if (nBufferSize > 0) pszBuffer[nBufferSize - 1] = _T('\0');
}

void UI_ShowError(const TCHAR* pszMessage) {
    // Display a modal error dialog to the user.
    MessageBox(g_State.hMainWnd,
               pszMessage ? pszMessage : _T("An unspecified error occurred."),
               _T("Error"), MB_OK | MB_ICONERROR);
}

void UI_InvalidateRender(void) {
    // Request a repaint of the content area.
    InvalidateRect(g_State.hRenderArea, NULL, TRUE);
}

// ============================================================================
// SECTION 2: MAIN WINDOW PROCEDURE
// ============================================================================

LRESULT CALLBACK UI_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    switch (uMsg) {
        case WM_CREATE: {
            // Create the standard Win98 UI font.
            g_State.hDefaultFont = CreateFont(-MulDiv(8, GetDeviceCaps(GetDC(hWnd), LOGPIXELSY), 72), 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, _T("MS Sans Serif"));
            if (!g_State.hDefaultFont) g_State.hDefaultFont = (HFONT)GetStockObject(SYSTEM_FONT);
            
            CreateControls(hWnd); // Create address bar, button, status bar.
            
            // Subclass address bar for Enter key handling
            g_pfnOldAddressProc = (WNDPROC)SetWindowLong(g_State.hAddressBar, GWL_WNDPROC, (LONG)AddressSubclassProc);
            
            SetFocus(g_State.hAddressBar); // Initial focus for immediate typing.
            
            // Initialize renderer and store in state with proper error handling
            g_State.pRenderer = new (std::nothrow) Renderer::HtmlRenderer();
            if (!g_State.pRenderer) {
                // Critical: Out of memory - cannot continue
                MessageBox(hWnd, 
                          _T("Critical Error: Out of memory.\nCannot create renderer.\nApplication will exit."),
                          _T("Fatal Error"), 
                          MB_OK | MB_ICONERROR);
                return -1; // Signal CreateWindowEx to fail
            }
            
            // Initialize renderer and check for errors
            Renderer::RenderResult initResult = g_State.pRenderer->Initialize(hWnd);
            if (initResult.status != Renderer::RENDER_SUCCESS) {
                // Renderer initialization failed - convert error message to TCHAR
                #ifdef UNICODE
                    int len = MultiByteToWideChar(CP_UTF8, 0, initResult.errorMessage.c_str(), -1, NULL, 0);
                    if (len > 0) {
                        std::vector<TCHAR> errMsg(len);
                        MultiByteToWideChar(CP_UTF8, 0, initResult.errorMessage.c_str(), -1, &errMsg[0], len);
                        MessageBox(hWnd, &errMsg[0], _T("Renderer Initialization Failed"), MB_OK | MB_ICONERROR);
                    }
                #else
                    MessageBox(hWnd, initResult.errorMessage.c_str(), _T("Renderer Initialization Failed"), MB_OK | MB_ICONERROR);
                #endif
                
                delete g_State.pRenderer;
                g_State.pRenderer = NULL;
                return -1; // Signal failure
            }
            
            // Initialize scrollbar
            SCROLLINFO si = { sizeof(SCROLLINFO) };
            si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
            si.nMin = 0;
            si.nMax = 0;
            si.nPage = 0;
            si.nPos = 0;
            SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
            
            return 0;
        }

        case WM_SIZE: {
            // Reposition child controls on window resize.
            RepositionControls(hWnd);
            if (g_State.pCallbacks && g_State.pCallbacks->pfnOnResize) {
                g_State.pCallbacks->pfnOnResize(LOWORD(lParam), HIWORD(lParam));
            }
            
            // Notify renderer and recalculate layout
            if (g_State.pRenderer) {
                RECT rcRender = GetRenderAreaRect();
                g_State.pRenderer->OnResize(hWnd);
                g_State.pRenderer->CalculateLayout(hWnd, rcRender);
                
                // Update scrollbar based on new dimensions using render area
                int contentHeight = g_State.pRenderer->GetTotalContentHeight();
                int viewHeight = rcRender.bottom - rcRender.top;
                
                if (contentHeight <= viewHeight) {
                    // Content fits - hide scrollbar
                    ShowScrollBar(hWnd, SB_VERT, FALSE);
                } else {
                    // Content requires scrolling - show and update scrollbar
                    ShowScrollBar(hWnd, SB_VERT, TRUE);
                    SCROLLINFO si = { sizeof(SCROLLINFO) };
                    si.fMask = SIF_RANGE | SIF_PAGE;
                    si.nMin = 0;
                    si.nMax = contentHeight - 1; // 0-indexed as per MSDN
                    si.nPage = viewHeight;
                    SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
                }
            }
            
            return 0;
        }

        case WM_COMMAND: {
            // Handle clicks on child controls.
            if (LOWORD(wParam) == IDC_GO_BUTTON && HIWORD(wParam) == BN_CLICKED) {
                HandleNavigate(hWnd);
            }
            return 0;
        }

        case WM_PAINT: {
            // Delegate all painting to the renderer for performance and correctness.
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            
            // Renderer uses internally cached scroll position from OnScroll()
            // No need to pass scroll info here - renderer maintains its own state
            if (g_State.pRenderer) {
                g_State.pRenderer->Render(hWnd, hdc, ps.rcPaint);
            }
            
            EndPaint(hWnd, &ps);
            return 0;
        }

        case WM_ERASEBKGND: {
            // Return 1 to prevent system background erase, eliminating flicker.
            return 1;
        }
        
        case WM_SETCURSOR: {
            // Change cursor to a hand if hovering over a link.
            // IDC_HAND is not available in older Windows, use MAKEINTRESOURCE for compatibility
            if (g_State.pRenderer) {
                POINT pt;
                GetCursorPos(&pt);
                ScreenToClient(hWnd, &pt);
                std::string dummyUrl;
                if (g_State.pRenderer->HandleClick(pt.x, pt.y, dummyUrl)) {
                     // Use IDC_HAND (32649) for hand cursor, fallback to system default if not available
                     HCURSOR hHandCursor = LoadCursor(NULL, MAKEINTRESOURCE(32649));
                     if (!hHandCursor) hHandCursor = LoadCursor(NULL, IDC_ARROW);
                     SetCursor(hHandCursor);
                     return TRUE; // We handled this message.
                }
            }
            // Let DefWindowProc handle it for other areas (arrow, I-beam, etc.).
            break; 
        }

        case WM_LBUTTONDOWN: {
            // Handle hyperlink clicks.
            if (g_State.pRenderer) {
                std::string href;
                if (g_State.pRenderer->HandleClick(LOWORD(lParam), HIWORD(lParam), href) && !href.empty()) {
                    // Post message with a heap-allocated copy of the URL.
                    #ifdef UNICODE
                        int len = MultiByteToWideChar(CP_UTF8, 0, href.c_str(), -1, NULL, 0);
                        TCHAR* urlCopy = new TCHAR[len];
                        MultiByteToWideChar(CP_UTF8, 0, href.c_str(), -1, urlCopy, len);
                        PostMessage(hWnd, UIM_LINK_CLICKED, 0, (LPARAM)urlCopy);
                    #else
                        PostMessage(hWnd, UIM_LINK_CLICKED, 0, (LPARAM)_strdup(href.c_str()));
                    #endif
                }
            }
            return 0;
        }

        case WM_VSCROLL: {
            // Handle vertical scrollbar events for page scrolling.
            if (!g_State.pRenderer) return 0;
            
            SCROLLINFO si = { sizeof(SCROLLINFO) };
            si.fMask = SIF_ALL;
            GetScrollInfo(hWnd, SB_VERT, &si);
            
            int yPos = si.nPos;
            
            switch (LOWORD(wParam)) {
                case SB_TOP:        yPos = si.nMin; break;
                case SB_BOTTOM:     yPos = si.nMax; break;
                case SB_LINEUP:     yPos -= SCROLL_LINE_AMOUNT; break;
                case SB_LINEDOWN:   yPos += SCROLL_LINE_AMOUNT; break;
                case SB_PAGEUP:     yPos -= si.nPage; break;
                case SB_PAGEDOWN:   yPos += si.nPage; break;
                case SB_THUMBTRACK: yPos = si.nTrackPos; break;
            }
            
            // Clamp to valid range with safe bounds check
            // Per MSDN: max position = nMax - nPage + 1
            int maxPos = si.nMax - (int)si.nPage + 1;
            if (maxPos < si.nMin) maxPos = si.nMin; // Prevent negative when content < view
            yPos = max(si.nMin, min(yPos, maxPos));
            
            if (yPos != si.nPos) {
                si.fMask = SIF_POS;
                si.nPos = yPos;
                SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
                
                // Notify renderer and trigger repaint
                g_State.pRenderer->OnScroll(yPos);
                InvalidateRect(hWnd, NULL, FALSE);
            }
            return 0;
        }

        case WM_MOUSEWHEEL: {
            // Handle mouse wheel scrolling (smooth scrolling).
            if (!g_State.pRenderer) return 0;
            
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            
            SCROLLINFO si = { sizeof(SCROLLINFO) };
            si.fMask = SIF_ALL;
            GetScrollInfo(hWnd, SB_VERT, &si);
            
            // Calculate scroll amount using constants
            int yPos = si.nPos - (delta / WHEEL_DELTA) * MOUSE_WHEEL_LINES * SCROLL_LINE_AMOUNT;
            
            // Clamp to valid range with safe bounds check
            int maxPos = si.nMax - (int)si.nPage + 1;
            if (maxPos < si.nMin) maxPos = si.nMin; // Prevent negative when content < view
            yPos = max(si.nMin, min(yPos, maxPos));
            
            if (yPos != si.nPos) {
                si.fMask = SIF_POS;
                si.nPos = yPos;
                SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
                
                // Notify renderer and trigger repaint
                g_State.pRenderer->OnScroll(yPos);
                InvalidateRect(hWnd, NULL, FALSE);
            }
            return 0;
        }
        
        // --- Custom Application Messages (from worker threads) ---
        case UIM_NAVIGATE_REQUEST: {
            UI_SetLoading(TRUE);
            if (g_State.pCallbacks && g_State.pCallbacks->pfnOnNavigate) {
                g_State.pCallbacks->pfnOnNavigate((const TCHAR*)lParam);
            }
            delete[] (TCHAR*)lParam; // Clean up the duplicated string.
            return 0;
        }

        case UIM_PARSING_COMPLETE: {
            UI_OnPageLoaded((ParsedPageData*)lParam); // lParam is a heap pointer, UI takes ownership.
            UI_SetLoading(FALSE);
            return 0;
        }

        case UIM_LINK_CLICKED: {
            if (g_State.pCallbacks && g_State.pCallbacks->pfnOnLinkClick) {
                g_State.pCallbacks->pfnOnLinkClick((const TCHAR*)lParam);
            }
            delete[] (TCHAR*)lParam; // Clean up the duplicated string.
            return 0;
        }

        case UIM_STATUS_UPDATE: {
            UI_SetStatusText((const TCHAR*)lParam);
            delete[] (TCHAR*)lParam; // Clean up the duplicated string.
            return 0;
        }

        case UIM_RENDER_REQUEST: {
            UI_InvalidateRender();
            return 0;
        }

        case UIM_IMAGE_LOADED: {
            // Image loaded from worker thread: wParam = HBITMAP, lParam = char* URL
            HBITMAP hBitmap = (HBITMAP)wParam;
            char* pszImageUrl = (char*)lParam;
            
            if (hBitmap && pszImageUrl && g_State.pRenderer) {
                // Convert char* URL to std::string for renderer
                std::string imageUrl(pszImageUrl);
                
                // Cache the loaded image in renderer
                g_State.pRenderer->NotifyImageLoaded(imageUrl, hBitmap);
                
                // Trigger repaint to display the newly loaded image
                UI_InvalidateRender();
            }
            
            // Free the heap-allocated URL string from worker thread
            if (pszImageUrl) {
                free(pszImageUrl);
            }
            
            return 0;
        }

        case WM_CLOSE: {
            // Notify core to begin cleanup before destroying the window.
            if (g_State.pCallbacks && g_State.pCallbacks->pfnOnClose) {
                g_State.pCallbacks->pfnOnClose();
            }
            DestroyWindow(hWnd);
            return 0;
        }
        
        case WM_DESTROY: {
            // Restore original window procedure
            if (g_State.hAddressBar && g_pfnOldAddressProc) {
                SetWindowLong(g_State.hAddressBar, GWL_WNDPROC, (LONG)g_pfnOldAddressProc);
            }
            
            // Post the quit message to terminate the message loop.
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


// ============================================================================
// SECTION 3: HELPER FUNCTION IMPLEMENTATIONS
// ============================================================================

static void CreateControls(HWND hParent) {
    // Create Address Bar (Edit Control).
    g_State.hAddressBar = CreateWindowEx(
        WS_EX_CLIENTEDGE, _T("EDIT"), _T(""),
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        0, 0, 0, 0, hParent, (HMENU)IDC_ADDRESS_BAR, g_State.hInstance, NULL);

    // Create "Go" Button.
    g_State.hGoButton = CreateWindowEx(
        0, _T("BUTTON"), _T("Go"),
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        0, 0, 0, 0, hParent, (HMENU)IDC_GO_BUTTON, g_State.hInstance, NULL);

    // Create Status Bar.
    g_State.hStatusBar = CreateStatusWindow(
        WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
        _T(""), hParent, IDC_STATUS_BAR);
    
    // Apply the standard Win98 font to controls.
    SendMessage(g_State.hAddressBar, WM_SETFONT, (WPARAM)g_State.hDefaultFont, TRUE);
    SendMessage(g_State.hGoButton, WM_SETFONT, (WPARAM)g_State.hDefaultFont, TRUE);
    SendMessage(g_State.hStatusBar, WM_SETFONT, (WPARAM)g_State.hDefaultFont, TRUE);
}

static void RepositionControls(HWND hParent) {
    // Calculate positions for a responsive layout.
    RECT rcClient;
    GetClientRect(hParent, &rcClient);
    
    int width = rcClient.right - rcClient.left;
    int height = rcClient.bottom - rcClient.top;

    // Reposition Address Bar and Go Button.
    int addressBarWidth = width - GO_BUTTON_WIDTH - UI_PADDING;
    MoveWindow(g_State.hAddressBar, UI_PADDING, UI_PADDING, addressBarWidth, ADDRESS_BAR_HEIGHT, TRUE);
    MoveWindow(g_State.hGoButton, addressBarWidth + UI_PADDING, UI_PADDING, GO_BUTTON_WIDTH, ADDRESS_BAR_HEIGHT, TRUE);

    // Let the status bar reposition itself.
    SendMessage(g_State.hStatusBar, WM_SIZE, 0, 0);

    // The renderer uses the remaining area.
    // InvalidateRect is called in WM_SIZE handler to trigger repaint.
}

static void HandleNavigate(HWND hWnd) {
    // Get URL and post a message to start the navigation flow.
    TCHAR urlBuffer[MAX_URL_LENGTH];
    UI_GetAddressBarText(urlBuffer, MAX_URL_LENGTH);
    if (urlBuffer[0] != _T('\0')) {
        PostMessage(hWnd, UIM_NAVIGATE_REQUEST, 0, (LPARAM)_tcsdup(urlBuffer));
    }
}

static RECT GetRenderAreaRect(void) {
    // Calculate the actual render area by subtracting UI chrome
    RECT rcClient;
    GetClientRect(g_State.hMainWnd, &rcClient);
    
    // Adjust for address bar at top
    rcClient.top = ADDRESS_BAR_HEIGHT + UI_PADDING * 2;
    
    // Adjust for status bar at bottom
    rcClient.bottom -= STATUS_BAR_HEIGHT;
    
    return rcClient;
}

static void ExtractAndSetPageTitle(const ParsedPageData* pData) {
    // Extract page title from parsed blocks and set window title
    // Strategy: Use first H1 heading if no <title> tag was found by parser
    // This is a UI-level policy decision, not parser responsibility
    
    if (!pData || pData->status != Parser::PARSE_SUCCESS) {
        UI_SetWindowTitle(NULL);
        return;
    }
    
    // Look for first H1 heading as fallback title
    for (size_t i = 0; i < pData->blocks.size(); i++) {
        if (pData->blocks[i].type == Parser::BLOCK_H1 && 
            !pData->blocks[i].content.empty()) {
            
            // Convert std::string to TCHAR* using std::vector for RAII
            #ifdef UNICODE
                int len = MultiByteToWideChar(CP_UTF8, 0, 
                                              pData->blocks[i].content.c_str(), 
                                              -1, NULL, 0);
                if (len > 0) {
                    std::vector<TCHAR> titleBuffer(len);
                    MultiByteToWideChar(CP_UTF8, 0, 
                                       pData->blocks[i].content.c_str(), 
                                       -1, &titleBuffer[0], len);
                    UI_SetWindowTitle(&titleBuffer[0]);
                    return; // Title set successfully
                }
            #else
                UI_SetWindowTitle(pData->blocks[i].content.c_str());
                return; // Title set successfully
            #endif
        }
    }
    
    // No suitable title found, use default
    UI_SetWindowTitle(NULL);
}

static LRESULT CALLBACK AddressSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    // Handle Enter key press in the address bar to trigger navigation.
    if (uMsg == WM_KEYDOWN && wParam == VK_RETURN) {
        HandleNavigate(GetParent(hWnd)); // Navigate using main window's context.
        return 0; // Don't process the key further.
    }
    
    // Select all text on focus for easy replacement.
    if (uMsg == WM_SETFOCUS) {
        PostMessage(hWnd, EM_SETSEL, 0, -1);
    }
    
    // Call original window procedure
    return CallWindowProc(g_pfnOldAddressProc, hWnd, uMsg, wParam, lParam);
}