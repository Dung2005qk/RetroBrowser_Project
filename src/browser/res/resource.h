#ifndef RESOURCE_H_INCLUDED
#define RESOURCE_H_INCLUDED

// ============================================================================
// resource.h - Central Resource ID Registry for Win98 Retro Browser
// ============================================================================
// PROJECT:  Win98 Retro Browser (OS Course Assignment)
// PURPOSE:  Immutable contract defining all resource identifiers (IDs) for
//           Windows resources (icons, menus, dialogs, controls, strings).
//           Serves as the bridge between C++ source code (logic/events) and
//           app.rc resource script (UI definitions).
//
// CRITICAL ROLE IN BUILD SYSTEM:
//   This file is included in stdafx.h (precompiled header). ANY modification
//   here triggers a FULL PROJECT REBUILD (~2-5 minutes on Win98 VM). Edit
//   with extreme caution and only when adding new UI elements.
//
// GOLDEN RULES:
//   1. ALL RESOURCE IDs MUST BE UNIQUE PROJECT-WIDE (no duplicates ever)
//   2. Use structured numbering ranges (see scheme below) to prevent conflicts
//   3. Follow Microsoft naming conventions strictly (IDI_, IDC_, IDR_, etc.)
//   4. SYNCHRONIZE with app.rc: Every ID here must have matching definition
//   5. NO C++ CODE HERE: Only #define macros (preprocessor definitions)
//   6. Changes are PERMANENT: Existing IDs cannot be renumbered (breaks .rc)
//
// TARGET PLATFORM:
//   Windows 98 SE with Visual C++ 6.0, IE5 common controls (comctl32.dll v5.0).
//   Constraints: 16-bit resource IDs, 16-color icons, no Unicode strings.
//
// MAINTENANCE:
//   - Add new IDs at END of appropriate range section
//   - Update _APS_NEXT_* macros when adding resources
//   - Document rationale for optional/enhancement IDs
//   - If using VC++ 6.0 Resource Editor, let it manage _APS_* macros
//
// AUTHOR NOTES:
//   This is a TEACHING PROJECT. Comments emphasize OS concepts: resource
//   loading (LoadIcon/LoadMenu), event handling (WM_COMMAND routing),
//   string localization (LoadString), and Win32 memory management.
// ============================================================================


// ============================================================================
// RESOURCE ID NUMBERING SCHEME
// ============================================================================
// Structured allocation prevents ID collisions and groups related resources
// for maintainability. Ranges leave room for expansion without renumbering.
//
// RANGE        PURPOSE                      EXAMPLES
// ------       -------------------------    ---------------------------------
// 1-99         Visual Identity              Icons, Cursors, Bitmaps
//              (Application Branding)       IDI_APP_ICON, IDC_HAND_CURSOR
//
// 100-199      User Interactions            Menus, Accelerators, Commands
//              (Navigation/Commands)        IDR_MAIN_MENU, IDM_FILE_EXIT
//
// 200-299      Modal UI Elements            Dialogs and Dialog Controls
//              (Dialogs Only)               IDD_ABOUT_DIALOG, IDC_ABOUT_ICON
//
// 300-399      Localization Strings         String Table Entries
//              (Text Resources)             IDS_APP_TITLE, IDS_ERROR_404
//
// 400-449      Main Window Controls         Non-modal Window Children
//              (Primary UI Controls)        IDC_ADDRESS_EDIT, IDC_GO_BUTTON
//
// 450-499      Advanced Features            Custom Messages, Timers, etc.
//              (Async/Extensions)           WM_APP_NETWORK_COMPLETE, IDT_LOADING_TIMER
//
// RATIONALE: Start at 1 (not 0) to avoid conflicts with system constants
//            like IDOK (1), IDCANCEL (2). System reserves 0-10, so 1-99
//            is safe for app-specific visuals. Higher ranges (100+) are
//            standard practice for Win32 apps to ensure uniqueness.
// ============================================================================


// ============================================================================
// SECTION 1: APPLICATION IDENTITY (ICONS, CURSORS, BITMAPS)
// ============================================================================
// Visual elements defining the application's look and feel. Win98 constraints:
// - Icons: 32x32 and 16x16 sizes, 16-color palette (4-bit) for compatibility
// - Cursors: Monochrome or 16-color, standard shapes + custom for hyperlinks
// - Bitmaps: BMP format only; JPEG via external libs (optional phase 4)
//
// USAGE PATTERN:
//   HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON));
//   SetClassLong(hwnd, GCL_HICON, (LONG)hIcon);
//
// OS CONCEPT: Resource loading abstracts file I/O. Icons embedded in .exe
//             PE section (".rsrc"), loaded on-demand by Windows kernel.
// ============================================================================

#define IDI_APP_ICON                    1
// Main application icon (32x32 + 16x16 multi-res). Displayed in:
// - Window title bar (16x16, left of "Win98 Retro Browser")
// - Taskbar button (16x16, shows when app is running)
// - Alt+Tab switcher (32x32, for task switching)
// - Explorer (when .exe is viewed as file)
// - About dialog (as static control IDC_ABOUT_ICON)
// MANDATORY: Every Win32 app should define this for professional appearance.
// DESIGN TIP: Use nostalgic 90s aesthetic (globe, gears, retro computer).

#define IDC_HAND_CURSOR                 2
// Custom hand cursor (pointing finger) for hovering over hyperlinks.
// USAGE: In WM_SETCURSOR handler when mouse over <a> tag text:
//        SetCursor(LoadCursor(hInstance, MAKEINTRESOURCE(IDC_HAND_CURSOR)));
// PHASE: Optional enhancement (Phase 4). Improves UX for clickable links.
// FALLBACK: If not defined in .rc, use system IDC_HAND (Win98 has it).

#define IDB_LOGO_SPLASH                 3
// Optional: Bitmap for splash screen or toolbar. Reserved for future use.
// RATIONALE: Included for forward-thinking design; demonstrates resource
//            planning even if not implemented in 8-week scope.


// ============================================================================
// SECTION 2: MENU RESOURCES AND COMMAND IDs
// ============================================================================
// Menus provide structured navigation and expose keyboard shortcuts. Even
// minimal apps benefit from File/Help menus for professionalism.
//
// ARCHITECTURE:
//   IDR_* = Resource ID for entire menu/accelerator table (loaded as unit)
//   IDM_* = Command ID for individual menu items (sent via WM_COMMAND)
//
// USAGE PATTERN:
//   HMENU hMenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MAIN_MENU));
//   SetMenu(hwnd, hMenu);
//   // In WndProc:
//   case WM_COMMAND:
//       if (LOWORD(wParam) == IDM_FILE_EXIT) { PostMessage(hwnd, WM_CLOSE, 0, 0); }
//
// OS CONCEPT: WM_COMMAND message routing. OS tracks menu states (enabled/
//             checked), developer handles logic. Separation of concerns.
// ============================================================================

#define IDR_MAIN_MENU                   100
// Resource ID for the complete menu bar attached to main window.
// STRUCTURE (defined in app.rc):
//   File    -> Exit (IDM_FILE_EXIT)
//   View    -> Refresh (IDM_VIEW_REFRESH), View Source (IDM_VIEW_SOURCE)
//   Navigate-> Back (IDM_NAV_BACK), Forward (IDM_NAV_FORWARD) [optional]
//   Help    -> About (IDM_HELP_ABOUT)
// RATIONALE: Standard Windows UX. Provides redundancy if buttons missing.

//--- File Menu Commands (101-109) ---
#define IDM_FILE_EXIT                   101
// Command: File > Exit. Cleanly shuts down application.
// HANDLER: Posts WM_CLOSE to trigger graceful cleanup (close sockets, free
//          memory, destroy window). Never call ExitProcess directly (leaks).
// ACCELERATOR: Alt+F4 (system default, no need to define in .rc).

//--- View Menu Commands (110-119) ---
#define IDM_VIEW_REFRESH                110
// Command: View > Refresh (or F5). Reloads current URL from network.
// HANDLER: Re-fetch HTML via network module, re-parse, re-render.
// PHASE: Core functionality (Phase 4). Essential for testing parser changes.
// TIP: Cache last URL in global variable; GetWindowText on address bar fallback.

#define IDM_VIEW_SOURCE                 111
// Command: View > View Source. Displays raw HTML in MessageBox or Notepad.
// HANDLER: ShellExecute("notepad.exe", temp_file_with_html) or multiline edit.
// PHASE: Optional (Phase 4 enhancement). Useful for debugging parser issues.
// RATIONALE: Educational value—shows students how HTML looks "under the hood."

//--- Navigate Menu Commands (120-129) [Reserved for Optional] ---
#define IDM_NAV_BACK                    120
// Command: Navigate > Back. Go to previous page in history stack.
// HANDLER: Pop from std::vector<std::string> historyStack; reload.
// PHASE: Optional (Phase 4). Requires implementing history management.
// CHALLENGE: Distinguish user-initiated back vs. new URL (push vs. pop logic).

#define IDM_NAV_FORWARD                 121
// Command: Navigate > Forward. Go to next page (if user went back earlier).
// HANDLER: Redo stack logic (std::vector forwardStack). Complex for assignment.
// PHASE: Bonus feature. Recommended only if core features stable by Week 7.

//--- Help Menu Commands (130-139) ---
#define IDM_HELP_ABOUT                  130
// Command: Help > About. Opens modal dialog with app info (version, credits).
// HANDLER: DialogBox(hInstance, MAKEINTRESOURCE(IDD_ABOUT_DIALOG), hwnd, AboutDlgProc);
// PHASE: Core (Phase 1-2). Demonstrates dialog creation, modal message loop.
// CONTENT: App icon, "Win98 Retro Browser v1.0", "OS Course Project", OK button.

//--- Accelerator Tables (140-149) ---
#define IDR_ACCELERATORS                140
// Accelerator table resource ID.
// BINDINGS (defined in .rc ACCELERATORS section):
//   F5          -> IDM_VIEW_REFRESH
//   Ctrl+Q      -> IDM_FILE_EXIT (alternative to Alt+F4)
//   Ctrl+L      -> IDM_FOCUS_ADDRESS (Set focus to address bar)
// USAGE:
//   HACCEL hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATORS));
//   // In message loop: if (!TranslateAccelerator(hwnd, hAccel, &msg)) { ... }
// OS CONCEPT: TranslateAccelerator intercepts keystrokes, converts to WM_COMMAND
//             messages with accelerator IDs. Seamless integration with menu logic.
// PHASE: Optional (Phase 5). Improves power-user efficiency.
// RATIONALE: Placed in same range as IDR_MAIN_MENU (100) since both are IDR_
//            resource types providing user interaction mechanisms.

#define IDM_FOCUS_ADDRESS               141
// Command: Ctrl+L accelerator to set focus to address bar (IDC_ADDRESS_EDIT).
// HANDLER: In WM_COMMAND handler:
//          if (LOWORD(wParam) == IDM_FOCUS_ADDRESS) {
//              SetFocus(GetDlgItem(hwnd, IDC_ADDRESS_EDIT));
//              // Optional: Select all text for quick overwrite
//              SendMessage(hAddressEdit, EM_SETSEL, 0, -1);
//          }
// PHASE: Optional (Phase 5). UX enhancement for keyboard-centric users.
// RATIONALE: Standard browser behavior (Chrome, Firefox use Ctrl+L/Alt+D).
//            Improves navigation efficiency without mouse interaction.
// ACCELERATOR BINDING: Defined in .rc ACCELERATORS section as:
//                      "L", IDM_FOCUS_ADDRESS, VIRTKEY, CONTROL
// NOTE: This is a command ID for accelerator, not a menu item. No menu entry
//       needed in IDR_MAIN_MENU structure.


// ============================================================================
// SECTION 3: DIALOG RESOURCES AND DIALOG CONTROL IDs
// ============================================================================
// Dialogs are modal or modeless child windows with predefined layouts (.rc).
// Controls (buttons, edits, statics) are identified by IDC_* constants.
//
// SPECIAL CONSTANT:
//   IDC_STATIC (-1): System-managed ID for non-interactive controls (labels,
//   group boxes, decorative icons). Saves ID space; Windows doesn't route
//   messages for these. Use for "About" text, status labels, etc.
//
// DIALOG LIFECYCLE:
//   1. Define template in .rc (position, size, styles)
//   2. Call DialogBox/CreateDialog with IDD_* resource ID
//   3. OS creates window, sends WM_INITDIALOG to DialogProc
//   4. DialogProc handles WM_COMMAND from controls (OK/Cancel buttons)
//   5. EndDialog closes modal dialog, returns control to parent
//
// OS CONCEPT: Message-driven UI. Dialog manager abstracts tab order, default
//             buttons (DEFPUSHBUTTON), Escape/Enter handling (IDOK/IDCANCEL).
// ============================================================================

#define IDC_STATIC                      -1
// Generic ID for static controls (labels, frames, non-interactive elements).
// USAGE: Any control that doesn't need unique identification. Examples:
//        - "URL:" label next to address bar
//        - About dialog header text "Win98 Retro Browser"
//        - Decorative icon in dialog (though IDC_ABOUT_ICON is better)
// RATIONALE: System convention; conserves ID space. Multiple statics can share
//            this ID because they never send WM_COMMAND messages.

//--- About Dialog (200-209) ---
#define IDD_ABOUT_DIALOG                200
// Template for Help > About modal dialog (200x150 pixels, DS_MODALFRAME style).
// STRUCTURE (defined in app.rc):
//   - Icon (IDC_ABOUT_ICON): 32x32, left-aligned
//   - Static text (IDC_ABOUT_TEXT): Multi-line, center-aligned
//   - OK button (IDOK): Default push button, closes dialog
// HANDLER: AboutDlgProc (trivial: WM_INITDIALOG loads strings, WM_COMMAND IDOK
//          calls EndDialog). No validation needed.

#define IDC_ABOUT_ICON                  210
// Static control (SS_ICON style) displaying IDI_APP_ICON in About dialog.
// USAGE: SetDlgItemIcon or defined directly in .rc as ICON IDI_APP_ICON.
// POSITION: Top-left, (10, 10), size (32, 32).

#define IDC_ABOUT_TEXT                  211
// Static control (SS_CENTER | SS_NOPREFIX) for multi-line About text.
// CONTENT (loaded via SetDlgItemText from IDS_ABOUT_TEXT or hardcoded):
//   "Win98 Retro Browser v1.0\n"
//   "A teaching project demonstrating Win32 API,\n"
//   "Winsock networking, and GDI rendering.\n\n"
//   "Built for Operating Systems course."
// POSITION: (50, 10), size (140, 80). Right of icon.

// Note: About dialog OK button uses system IDOK (1), not custom IDC_ABOUT_OK.
// RATIONALE: EndDialog(hDlg, IDOK) is standard pattern; no need to redefine.


// ============================================================================
// SECTION 4: MAIN WINDOW CHILD CONTROLS (Non-Modal)
// ============================================================================
// These are not dialog controls but direct children of the main window,
// created programmatically via CreateWindow/Ex in WM_CREATE handler.
// ARCHITECTURAL NOTE: Separated from dialog controls (200-299) for clarity.
//
// LAYOUT DESIGN (Top to Bottom):
//   +----------------------------------------------------------------+
//   | [Address Bar: __________________________] [Go] <-- Top 5px     |
//   |                                                                |
//   | +------------------------------------------------------------+ |
//   | | Content Area (rendered HTML, scrollable)                   | |
//   | | (WM_PAINT draws here; custom control or static background) | |
//   | +------------------------------------------------------------+ |
//   |                                                                |
//   | [Status: Ready_________________________________] <-- Bottom 2px |
//   +----------------------------------------------------------------+
//
// SIZING: Handle WM_SIZE to reposition controls dynamically. Use GetClientRect
//         to compute widths. Example: Address bar = clientWidth - 60 (for button).
//
// OS CONCEPT: Child windows send WM_COMMAND (buttons) or WM_NOTIFY (edits with
//             EN_CHANGE) to parent. Parent's WndProc routes based on LOWORD(wParam).
// ============================================================================

#define IDC_ADDRESS_EDIT                400
// Edit control for URL input (single-line, auto horizontal scroll).
// STYLES: ES_LEFT | ES_AUTOHSCROLL | WS_BORDER | WS_TABSTOP
// USAGE:
//   CreateWindow("EDIT", "", ES_LEFT | ES_AUTOHSCROLL | WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP,
//                10, 5, 500, 25, hwndMain, (HMENU)IDC_ADDRESS_EDIT, hInstance, NULL);
//   // Retrieve URL: char url[MAX_URL_LENGTH]; GetWindowText(hEdit, url, sizeof(url));
// EVENTS: EN_CHANGE notification (optional, for "type-ahead" features not in scope).
//         Enter key: Subclass to detect VK_RETURN, trigger IDC_GO_BUTTON click.
// PHASE: Core (Phase 5). First interactive element users see.

#define IDC_GO_BUTTON                   401
// Push button to initiate navigation (BS_PUSHBUTTON style).
// USAGE:
//   CreateWindow("BUTTON", "Go", BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE | WS_TABSTOP,
//                520, 5, 50, 25, hwndMain, (HMENU)IDC_GO_BUTTON, hInstance, NULL);
//   // In WndProc WM_COMMAND: if (LOWORD(wParam) == IDC_GO_BUTTON) { /* fetch URL */ }
// HANDLER: Core navigation logic (Phase 2-4):
//   1. GetWindowText(hAddressEdit) -> URL string
//   2. Call network::FetchUrlViaProxy(url) -> raw HTML
//   3. Call parser::ParseHtml(html) -> std::vector<HtmlBlock>
//   4. Store blocks in global/class member
//   5. InvalidateRect(hwndContent, NULL, TRUE) -> triggers WM_PAINT
// PHASE: Core (Phase 5). Primary user action.

#define IDC_STATUS_STATIC               402
// Static control for status messages (bottom of window, aligned left).
// STYLES: SS_LEFT | WS_BORDER | SS_SUNKEN (optional for Win98 look)
// USAGE:
//   CreateWindow("STATIC", "Ready", SS_LEFT | WS_CHILD | WS_VISIBLE | WS_BORDER,
//                0, clientHeight - 20, clientWidth, 20, hwndMain, (HMENU)IDC_STATUS_STATIC, hInstance, NULL);
//   // Update: SetWindowText(hStatusStatic, "Loading http://example.com...");
// CONTENT: Rotate between IDS_LOADING ("Loading..."), IDS_DONE ("Done"),
//          error strings (IDS_ERROR_CONNECTION, IDS_ERROR_404).
// PHASE: Core (Phase 4). Provides feedback during async operations (simulated
//        by blocking recv in this project; real async would use threads).

// NOTE: Content area (where HTML is rendered) does NOT need a control ID.
//       It's the main window's client area itself, drawn in WM_PAINT using
//       renderer module. Alternatively, create a child static window with
//       IDC_CONTENT_AREA if implementing scroll bars (Phase 4 optional).

#define IDC_CONTENT_AREA                403
// Optional: Custom control or child window for rendering area.
// USAGE: If implementing scrolling or isolating rendering logic from main WndProc.
//   CreateWindow("STATIC", "", SS_OWNERDRAW | WS_CHILD | WS_VISIBLE | WS_VSCROLL,
//                0, 30, clientWidth, clientHeight - 50, hwndMain, (HMENU)IDC_CONTENT_AREA, hInstance, NULL);
//   // Handle WM_PAINT for this child window separately, easier scroll management.
// PHASE: Optional (Phase 4). Simplifies code if going beyond minimal scope.
// ALTERNATIVE: Render directly to main window; adjust PAINTSTRUCT rcPaint bounds.

//--- Optional Navigation Buttons (404-409) ---
#define IDC_NAV_BACK_BUTTON             404
// Optional: Back navigation button control (toolbar or child button).
// USAGE: If implementing UI buttons (not just menu items) for navigation:
//   CreateWindow("BUTTON", "<", BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE,
//                x, y, 30, 25, hwndMain, (HMENU)IDC_NAV_BACK_BUTTON, hInstance, NULL);
//   // In WM_COMMAND: Route to IDM_NAV_BACK command handler for consistency.
// PHASE: Optional (Phase 5). Complements IDM_NAV_BACK menu command.
// RATIONALE: Provides explicit control ID for button while keeping command logic
//            centralized through IDM_NAV_BACK. Button sends WM_COMMAND with this
//            ID, which can then invoke the same handler as the menu item.

#define IDC_NAV_FORWARD_BUTTON          405
// Optional: Forward navigation button control.
// USAGE: Parallel to IDC_NAV_BACK_BUTTON; routes to IDM_NAV_FORWARD handler.
//   CreateWindow("BUTTON", ">", BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE,
//                x, y, 30, 25, hwndMain, (HMENU)IDC_NAV_FORWARD_BUTTON, hInstance, NULL);
// PHASE: Optional (Phase 5). Visual companion to Forward menu command.
// ARCHITECTURAL NOTE: Separating control IDs (IDC_) from command IDs (IDM_)
//                     provides flexibility: same visual button can trigger
//                     different commands based on state/context if needed.


// ============================================================================
// SECTION 5: STRING RESOURCES (STRING TABLE)
// ============================================================================
// Centralized text for UI messages, enabling easy localization (hypothetically;
// Win98 CP1252 codepage limits to Western European languages for this project).
//
// ADVANTAGES:
//   - Change wording without recompiling (edit .rc only; but still rebuild).
//   - Consistent messaging (e.g., error format "Error: %s").
//   - Preparation for multi-language support (RC conditional compilation).
//
// USAGE PATTERN:
//   char buffer[256];
//   LoadString(hInstance, IDS_APP_TITLE, buffer, sizeof(buffer));
//   SetWindowText(hwnd, buffer);
//
// OS CONCEPT: Resource-based strings reduce hardcoded literals, centralizing
//             text management. Windows caches string table in memory for fast
//             repeated LoadString calls (no disk I/O per call).
// ============================================================================

//--- Application Identity Strings (300-309) ---
#define IDS_APP_TITLE                   300
// "Win98 Retro Browser"
// USAGE: Main window title (CreateWindow lpWindowName), About dialog header.
// CONTEXT: First string users see; branding consistency crucial.

//--- Error Messages (310-329) ---
#define IDS_ERROR_CONNECTION            310
// "Failed to connect to proxy at %s:%d. Ensure proxy.py is running on the host machine."
// USAGE: MessageBox on Winsock connect() failure (network module).
// PARAMS: sprintf with PROXY_DEFAULT_HOST, PROXY_DEFAULT_PORT.
// TROUBLESHOOTING TIP: Embed in string to reduce support questions.

#define IDS_ERROR_PARSE                 311
// "Unable to parse HTML. The page may contain unsupported tags or malformed syntax."
// USAGE: MessageBox after parser::ParseHtml returns empty vector or throws exception.
// RATIONALE: Graceful degradation; helps students debug parser logic.

#define IDS_ERROR_404                   312
// "Page not found (HTTP 404). The requested URL does not exist on the server."
// USAGE: Check HTTP response status line in network module; show if "404" detected.
// CHALLENGE: Requires basic HTTP header parsing (search for "404" in first line).

#define IDS_ERROR_TIMEOUT               313
// "Connection timed out. The server did not respond within 30 seconds."
// USAGE: If recv() blocks beyond HTTP_TIMEOUT_MS (set via setsockopt SO_RCVTIMEO).
// PHASE: Optional (Phase 2). Improves robustness; prevents hang on slow sites.

#define IDS_ERROR_UNKNOWN               314
// "An unknown error occurred. Please check the proxy logs for details."
// USAGE: Catch-all for unexpected exceptions or unhandled Winsock errors.
// LOG TIP: Direct users to proxy console output (valuable for debugging).

//--- Status Messages (320-329) ---
#define IDS_LOADING                     320
// "Loading %s..."
// USAGE: SetWindowText on IDC_STATUS_STATIC when fetch starts.
// PARAMS: sprintf with URL (first 50 chars to fit status bar width).

#define IDS_DONE                        321
// "Done"
// USAGE: Update status bar after successful render.

#define IDS_READY                       322
// "Ready"
// USAGE: Default status bar text on app startup or after user cancels action.

//--- About Dialog Text (330-339) ---
#define IDS_ABOUT_TEXT                  330
// "Win98 Retro Browser v1.0\r\n\r\n"
// "A teaching project demonstrating Win32 API, Winsock networking, and GDI rendering.\r\n\r\n"
// "Developed for Operating Systems course.\r\n"
// "Tested on Windows 98 SE with Visual C++ 6.0."
// USAGE: SetDlgItemText(hDlg, IDC_ABOUT_TEXT, buffer) in WM_INITDIALOG.
// NOTE: \r\n for Windows line breaks (not \n). LoadString handles this.

//--- Tooltip and Accessibility Strings (Optional, 340-349) ---
#define IDS_GO_TOOLTIP                  340
// "Navigate to the entered URL"
// USAGE: If implementing tooltips via TTM_ADDTOOL (comctl32 tooltip control).
// PHASE: Bonus feature (Week 8 if time permits). Low priority but polishes UX.

#define IDS_REFRESH_TOOLTIP             341
// "Reload the current page (F5)"
// USAGE: Toolbar button tooltip (if implementing toolbar; not in minimal scope).


// ============================================================================
// SECTION 6: ADVANCED FEATURES (CUSTOM MESSAGES, TIMERS, ASYNC EVENTS)
// ============================================================================
// Custom window messages and advanced UI features for asynchronous operations,
// background tasks, and inter-thread communication.
//
// CUSTOM MESSAGE ARCHITECTURE:
//   WM_APP is the base for user-defined messages (0x8000). Adding offsets
//   (WM_APP + 1, WM_APP + 2, etc.) creates unique message IDs for custom events.
//
// USAGE PATTERN (Async Network Communication):
//   // In network thread (hypothetical):
//   PostMessage(hwndMain, WM_APP_NETWORK_COMPLETE, (WPARAM)statusCode, (LPARAM)dataPtr);
//   
//   // In main WndProc:
//   case WM_APP_NETWORK_COMPLETE:
//       int status = (int)wParam;
//       char* html = (char*)lParam;
//       // Process HTML, update UI, free memory
//       break;
//
// OS CONCEPT: PostMessage is thread-safe; allows background threads to notify
//             UI thread without blocking. Essential for responsive applications.
// ============================================================================

//--- Custom Window Messages (450-469) ---
#define WM_APP_NETWORK_COMPLETE         (WM_APP + 1)
// Custom message: Network fetch operation completed (success or failure).
// WPARAM: HTTP status code (200 = success, 404 = not found, 0 = connection error)
// LPARAM: Pointer to dynamically allocated HTML string (char*) or NULL on error
// USAGE: Posted by network thread/module to notify main window of completion:
//   PostMessage(hwndMain, WM_APP_NETWORK_COMPLETE, (WPARAM)200, (LPARAM)htmlData);
// HANDLER RESPONSIBILITY: Must free LPARAM memory if non-NULL (HeapFree/delete).
// PHASE: Core (Phase 2-3). Enables async network operations without blocking UI.
// RATIONALE: Separates network I/O (blocking recv) from UI responsiveness.
//            Even in single-threaded implementation, demonstrates async pattern.

#define WM_APP_PARSE_COMPLETE           (WM_APP + 2)
// Custom message: HTML parsing completed.
// WPARAM: Number of parsed elements (size_t cast to WPARAM)
// LPARAM: Pointer to std::vector<HtmlBlock>* (must be freed by handler)
// USAGE: If parser runs in separate thread (optional Phase 4 enhancement):
//   PostMessage(hwndMain, WM_APP_PARSE_COMPLETE, (WPARAM)blockCount, (LPARAM)blocks);
// HANDLER: Stores blocks, triggers InvalidateRect for re-render, deletes vector.
// PHASE: Optional (Phase 4). For true multi-threaded parsing (advanced).
// NOTE: Single-threaded version can skip this; parse synchronously in main thread.

#define WM_APP_STATUS_UPDATE            (WM_APP + 3)
// Custom message: Update status bar with progress/state information.
// WPARAM: Message type (0=info, 1=warning, 2=error)
// LPARAM: Pointer to status string (char*) - handler must free
// USAGE: Internal messaging for modular status updates:
//   char* msg = _strdup("Loading http://example.com...");
//   PostMessage(hwndMain, WM_APP_STATUS_UPDATE, 0, (LPARAM)msg);
// HANDLER: SetWindowText(hStatusBar, (char*)lParam); free((void*)lParam);
// PHASE: Optional (Phase 5). Cleaner than direct SetWindowText calls.
// BENEFIT: Decouples modules from UI controls; better separation of concerns.

//--- Timer IDs (470-479) ---
#define IDT_LOADING_TIMER               470
// Timer ID for animated "Loading..." status (e.g., rotating ellipsis).
// USAGE: SetTimer(hwnd, IDT_LOADING_TIMER, 500, NULL); // 500ms interval
//        In WM_TIMER: Cycle status text "Loading", "Loading.", "Loading..", "Loading...".
//        KillTimer(hwnd, IDT_LOADING_TIMER); // Stop when WM_APP_NETWORK_COMPLETE received
// PHASE: Bonus polish (Week 8). Demonstrates timer usage (OS concept: time-based
//        events without blocking main thread).
// PATTERN: Start timer on Go button click, kill on network completion message.

#define IDT_BLINK_CURSOR                471
// Timer ID for simulated text cursor blinking in address bar (if custom-drawn).
// USAGE: Advanced feature for owner-drawn edit control with custom cursor.
// PHASE: Out of scope. Included for completeness in numbering scheme.

//--- Progress and Status Controls (480-489) ---
#define IDC_PROGRESS_BAR                480
// Progress bar control (PROGRESS_CLASS from comctl32) for download percentage.
// USAGE: If implementing chunked recv with Content-Length tracking (advanced).
//        SendMessage(hProgress, PBM_SETPOS, (WPARAM)percentComplete, 0);
// PHASE: Stretch goal. Requires parsing HTTP headers for Content-Length.


// ============================================================================
// SECTION 8: IDE AUTO-GENERATION MACROS (VISUAL C++ 6.0 RESOURCE EDITOR)
// ============================================================================
// The VC++ 6.0 Resource Editor uses these macros to auto-increment IDs when
// adding resources via the GUI (Insert > Resource). Wrapped in preprocessor
// check to only activate when RC compiler invokes with APSTUDIO_INVOKED.
//
// RATIONALE: Prevents manual ID conflicts. Developer should update these after
//            adding resources in this header to keep IDE in sync.
//
// MAINTENANCE: After adding the highest ID in each category, set:
//   _APS_NEXT_RESOURCE_VALUE  = highest IDD_/IDR_/IDI_/IDB_ + 1
//   _APS_NEXT_COMMAND_VALUE   = highest IDM_ + 1
//   _APS_NEXT_CONTROL_VALUE   = highest IDC_ (all controls including main window) + 1
//   _APS_NEXT_SYMED_VALUE     = highest symbol (usually 101+ if unused)
//
// READONLY_SYMBOLS: Block manual edits in IDE; ensures consistency.
// ============================================================================

#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NEXT_RESOURCE_VALUE        250
// Next available ID for dialogs (IDD_*), menus (IDR_*), icons (IDI_*), bitmaps (IDB_*).
// CURRENT HIGHEST: IDD_ABOUT_DIALOG (200).
// SET TO: 250 (adequate buffer for dialog and resource expansion).
// NOTE: IDR_ACCELERATORS (140) relocated to Section 2 with other IDR_ resources.

#define _APS_NEXT_COMMAND_VALUE         142
// Next available ID for menu commands (IDM_*) and accelerators.
// CURRENT HIGHEST: IDM_FOCUS_ADDRESS (141).
// SET TO: 142 (leaves room in 140-149 for additional accelerator commands).

#define _APS_NEXT_CONTROL_VALUE         490
// Next available ID for dialog and window controls (IDC_*).
// CURRENT HIGHEST: IDC_PROGRESS_BAR (480).
// SET TO: 490 (leaves room in 480-489 for additional progress/status controls).
// NOTE: Numbering scheme - Dialog controls: 200-299, Main window: 400-449, 
//       Advanced: 450-489. This value correctly reflects highest control ID.

#define _APS_NEXT_SYMED_VALUE           101
// Next available symbol ID (rarely used; legacy from older RC compilers).
// DEFAULT: 101 (safe starting point; VC++ 6.0 rarely touches this).
// NOTE: If project doesn't use symbols, leave as-is.

#endif // APSTUDIO_READONLY_SYMBOLS
#endif // APSTUDIO_INVOKED


// ============================================================================
// END OF RESOURCE DEFINITIONS
// ============================================================================
// FINAL WARNINGS:
//   1. DO NOT ADD C++ CODE HERE (functions, variables, classes). This is a
//      HEADER FOR PREPROCESSOR DEFINITIONS ONLY. Code belongs in .cpp files.
//
//   2. SYNCHRONIZE WITH app.rc: Every ID here must have a corresponding entry
//      in app.rc (ICON, MENU, DIALOG, STRINGTABLE). Orphaned IDs cause linker
//      errors or runtime LoadIcon/LoadMenu failures (returns NULL).
//
//   3. IMMUTABLE IDs: Once an ID is assigned and used in .rc, NEVER change its
//      value. Renumbering breaks binary compatibility with compiled .res files
//      and confuses version control diffs. Add new IDs at range ends.
//
//   4. VERSION CONTROL TIP: When merging branches, watch for ID conflicts.
//      Use structured ranges to minimize collision risk (e.g., developer A
//      uses 225-229 for feature X, developer B uses 230-234 for feature Y).
//
//   5. WIN98 TESTING: Always verify resources load on VM (check for NULL returns
//      from LoadIcon/LoadMenu). Modern dev machines may succeed but VM fails
//      due to missing common control versions or icon format incompatibility.
//
//   6. REBUILD IMPACT: Since this file is in stdafx.h, changes here = full
//      rebuild. Batch multiple resource additions to minimize downtime.
//
// QUESTIONS OR ISSUES: Consult MSDN archive (Win32 API reference, Resource
//                      Compiler documentation) or course instructor/TA.
// ============================================================================

#endif // RESOURCE_H_INCLUDED