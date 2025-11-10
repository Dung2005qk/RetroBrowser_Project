// ============================================================================
// renderer.h - Public Interface for the HTML Renderer Module
// ============================================================================
// MODULE PURPOSE:
// Provides a stateful, high-performance GDI-based rendering engine that transforms
// the flat list of HtmlBlock structures (from Parser module) into visible content
// painted on a Win32 window surface. This module sits at the final stage of the
// browser pipeline, converting abstract content representation (parsed HTML blocks)
// into actual pixels on screen using Windows 98's Graphics Device Interface (GDI).
//
// ARCHITECTURE OVERVIEW:
//
//   +-------------------+       +--------------------+       +------------------+
//   | Parser Module     | ----> | Renderer Module    | ----> | Screen Display   |
//   | [HtmlBlock list]  |       | [GDI API calls]    |       | [Visible pixels] |
//   | std::vector       |       | Layout + Paint     |       | User sees HTML   |
//   +-------------------+       +--------------------+       +------------------+
//         ^                              ^                            |
//         |                       This Module                         |
//         |                    (Stateful Layout,                      |
//         |                     Resource Manager,                     |
//         |                     Event Handler)                        |
//         |                                                            |
//   Input: Typed Blocks  --->  Process: Layout+Cache  ---> Output: Painted Window
//
//   RENDERING PIPELINE (Two-Phase Architecture):
//   
//   Phase 1: LAYOUT (CalculateLayout) - Triggered by content/size changes
//     Input:  vector<HtmlBlock> + RECT clientArea
//     Process: Word-wrapping, position calculation, clickable region mapping
//     Output: vector<RenderItem> (display list with computed bounds)
//     Cost:   O(n) where n = block count, expensive (~50-200ms for 200 blocks)
//   
//   Phase 2: PAINTING (Render) - Triggered by WM_PAINT, scroll, invalidation
//     Input:  vector<RenderItem> + HDC + scrollY
//     Process: GDI drawing (TextOut, BitBlt), clipping, double-buffering
//     Output: Pixels on screen
//     Cost:   O(v) where v = visible blocks, fast (~10-50ms, 60fps capable)
//
// DESIGN PHILOSOPHY (6 Core Principles):
//
// 1. SEPARATION OF LAYOUT AND PAINTING - PERFORMANCE CRITICAL:
//    The renderer strictly separates expensive layout computation (word-wrapping,
//    position calculation) from fast painting operations (GDI drawing). This is
//    the MOST IMPORTANT architectural decision for Win98 performance.
//
//    TWO-PHASE RENDERING:
//      Phase 1: LAYOUT (CalculateLayout method)
//        - Triggered: SetContent (new page), WM_SIZE (window resize)
//        - Operation: Iterate all blocks, compute positions/sizes via DrawText
//                     with DT_CALCRECT flag (measures without drawing)
//        - Output: m_displayList (pre-computed RenderItem structs with RECT bounds)
//        - Cost: Expensive O(n) - calls DrawText n times for sizing
//        - Frequency: Rare (only on content/window changes)
//
//      Phase 2: PAINTING (Render method)
//        - Triggered: WM_PAINT, scrolling, partial invalidation
//        - Operation: Draw only VISIBLE RenderItems from display list
//        - Input: Pre-computed bounds from Phase 1 (no recalculation)
//        - Cost: Fast O(v) where v << n (only visible blocks)
//        - Frequency: Very high (every scroll, every repaint)
//
//    RATIONALE:
//      - Win98 CPU limitation: 200MHz Pentium MMX cannot afford layout in paint loop
//      - Smooth scrolling: 60fps requires <16ms per frame; layout would take 100ms+
//      - GDI performance: TextOut is fast (~1ms), DrawText with sizing is slow (~5ms)
//      - Battery efficiency: Minimize CPU cycles on every WM_PAINT message
//
//    EXAMPLE IMPACT:
//      Without separation: Scroll 100px -> Render() -> Layout 200 blocks -> 150ms lag
//      With separation:    Scroll 100px -> Render() -> Draw 20 visible -> 15ms smooth
//
// 2. STATEFUL RESOURCE MASTERY - GDI HEAP MANAGEMENT:
//    Unlike the stateless Parser, Renderer is intentionally STATEFUL to manage:
//      - Content state: Current HtmlBlock vector, display list, scroll position
//      - GDI resources: Cached fonts (HFONT), bitmaps (HBITMAP), brushes, pens
//      - Layout cache: Pre-computed positions, clickable areas, total height
//      - Offscreen buffer: Memory DC and bitmap for double-buffering
//
//    GDI RESOURCE LIFETIME (RAII-Like Pattern):
//      Constructor:  Allocate common resources (fonts for H1/H2/P/A, default colors)
//      SetContent:   Cache persists (don't recreate fonts per page load)
//      Destructor:   Release ALL GDI handles (prevent leaks on window close)
//
//    WHY STATEFUL?
//      - Performance: Font creation is expensive (~10-20ms per CreateFont on Win98)
//                     Caching 5 fonts saves 100ms+ per page load
//      - Resource limits: Win98 GDI heap is tiny (~64KB, ~10,000 handles total)
//                         Creating/deleting fonts per paint would exhaust heap
//      - Interaction: Need to remember clickable areas between paint and mouse click
//      - Scrolling: Must track scroll position to translate coordinates
//
//    RESOURCE SAFETY GUARANTEES:
//      - All HFONT created in constructor -> deleted in destructor (no leaks)
//      - Image cache bounded (MAX_IMAGE_CACHE_SIZE) with LRU eviction
//      - Offscreen bitmap recreated on resize, old one deleted
//      - Exception safety: If GDI call fails, fallback to system defaults
//
//    MEMORY FOOTPRINT (Typical Page):
//      - 5 cached fonts: ~5KB GDI heap
//      - 10 cached images (100KB each): ~1MB RAM
//      - Display list (200 items * 64 bytes): ~13KB RAM
//      - Offscreen buffer (800x600x24bpp): ~1.4MB RAM
//      TOTAL: ~2.5MB (acceptable on Win98 with 64-128MB RAM)
//
// 3. PERFORMANCE OPTIMIZATION FOR LEGACY HARDWARE:
//    Every design choice optimized for 200MHz Pentium MMX, 64MB RAM, slow GDI:
//
//    DOUBLE-BUFFERING (Eliminates Flicker):
//      - Create offscreen DC + bitmap matching window size
//      - Render all content to offscreen surface
//      - Single BitBlt to screen DC (atomic, flicker-free)
//      - Cost: Extra 1-2MB RAM, but smooth 60fps scrolling worth it
//
//    CLIPPING (Draw Only Visible):
//      - Calculate visible region: [scrollY, scrollY + clientHeight]
//      - Skip RenderItems with bounds.top > visibleBottom or bounds.bottom < visibleTop
//      - Reduces 200-block page to ~20 blocks drawn (10x speedup)
//
//    FONT CACHING (Avoid Repeated CreateFont):
//      - Pre-create fonts in constructor: m_hFontH1, m_hFontH2, m_hFontP, m_hFontA
//      - Reuse via SelectObject (fast, <1ms vs 10ms for CreateFont)
//      - Only recreate on DPI change (rare on Win98, fixed 96 DPI typical)
//
//    BATCH GDI CALLS (Reduce SelectObject Overhead):
//      - Group blocks by type: Draw all H1s, then all Ps (minimize font switches)
//      - Sorting disabled by default (display order more important), but ready
//
//    SINGLE-PASS ALGORITHMS (No Recursion):
//      - Layout: One forward pass through blocks, no backtracking
//      - Render: One forward pass through display list, no sorting needed
//      - Stack safety: <100 bytes stack per call (Win98 default stack: 1MB)
//
//    TARGET BENCHMARKS (200MHz Pentium MMX):
//      - CalculateLayout: <200ms for 200 blocks (1ms/block average)
//      - Render:          <50ms for 20 visible blocks (<3ms/block)
//      - Scroll:          <16ms per frame (60fps capable)
//      - Memory:          <5MB per page (fits in 64MB system)
//
// 4. DATA-DRIVEN AND MINIMALIST API - WIN32 MESSAGE MAPPING:
//    The public API is designed to map directly to Win32 message loop patterns,
//    requiring minimal glue code in the UI layer. Data flows in (blocks, events),
//    results flow out (RenderResult, metrics), no callbacks or complex state machines.
//
//    API DESIGN PRINCIPLES:
//      - Message-driven: Each method corresponds to a Win32 message
//        * SetContent     -> After parsing new URL/page
//        * CalculateLayout-> WM_SIZE (window resize)
//        * Render         -> WM_PAINT
//        * HandleClick    -> WM_LBUTTONDOWN
//        * OnScroll       -> WM_VSCROLL / WM_MOUSEWHEEL
//        * OnResize       -> WM_SIZE (calls CalculateLayout + scrollbar update)
//
//      - Idempotent operations: Render() can be called repeatedly without side effects
//      - Stateless parameters: All context passed as arguments (no hidden state)
//      - Error via return values: No exceptions (C++98, Win32 convention)
//
//    DATA STRUCTURES:
//      - Input:  std::vector<Parser::HtmlBlock> (simple, copyable, STL)
//      - Output: RenderResult (status + diagnostics, mirrors parser.h pattern)
//      - Internal: std::vector<RenderItem> (display list, vector for cache locality)
//
//    LOOSE COUPLING:
//      - Renderer.h does NOT #include "network.h" (network only in renderer.cpp)
//      - Image loading abstracted: Renderer requests URL, Network module fetches
//      - No direct UI dependencies: Renderer doesn't know about address bars, etc.
//
//    EXAMPLE INTEGRATION (Minimal Glue Code):
//      ```cpp
//      static Renderer::HtmlRenderer g_renderer;
//      
//      LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
//          switch (msg) {
//              case WM_CREATE:  g_renderer = Renderer::HtmlRenderer(); break;
//              case WM_SIZE:    g_renderer.OnResize(hwnd); break;
//              case WM_PAINT:   { PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps);
//                                 g_renderer.Render(hwnd, hdc, ps.rcPaint, 0);
//                                 EndPaint(hwnd, &ps); } break;
//              case WM_VSCROLL: g_renderer.OnScroll(hwnd, LOWORD(wp), HIWORD(wp)); break;
//              case WM_LBUTTONDOWN: { std::string url;
//                                 if (g_renderer.HandleClick(GET_X_LPARAM(lp), 
//                                     GET_Y_LPARAM(lp), url)) LoadUrl(url); } break;
//          }
//          return DefWindowProc(hwnd, msg, wp, lp);
//      }
//      ```
//
// 5. INTERACTION-AWARE ROBUSTNESS - GRACEFUL DEGRADATION:
//    The renderer is designed to handle real-world failures and edge cases without
//    crashing or displaying blank content. Every potential failure has a fallback.
//
//    COORDINATE SYSTEM MANAGEMENT:
//      - Client coordinates: Mouse clicks, window rect (origin at window top-left)
//      - Document coordinates: Content layout (origin at document top, scrollable)
//      - Conversion: DocumentY = ClientY + scrollY (for hit-testing)
//      - Reverse: ClientY = DocumentY - scrollY (for rendering)
//
//    CLICKABLE REGION TRACKING:
//      - During layout: Store RECT bounds for each BLOCK_A in m_clickableAreas
//      - On click: Iterate areas, check if point-in-rect (with scroll adjustment)
//      - Return: href attribute string if hit, empty string if miss
//      - Optimization: Could use spatial index (quadtree) if >1000 links per page
//
//    IMAGE HANDLING (Multi-Stage Fallback):
//      1. Try m_imageCache lookup (instant if cached)
//      2. If miss, trigger async load (PostMessage to network thread, non-blocking)
//      3. While loading, display alt text (block.content field) in placeholder rect
//      4. If load succeeds, cache HBITMAP, invalidate rect for repaint
//      5. If load fails (404, decode error), keep displaying alt text permanently
//      BENEFIT: Page renders immediately with text, images "pop in" as they load
//
//    GDI FAILURE RECOVERY:
//      - CreateFont fails -> Use GetStockObject(SYSTEM_FONT) fallback
//      - CreateCompatibleDC fails -> Skip double-buffering, draw direct to screen
//      - CreateCompatibleBitmap fails -> Reduce offscreen buffer size, retry
//      - TextOut fails -> Log warning, skip block (don't crash entire page)
//
//    EDGE CASES HANDLED:
//      - Zero-size window: Clamp layout to minimum 100x100 (avoid divide-by-zero)
//      - Empty block list: Render blank page (no error, just clear background)
//      - Scroll beyond bounds: Clamp scrollY to [0, max(0, totalHeight - clientHeight)]
//      - Click on overlapping links: Return topmost link (last in display list)
//      - Very long text block: Word-wrap with DrawText DT_WORDBREAK, no truncation
//
// 6. DOCUMENTATION-DRIVEN ENCAPSULATION - SINGLE SOURCE OF TRUTH:
//    This header file is the complete contract for the Renderer module. A developer
//    should be able to integrate the renderer into their UI layer WITHOUT reading
//    the .cpp implementation file. Every struct, enum, and method is exhaustively
//    documented with WHAT it does, WHY design decisions were made, HOW to use it
//    correctly, and WHEN it fails.
//
//    DOCUMENTATION STANDARDS (Enforced):
//      - Purpose: First sentence explains function in plain English
//      - Parameters: Every parameter documented with type, meaning, constraints
//      - Return values: All possible return states explained
//      - Rationale: Design decisions justified (WHY not just WHAT)
//      - Examples: Code snippets show correct usage patterns
//      - Errors: All failure modes and recovery strategies listed
//      - Performance: Big-O complexity and timing estimates provided
//
//    INFORMATION HIDING:
//      - Public API: Only 9 methods (constructor, destructor, 7 operations)
//      - Private helpers: Declared in header for transparency, defined in .cpp
//      - Implementation details: Word-wrapping algorithm, image decoding, font
//        metrics calculation all hidden in .cpp (can be optimized without breaking API)
//
//    FORWARD COMPATIBILITY:
//      - Adding new BlockType: Only requires switch case in RenderBlock helper
//      - Adding new features (zoom, print): Add new public methods, existing code untouched
//      - Replacing GDI with GDI+: Change only .cpp internals, header API unchanged
//
// USAGE PATTERN (Production-Ready Integration Example):
// ```cpp
//     #include "parser.h"
//     #include "renderer.h"
//     #include "network.h"
//     
//     // Global or per-window state
//     static Renderer::HtmlRenderer g_renderer;
//     static std::vector<Parser::HtmlBlock> g_currentPage;
//     
//     // Initialize renderer when window created
//     case WM_CREATE: {
//         // Two-phase initialization: construct + explicit Initialize()
//         Renderer::RenderResult initResult = g_renderer.Initialize(hwnd);
//         if (initResult.status != Renderer::RENDER_SUCCESS) {
//             MessageBox(hwnd, initResult.errorMessage.c_str(), 
//                        "Renderer Init Failed", MB_ICONERROR);
//             return -1; // Abort window creation
//         }
//         // Log warnings (font fallbacks, etc.) for debugging
//         for (size_t i = 0; i < initResult.warnings.size(); ++i) {
//             OutputDebugString(initResult.warnings[i].c_str());
//         }
//         return 0;
//     }
//     
//     // Load new page after network fetch + parse
//     void LoadHtmlPage(const std::string& url) {
//         // 1. Fetch HTML from network (via proxy)
//         Network::HttpResponse response = FetchUrl(url);
//         if (response.status != Network::SUCCESS) {
//             MessageBox(NULL, "Network error", "Error", MB_ICONERROR);
//             return;
//         }
//         
//         // 2. Parse HTML into blocks
//         Parser::HtmlParser parser;
//         Parser::ParseResult parseResult = parser.Parse(response.body);
//         if (parseResult.status != Parser::PARSE_SUCCESS) {
//             MessageBox(NULL, parseResult.errorMessage.c_str(), "Parse Error", MB_ICONWARNING);
//             return;
//         }
//         
//         // 3. Load blocks into renderer (resets scroll, marks layout dirty)
//         g_renderer.SetContent(parseResult.blocks);
//         g_currentPage = parseResult.blocks; // Keep copy for debugging
//         
//         // 4. Calculate layout with current window size
//         RECT clientRect;
//         GetClientRect(hwnd, &clientRect);
//         g_renderer.CalculateLayout(hwnd, clientRect);
//         
//         // 5. Update scrollbar range based on content height
//         SCROLLINFO si = {0};
//         si.cbSize = sizeof(SCROLLINFO);
//         si.fMask = SIF_RANGE | SIF_PAGE;
//         si.nMin = 0;
//         si.nMax = g_renderer.GetTotalContentHeight();
//         si.nPage = clientRect.bottom - clientRect.top;
//         SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
//         
//         // 6. Force full repaint
//         InvalidateRect(hwnd, NULL, TRUE);
//     }
//     
//     // Render content on paint message
//     case WM_PAINT: {
//         PAINTSTRUCT ps;
//         HDC hdc = BeginPaint(hwnd, &ps);
//         
//         // Get current scroll position
//         SCROLLINFO si = {0};
//         si.cbSize = sizeof(SCROLLINFO);
//         si.fMask = SIF_POS;
//         GetScrollInfo(hwnd, SB_VERT, &si);
//         int scrollY = si.nPos;
//         
//         // Render visible content (fast, uses cached layout)
//         Renderer::RenderResult result = g_renderer.Render(hwnd, hdc, ps.rcPaint, scrollY);
//         
//         if (result.status != Renderer::RENDER_SUCCESS) {
//             // Fallback: Display error message in window
//             TextOut(hdc, 10, 10, result.errorMessage.c_str(), 
//                     result.errorMessage.length());
//         }
//         
//         // Log warnings for debugging (skipped images, GDI failures, etc.)
//         for (size_t i = 0; i < result.warnings.size(); ++i) {
//             OutputDebugString(result.warnings[i].c_str());
//         }
//         
//         EndPaint(hwnd, &ps);
//         return 0;
//     }
//     
//     // Handle window resize
//     case WM_SIZE: {
//         if (wParam != SIZE_MINIMIZED) {
//             g_renderer.OnResize(hwnd);
//             InvalidateRect(hwnd, NULL, TRUE);
//         }
//         return 0;
//     }
//     
//     // Handle vertical scrolling
//     case WM_VSCROLL: {
//         g_renderer.OnScroll(hwnd, LOWORD(wParam), HIWORD(wParam));
//         InvalidateRect(hwnd, NULL, TRUE);
//         return 0;
//     }
//     
//     // Handle mouse wheel scrolling (Win98 with IntelliMouse)
//     case WM_MOUSEWHEEL: {
//         int delta = GET_WHEEL_DELTA_WPARAM(wParam);
//         int scrollAmount = -delta / WHEEL_DELTA * 40; // 40 pixels per notch
//         g_renderer.OnScroll(hwnd, SB_THUMBPOSITION, scrollAmount);
//         InvalidateRect(hwnd, NULL, TRUE);
//         return 0;
//     }
//     
//     // Handle hyperlink clicks
//     case WM_LBUTTONDOWN: {
//         int x = GET_X_LPARAM(lParam);
//         int y = GET_Y_LPARAM(lParam);
//         
//         std::string clickedUrl;
//         if (g_renderer.HandleClick(x, y, clickedUrl)) {
//             // User clicked a link, navigate to the URL
//             LoadHtmlPage(clickedUrl);
//         }
//         return 0;
//     }
//     
//     // Handle cursor change on hover (optional polish)
//     case WM_SETCURSOR: {
//         POINT pt;
//         GetCursorPos(&pt);
//         ScreenToClient(hwnd, &pt);
//         
//         std::string dummyUrl;
//         if (g_renderer.HandleClick(pt.x, pt.y, dummyUrl)) {
//             // Over a link, show hand cursor
//             SetCursor(LoadCursor(NULL, IDC_HAND)); // IDC_HAND available on Win98+IE4
//             return TRUE;
//         }
//         break; // Default cursor
//     }
//     
//     // Clean up renderer on window destroy
//     case WM_DESTROY: {
//         // Renderer destructor automatically releases all GDI resources
//         // No explicit Cleanup() needed (RAII pattern)
//         PostQuitMessage(0);
//         return 0;
//     }
// ```
//
// TESTING RECOMMENDATIONS:
//     UNIT TESTS (renderer_test.cpp):
//       BASIC RENDERING:
//         - Empty content: SetContent([]), Render() -> Blank page, no errors
//         - Single text block: SetContent([TEXT "Hello"]), Render() -> Text at origin
//         - Multiple blocks: SetContent([H1, P, P]), Render() -> Vertically stacked
//         - Long text: SetContent([TEXT 500 chars]), Render() -> Word-wrapped to width
//
//       LAYOUT CALCULATION:
//         - Narrow window: CalculateLayout(RECT {0,0,200,600}) -> Text wraps, height increases
//         - Wide window: CalculateLayout(RECT {0,0,800,600}) -> Less wrapping, shorter height
//         - Zero size: CalculateLayout(RECT {0,0,0,0}) -> Doesn't crash, uses minimum
//         - Layout dirty flag: SetContent -> m_layoutDirty=true -> CalculateLayout -> false
//
//       FONT HANDLING:
//         - H1 rendering: Verify large font selected (24pt), bold weight
//         - H2 rendering: Verify medium font (18pt), bold
//         - P rendering: Verify normal font (12pt), regular weight
//         - Font cache: Render 100 P blocks -> Only 1 SelectObject(m_hFontP)
//         - Font creation fail: Mock CreateFont failure -> Falls back to system font
//
//       HYPERLINKS:
//         - Link rendering: BLOCK_A -> Blue color (#0000FF), underline
//         - Hit testing hit: HandleClick on link bounds -> Returns href, true
//         - Hit testing miss: HandleClick outside bounds -> Returns "", false
//         - Multiple links: Page with 10 links -> All independently clickable
//         - Overlapping links: Two links same position -> Returns topmost (last)
//
//       IMAGES:
//         - Image rendering: BLOCK_IMG with cached bitmap -> BitBlt to screen
//         - Alt text fallback: BLOCK_IMG, no bitmap -> Renders block.content
//         - Image cache hit: Render same IMG twice -> Uses cached HBITMAP, no reload
//         - Image cache eviction: Load 51 images -> LRU evicts oldest
//
//       SCROLLING:
//         - Scroll down: OnScroll(SB_LINEDOWN) -> scrollY increases by SCROLL_LINE_SIZE
//         - Scroll up: OnScroll(SB_LINEUP) -> scrollY decreases, clamped at 0
//         - Scroll bounds: OnScroll beyond max -> Clamped to totalHeight - clientHeight
//         - Page scroll: OnScroll(SB_PAGEDOWN) -> scrollY increases by page size
//         - Thumb tracking: OnScroll(SB_THUMBTRACK, pos) -> Sets scrollY to pos
//
//       COORDINATE TRANSFORMS:
//         - Client to document: Click at (10, 50), scrollY=100 -> Document coords (10, 150)
//         - Document to client: Item at Y=200, scrollY=100 -> Renders at Y=100
//
//       DOUBLE-BUFFERING:
//         - Verify offscreen DC: After Render, m_memDC != NULL
//         - Verify offscreen bitmap: m_offscreenBmp != NULL, correct dimensions
//         - Resize: OnResize -> Old bitmap deleted, new one created
//
//       EDGE CASES:
//         - NULL HDC: Render(hwnd, NULL, ...) -> Returns RENDER_FAILED, errorMessage set
//         - Empty visible rect: Render(..., rcPaint={0,0,0,0}) -> No crash, skip drawing
//         - Uninitialized content: Render without SetContent -> Renders blank (safe)
//         - Very tall page: SetContent with 1000 blocks -> Layout succeeds, scrollbar shown
//         - Narrow window: CalculateLayout with 100px width -> Word wrapping works
//
//     INTEGRATION TESTS:
//       - Full pipeline: Network fetch -> Parse -> SetContent -> CalculateLayout -> Render
//       - Click navigation: Load page, click link, verify new page loads
//       - Scroll interaction: Load long page, scroll down, verify content updates
//       - Resize handling: Load page, resize window, verify reflow and scrollbar update
//       - Image loading: Load page with images, verify BitBlt or alt text display
//
//     PERFORMANCE TESTS:
//       - Large page layout: CalculateLayout with 500 blocks -> Measure time, target <500ms
//       - Large page render: Render 500 blocks (20 visible) -> Measure time, target <50ms
//       - Scroll speed: Measure Render time during scroll -> Target <16ms (60fps)
//       - Memory usage: Load 50 pages sequentially -> Monitor RAM, verify cache eviction
//       - Font cache efficiency: Render 1000 blocks -> Verify CreateFont called only 5 times
//
//     STRESS TESTS:
//       - Rapid scrolling: Scroll up/down rapidly for 60 seconds -> No crashes or slowdown
//       - Rapid resize: Resize window continuously -> No memory leaks or GDI handle leaks
//       - Many images: Page with 200 images -> Cache evicts properly, no OOM
//       - Deep nesting: Parser output with many nested structures -> No stack overflow
//
//     GDI RESOURCE LEAK DETECTION:
//       - GetGuiResources before/after: Create renderer, load page, destroy -> Verify count same
//       - Task Manager monitoring: GDI Objects column during stress test -> Should be stable
//       - Repeated page loads: Load same page 1000 times -> GDI handle count doesn't grow
//
//     VISUAL REGRESSION TESTS:
//       - Capture screenshots of known pages, compare pixel-by-pixel after code changes
//       - Test pages: Simple text, headings, links, images, long scrollable content
//
// THREAD SAFETY:
//     NOT THREAD-SAFE. The HtmlRenderer class is designed for single-threaded use
//     within the Win32 browser's main message loop (UI thread). Do NOT call any
//     methods from multiple threads simultaneously.
//
//     GDI RESTRICTION: Windows 98 GDI is inherently single-threaded. All GDI calls
//                      (CreateFont, SelectObject, TextOut, BitBlt, etc.) must occur
//                      on the same thread that created the HDC. Violating this causes
//                      undefined behavior, including crashes, corruption, and resource leaks.
//
//     BACKGROUND OPERATIONS: If implementing asynchronous image loading in the future,
//                            the network fetch can occur on a worker thread, but the
//                            resulting HBITMAP creation and caching MUST be done on the
//                            UI thread via PostMessage callback.
//
// DEPENDENCIES:
//     - stdafx.h: Provides ALL system headers (windows.h, GDI32.lib, STL, Win32 types)
//                 This is the ONLY system include needed (precompiled header optimization)
//     
//     - parser.h: Defines HtmlBlock, BlockType, ParseResult (input data structures)
//                 Renderer consumes Parser output directly (tight coupling by design)
//     
//     - network.h (in renderer.cpp only, NOT in this header):
//                 Used by internal image loader to fetch BLOCK_IMG src URLs
//                 Loose coupling: Header doesn't expose network dependency
//     
//     - libjpeg (optional, linked in renderer.cpp):
//                 Used to decode JPEG images into DIB for CreateDIBitmap
//                 Win98 GDI supports BMP natively, JPEG requires external lib
//                 Fallback: If JPEG decode fails, display alt text
//
// LIMITATIONS (By Design - Out of Scope for 8-Week Academic Project):
//     LAYOUT LIMITATIONS:
//       - No CSS support: Inline styles ignored, no external stylesheets
//       - No floats: No "float: left" or "float: right" positioning
//       - No absolute positioning: No "position: absolute" or "top/left" properties
//       - No z-index: Blocks drawn in order, no layering control
//       - No tables: <table>, <tr>, <td> tags not supported (display as text)
//       - No flexbox/grid: Modern layout modes unavailable on Win98 GDI
//
//     RENDERING LIMITATIONS:
//       - No anti-aliasing: Text is aliased (jagged edges), GDI limitation on Win98
//       - No transparency: No alpha blending, no PNG transparency (use BMP/JPEG only)
//       - Static images only: No animated GIFs, no video, no <canvas>
//       - No custom fonts: Uses system fonts only (Arial, Times New Roman, Courier)
//       - No text shadows or outlines: Decorative effects unavailable
//       - No rotation or transforms: Text/images always axis-aligned
//
//     INTERACTION LIMITATIONS:
//       - No JavaScript: No dynamic content updates, no event handlers
//       - No forms: <input>, <select>, <textarea> not interactive (display as text)
//       - No drag-and-drop: No reordering elements or selecting text (future work)
//       - No zoom: Fixed 100% scale, no Ctrl+Plus/Minus zooming
//       - No print: No print preview or print-to-paper support
//
//     MULTIMEDIA LIMITATIONS:
//       - No audio: <audio> tag ignored, no sound playback
//       - No video: <video> tag ignored, no video playback
//       - No plugins: No Flash, no ActiveX, no browser plugins
//
//     ACCESSIBILITY LIMITATIONS:
//       - No screen reader support: ARIA attributes ignored
//       - No keyboard navigation: Tab key doesn't cycle through links (future work)
//       - No high contrast mode: Uses default system colors only
//
//     PERFORMANCE LIMITATIONS:
//       - Large images: Images >1MB may cause slowdown (recommend size limits in proxy)
//       - Many links: Pages with >1000 links may have slow hit-testing (no spatial index)
//       - Deep scrolling: Scrolling through >10,000 blocks may lag (need virtualization)
//
// MAINTAINABILITY:
//     To add rendering support for a new HTML block type (e.g., BLOCK_UL for lists):
//       1. Parser module: Add BLOCK_UL to BlockType enum in parser.h
//       2. Parser module: Update GetBlockType() to recognize "ul" tag
//       3. Renderer module: Add case in RenderBlock() helper method (renderer.cpp)
//       4. Renderer module: Implement list layout logic (bullet point, indentation)
//       5. Renderer module: Add unit test for BLOCK_UL rendering
//       6. No changes needed to this header file (extensible enum-based design)
//
//     To optimize rendering performance:
//       - Profile with Performance Monitor (Win98 tool) to identify hotspots
//       - Common targets: Word-wrapping (use fixed-width calculation), font selection,
//         image decoding (cache more aggressively), double-buffering overhead
//       - Optimization order: Measure first, optimize hottest path, re-measure
//
//     To add new features (e.g., print support):
//       - Add public method: void Print(HDC printerDC, int pageWidth, int pageHeight);
//       - Reuse existing layout engine (CalculateLayout with printer dimensions)
//       - Modify Render to support printer DC (different capabilities than screen DC)
//       - No changes to existing methods (backward compatible addition)
//
// FUTURE ENHANCEMENTS (Post-MVP, If Time Permits):
//     - Keyboard navigation: Tab cycles through links, Enter activates
//     - Text selection: Click-drag to select text, Ctrl+C to copy
//     - Find on page: Ctrl+F to search text, highlight matches
//     - Image scaling: Fit large images to window width, maintain aspect ratio
//     - Link hover effect: Underline becomes thicker on mouseover
//     - Smooth scrolling: Animated scroll instead of instant jump
//     - Back/forward cache: Keep last 10 rendered pages in memory for instant navigation
// ============================================================================

#pragma once
#ifndef RENDERER_H
#define RENDERER_H

#include "stdafx.h"      // Provides: windows.h, GDI (gdi32.lib), STL (string, vector, map)
#include "../parser/parser.h"  // Provides: Parser::HtmlBlock, Parser::BlockType

// ============================================================================
//  NAMESPACE: Renderer
//  Encapsulates all HTML rendering functionality to prevent global namespace
//  pollution and provide clear module boundaries. All public types and methods
//  are accessed via Renderer:: prefix.
// ============================================================================
namespace Renderer
{

// ============================================================================
// CONFIGURATION CONSTANTS (Tunable for Performance/Memory Trade-offs)
// ============================================================================

/// @brief Maximum number of fonts to cache (prevents unbounded GDI handle growth).
///        Each cached font consumes ~1KB of GDI heap. Win98 GDI heap is ~64KB total,
///        so 20 fonts is safe (20KB used, 44KB free for other apps).
///        TUNING: Increase if using many font sizes, decrease on low-memory systems.
const int MAX_FONT_CACHE_SIZE = 20;

/// @brief Maximum number of images to cache in memory (LRU eviction when exceeded).
///        Assuming 100KB per image average: 50 images = ~5MB RAM, acceptable on
///        Win98 systems with 64-128MB RAM. Images exceeding cache are re-downloaded.
///        TUNING: Increase on systems with more RAM, decrease for slower networks.
const int MAX_IMAGE_CACHE_SIZE = 50;

/// @brief Left/right margin from window edges (in pixels).
///        Content is inset by this amount to provide visual breathing room.
///        Standard web browsers use 8-10px, we use 10px for readability.
const int MARGIN_LEFT = 10;
const int MARGIN_RIGHT = 10;

/// @brief Top margin from window top edge (in pixels).
///        First block starts this far down from the window top.
const int MARGIN_TOP = 10;

/// @brief Line height multiplier for text blocks (relative to font height).
///        1.5 = 150% spacing, provides comfortable reading without excessive gaps.
///        W3C CSS default is 1.2, but 1.5 is better for low-res screens.
const float LINE_HEIGHT_MULTIPLIER = 1.5f;

/// @brief Vertical spacing ABOVE each block type (in pixels).
///        Different block types use different multiples of this base value:
///        - H1: 2.0x (20px above for visual prominence)
///        - H2: 1.5x (15px above)
///        - H3: 1.2x (12px above)
///        - P:  1.0x (10px above, standard paragraph spacing)
///        - A:  0.5x (5px above, tight for inline links)
const int BLOCK_SPACING_BASE = 10;

/// @brief Vertical spacing AFTER <br> tags (in pixels).
///        <br> forces a line break with this much extra vertical space.
const int BR_SPACING = 5;

/// @brief Default placeholder width for images when actual dimensions are unknown (in pixels).
///        Used during layout calculation before image is loaded or if width attribute missing.
const int IMAGE_PLACEHOLDER_WIDTH = 150;

/// @brief Default placeholder height for images when actual dimensions are unknown (in pixels).
///        Used during layout calculation before image is loaded or if height attribute missing.
const int IMAGE_PLACEHOLDER_HEIGHT = 100;

/// @brief Scrolling step size for arrow keys or SB_LINEUP/SB_LINEDOWN (in pixels).
///        Small scroll increments for precise positioning.
const int SCROLL_LINE_SIZE = 20;

/// @brief Scrolling step size for page up/down or SB_PAGEUP/SB_PAGEDOWN (in pixels).
///        Large scroll jumps for rapid navigation. Typically 80-90% of window height.
const int SCROLL_PAGE_SIZE = 200;

// ============================================================================
// COLOR CONSTANTS (RGB values for standard web colors)
// ============================================================================

/// @brief Hyperlink color (standard web blue: #0000FF).
///        Unvisited links are rendered in this color.
///        FUTURE: Track visited links, render in VISITED_LINK_COLOR (purple).
const COLORREF LINK_COLOR = RGB(0, 0, 255);

/// @brief Visited hyperlink color (standard web purple: #551A8B).
///        Currently unused (visited tracking not implemented), but defined for future.
const COLORREF VISITED_LINK_COLOR = RGB(85, 26, 139);

/// @brief Default text color (black: #000000).
///        Used for BLOCK_TEXT, BLOCK_P, BLOCK_H1/H2/H3 unless overridden.
const COLORREF TEXT_COLOR = RGB(0, 0, 0);

/// @brief Background color (white: #FFFFFF).
///        Window is cleared to this color before rendering content.
const COLORREF BACKGROUND_COLOR = RGB(255, 255, 255);

// ============================================================================
// FONT SIZE CONSTANTS (in points, 1 point = 1/72 inch)
// ============================================================================

/// @brief Font size for <h1> headings (24 points = ~32 pixels at 96 DPI).
///        Large and bold for maximum visual hierarchy.
const int FONT_SIZE_H1 = 24;

/// @brief Font size for <h2> headings (18 points = ~24 pixels at 96 DPI).
///        Medium-large and bold for sub-sections.
const int FONT_SIZE_H2 = 18;

/// @brief Font size for <h3> headings (14 points = ~19 pixels at 96 DPI).
///        Slightly larger than body text, bold for minor sections.
const int FONT_SIZE_H3 = 14;

/// @brief Font size for <p>, <a>, and default text (12 points = ~16 pixels at 96 DPI).
///        Standard body text size, readable at typical viewing distances on CRT monitors.
const int FONT_SIZE_DEFAULT = 12;

/**
 * @enum RenderStatus
 * @brief Defines the possible outcomes of a Render() operation.
 *        This enum provides fine-grained result reporting, allowing callers to
 *        distinguish between different success/failure modes and take appropriate
 *        action (e.g., display error message, fall back to text-only mode, retry).
 * 
 * DESIGN: Mirrors Parser::ParseStatus enum for consistency across modules.
 *         Always check this field FIRST in RenderResult before accessing other fields.
 */
enum RenderStatus
{
    /// @brief Rendering completed successfully. Content is visible on screen.
    ///        NOTE: This does NOT guarantee perfect rendering (some images may have
    ///        failed to load, some fonts may have fallen back to defaults), but the
    ///        core content is displayed. Check warnings vector for non-critical issues.
    ///        USAGE: Normal success case, no user-visible error.
    RENDER_SUCCESS,

    /// @brief The block list is empty, so there is nothing to render.
    ///        CAUSES:
    ///          - SetContent called with empty vector
    ///          - Parser returned zero blocks (empty HTML or all tags filtered)
    ///        USER ACTION: Display "No content available" or blank page (not an error).
    ///        NOTE: Not necessarily an error condition; some pages are legitimately empty.
    RENDER_EMPTY,

    /// @brief Invalid device context (HDC) was provided to Render().
    ///        CAUSES:
    ///          - NULL HDC passed as parameter
    ///          - HDC invalidated before Render() called (e.g., window destroyed)
    ///        USER ACTION: This is a programming error, not a user error. Log and debug.
    ///        RECOVERY: Render() returns immediately without drawing, no GDI calls made.
    RENDER_INVALID_DC,

    /// @brief Renderer has not been initialized (constructor not called or failed).
    ///        CAUSES:
    ///          - Render() called on uninitialized HtmlRenderer object
    ///          - Constructor failed to allocate GDI resources (rare but possible)
    ///        USER ACTION: Programming error or system resource exhaustion. Log and debug.
    ///        RECOVERY: Cannot render; caller should recreate renderer or show error.
    RENDER_NOT_INITIALIZED,

    /// @brief A critical GDI operation failed during rendering (e.g., BitBlt, TextOut).
    ///        CAUSES:
    ///          - Out of GDI resources (handle limit reached)
    ///          - Invalid device context state (rare, usually driver bug)
    ///          - Offscreen bitmap creation failed (out of video memory)
    ///        USER ACTION: Display error message, suggest closing other apps to free resources.
    ///        RECOVERY: Partial content may be visible; subsequent calls may succeed.
    RENDER_FAILED,

    /// @brief An unexpected or unhandled error occurred (should never happen in production).
    ///        CAUSES:
    ///          - Logic error in renderer code (bug)
    ///          - Corrupted internal state (memory corruption)
    ///          - Exception caught and converted to error code (defensive programming)
    ///        USER ACTION: File bug report with reproduction steps and error message.
    ///        RECOVERY: Undefined; may need to restart application.
    RENDER_UNKNOWN_ERROR
};

/**
 * @struct RenderResult
 * @brief A self-contained, immutable-after-return structure holding the complete
 *        result of a Render() operation. Designed for value semantics and robustness.
 * 
 * DESIGN PHILOSOPHY (mirrors Parser::ParseResult):
 *   - Self-documenting: All information needed to handle the result is included.
 *   - Error-first: Status field MUST be checked before accessing other fields.
 *   - Robustness: Includes both fatal errors (status != SUCCESS) and warnings
 *                 (non-fatal issues like missing images, font fallbacks).
 *   - Value semantics: Safe to copy/return by value (C++ RVO optimization applies).
 * 
 * USAGE RULES:
 *   1. ALWAYS check `status` field FIRST before using renderer output.
 *   2. If status != RENDER_SUCCESS, only `errorMessage` is valid (warnings may be empty).
 *   3. If status == RENDER_SUCCESS, content is rendered (check warnings for issues).
 *   4. Check `warnings` even on success to log skipped content (e.g., unsupported images).
 */
struct RenderResult
{
    /// @brief The overall rendering status. CHECK THIS FIRST.
    ///        Only when this equals RENDER_SUCCESS is the rendering considered successful.
    ///        Other values indicate partial or complete failure.
    RenderStatus status;

    /// @brief Human-readable error message for fatal failures.
    ///        BEHAVIOR:
    ///          - If status == RENDER_SUCCESS or RENDER_EMPTY: Empty string (no error).
    ///          - If status != RENDER_SUCCESS: Detailed diagnostic message including:
    ///              * Error type ("GDI failure", "Invalid HDC", "Not initialized")
    ///              * Context ("while rendering block 42", "during double-buffer setup")
    ///              * Suggested action ("Close other apps to free GDI resources", "Restart browser")
    ///        USAGE: Display in MessageBox, log to file, or show in browser error page.
    ///        EXAMPLE: "Render failed: BitBlt returned FALSE while copying offscreen buffer to screen DC. Out of video memory."
    std::string errorMessage;

    /// @brief Non-fatal issues encountered during rendering (informational).
    ///        POPULATED EVEN IF: status == RENDER_SUCCESS
    ///        EXAMPLES:
    ///          - "Image 'logo.png' failed to load, displaying alt text 'Company Logo'"
    ///          - "Font 'Arial Bold 24pt' creation failed, using system font fallback"
    ///          - "Skipped rendering block 17 (BLOCK_UNKNOWN type)"
    ///          - "Offscreen bitmap creation failed (low memory), rendering direct to screen"
    ///        USAGE: Log to debug output (OutputDebugString) or console for troubleshooting.
    ///                Typically hidden from end users unless debug mode enabled.
    ///        NOTE: Accumulates during rendering; may contain 0-50+ entries for complex pages.
    std::vector<std::string> warnings;

    /**
     * @brief Default constructor - initializes to a safe sentinel error state.
     *        Ensures uninitialized RenderResult objects are immediately detectable
     *        and won't cause undefined behavior if used incorrectly.
     */
    RenderResult()
        : status(RENDER_UNKNOWN_ERROR),
          errorMessage("Uninitialized RenderResult - Render() was never called")
    {
        // Warnings vector default-constructs to empty (no reserve needed)
    }

    /**
     * @brief Convenience constructor for success case (used internally by Render).
     * @param s The render status (typically RENDER_SUCCESS)
     */
    explicit RenderResult(RenderStatus s)
        : status(s),
          errorMessage(s == RENDER_SUCCESS || s == RENDER_EMPTY ? "" : "Unspecified error")
    {
    }
};

/**
 * @class HtmlRenderer
 * @brief Stateful GDI-based HTML rendering engine.
 *        Manages content, layout, resources, and painting for a single browser window.
 *        Designed for integration with Win32 message loop (WM_PAINT, WM_SIZE, etc.).
 * 
 * LIFETIME:
 *   Typically instantiate ONCE per window (e.g., static global or in window user data):
 *     static Renderer::HtmlRenderer g_renderer; // Lives for entire window lifetime
 *     // In WndProc:
 *     case WM_CREATE:  /* g_renderer constructed here (or earlier) */ break;
 *     case WM_PAINT:   g_renderer.Render(...); break;
 *     case WM_DESTROY: /* g_renderer destroyed here (or later) */ break;
 * 
 * STATEFUL DESIGN:
 *   Unlike stateless Parser, Renderer maintains per-window state:
 *     - Content: Current vector<HtmlBlock> loaded via SetContent()
 *     - Layout: Pre-computed display list (vector<RenderItem>) with positions
 *     - Scroll: Current scroll position (Y offset into document)
 *     - Resources: Cached GDI handles (fonts, bitmaps, offscreen DC)
 *     - Interaction: Clickable region map for hit-testing
 *   WHY? Performance (avoid recomputing layout on every paint) and interaction
 *        (need to remember click targets between events).
 * 
 * THREAD SAFETY:
 *   NOT thread-safe. All methods must be called from the UI thread (same thread
 *   as window message loop). GDI calls are inherently single-threaded on Win98.
 */
class HtmlRenderer
{
public:
    // ========================================================================
    // PUBLIC API - LIFECYCLE METHODS
    // ========================================================================

    /**
     * @brief Constructor - Initializes renderer to safe default state.
     *        Sets member variables to sentinel values but does NOT allocate GDI resources.
     *        MUST call Initialize() after construction to allocate resources and check errors.
     * 
     * OPERATIONS:
     *   1. Initialize member variables (scroll=0, layoutDirty=true, totalHeight=0)
     *   2. Set all GDI handles to NULL (m_hFontH1, m_memDC, etc.)
     *   3. Clear all collections (m_blocks, m_displayList, m_imageCache)
     * 
     * TWO-PHASE INITIALIZATION RATIONALE:
     *   Constructors in C++98 cannot report errors cleanly (no exceptions in this project).
     *   Separating construction from initialization allows:
     *     - Constructor: Always succeeds, creates safe empty object
     *     - Initialize(): May fail, returns detailed RenderResult with error info
     *   This follows Win32 COM pattern: CoCreateInstance (construct) + QueryInterface (init)
     * 
     * COST: <1ms (no GDI calls, just member initialization).
     * 
     * @note ALWAYS call Initialize() immediately after construction.
     * @note Do NOT call any other methods before Initialize() succeeds.
     * @note See Initialize() documentation for complete initialization process.
     */
    HtmlRenderer();

    /**
     * @brief Initializes GDI resources and prepares renderer for use.
     *        MUST be called after construction and BEFORE any rendering operations.
     *        This is the second phase of two-phase initialization.
     * 
     * @param hwnd The window handle (needed to get device context for font metrics).
     *             Pass the window this renderer will draw into.
     * 
     * @return RenderResult with initialization status:
     *         - RENDER_SUCCESS: All resources allocated successfully, renderer ready
     *         - RENDER_FAILED: Critical GDI resource creation failed (see errorMessage)
     *         - Warnings: Non-critical failures (font fallbacks, etc.)
     * 
     * OPERATIONS:
     *   1. Get device context: GetDC(hwnd) for font/bitmap creation
     *   2. Create cached fonts via CreateFont() for H1/H2/H3/P/A styles
     *      - If font creation fails: Use GetStockObject(SYSTEM_FONT) fallback
     *      - Add warning to result.warnings for each fallback
     *   3. Create cached brushes/pens for link color, background color
     *      - If brush creation fails: Use GetStockObject(WHITE_BRUSH) fallback
     *   4. Verify at least ONE font and brush created (critical requirement)
     *   5. Set internal m_initialized flag to true (checked by other methods)
     *   6. Release device context: ReleaseDC(hwnd, hdc)
     * 
     * FAILURE SCENARIOS:
     *   CRITICAL (returns RENDER_FAILED):
     *     - GetDC returns NULL (invalid window handle)
     *     - ALL font creation failed AND GetStockObject also failed (rare)
     *     - Out of GDI resources (Win98 GDI heap exhausted)
     *   
     *   NON-CRITICAL (returns RENDER_SUCCESS with warnings):
     *     - Some fonts failed but fallbacks work (common on font-limited systems)
     *     - Brush creation failed but stock brush available (acceptable)
     * 
     * ERROR HANDLING EXAMPLE:
     *   ```cpp
     *   Renderer::HtmlRenderer renderer;
     *   Renderer::RenderResult initResult = renderer.Initialize(hwnd);
     *   
     *   if (initResult.status != Renderer::RENDER_SUCCESS) {
     *       MessageBox(hwnd, initResult.errorMessage.c_str(), 
     *                  "Renderer Initialization Failed", MB_ICONERROR);
     *       return FALSE; // Abort window creation
     *   }
     *   
     *   // Log non-critical warnings (fonts that fell back to system font)
     *   for (size_t i = 0; i < initResult.warnings.size(); ++i) {
     *       OutputDebugString(initResult.warnings[i].c_str());
     *   }
     *   
     *   // Renderer ready for SetContent/CalculateLayout/Render
     *   ```
     * 
     * COST: ~50-100ms on Win98 (5 CreateFont calls + brush creation).
     *       Acceptable one-time cost on window creation.
     * 
     * IDEMPOTENCY:
     *   Safe to call multiple times (checks m_initialized flag, skips if already init).
     *   Subsequent calls return RENDER_SUCCESS immediately without re-creating resources.
     * 
     * @note MUST be called before SetContent/CalculateLayout/Render (checked via m_initialized).
     * @note Only call once per renderer lifetime (unless resources explicitly released).
     * @note If this fails, renderer is unusable - must destroy and recreate.
     */
    RenderResult Initialize(HWND hwnd);

    /**
     * @brief Destructor - Releases all GDI resources and cleans up state.
     *        Deletes cached fonts, bitmaps, DCs, and frees all dynamically allocated memory.
     * OPERATIONS:
     *   1. Delete all cached fonts (m_hFontH1, m_hFontH2, m_hFontP, etc.)
     *   2. Delete cached bitmaps (m_imageCache entries, m_offscreenBmp)
     *   3. Delete offscreen DC (m_memDC)
     *   4. Clear internal vectors (m_blocks, m_displayList, m_clickableAreas)
     * 
     * SAFETY GUARANTEES:
     *   - No GDI resource leaks: All DeleteObject/DeleteDC calls paired with Create calls
     *   - Exception-safe: Uses SAFE_DELETE macro (checks for NULL before delete)
     *   - Idempotent: Safe to call multiple times (e.g., if destructor re-entered)
     * 
     * COST: ~10-20ms on Win98 (deletion is faster than creation).
     *       Acceptable during window destruction (user doesn't see delay).
     * 
     * @note Automatically called when renderer goes out of scope or window destroyed.
     * @note No explicit Cleanup() method needed (RAII design philosophy).
     */
    ~HtmlRenderer();

    // ========================================================================
    // PUBLIC API - CONTENT MANAGEMENT
    // ========================================================================

    /**
     * @brief Loads new HTML content into the renderer, replacing previous content.
     *        This method is called after parsing a new page and prepares the renderer
     *        to display the new blocks. Does NOT immediately compute layout or draw;
     *        call CalculateLayout() and Render() separately.
     * 
     * @param blocks The vector of HtmlBlock structures from Parser::ParseResult.
     *               Can be empty (will display blank page).
     *               OWNERSHIP: Renderer makes internal COPY (safe to destroy original).
     * 
     * OPERATIONS:
     *   1. Store copy of blocks in m_blocks (std::vector copy assignment)
     *   2. Clear cached layout (m_displayList, m_clickableAreas)
     *   3. Reset scroll position to top (m_scrollY = 0)
     *   4. Mark layout as dirty (m_layoutDirty = true)
     *   5. Clear old image cache entries that are no longer referenced
     * 
     * STATE CHANGES:
     *   - m_blocks = blocks (copy)
     *   - m_scrollY = 0 (reset to top of page)
     *   - m_layoutDirty = true (forces CalculateLayout on next paint)
     *   - m_displayList.clear() (invalidate old layout)
     *   - m_clickableAreas.clear() (invalidate old hit regions)
     * 
     * PERFORMANCE:
     *   - Cost: O(n) where n = number of blocks (vector copy)
     *   - Timing: ~5-20ms for typical 200-block page on Win98
     *   - Optimization: Uses std::vector::reserve to minimize reallocations
     * 
     * USAGE PATTERN:
     *   ```cpp
     *   Parser::ParseResult parseResult = parser.Parse(htmlBytes);
     *   if (parseResult.status == Parser::PARSE_SUCCESS) {
     *       g_renderer.SetContent(parseResult.blocks);
     *       g_renderer.CalculateLayout(hwnd, clientRect); // Compute layout
     *       InvalidateRect(hwnd, NULL, TRUE);             // Trigger repaint
     *   }
     *   ```
     * 
     * @note Does NOT trigger layout or painting automatically (explicit control).
     * @note Safe to call multiple times (e.g., navigating between pages).
     * @note Empty blocks vector is valid (displays blank page, not an error).
     */
    void SetContent(const std::vector<Parser::HtmlBlock>& blocks);

    /**
     * @brief Computes layout for all blocks, preparing for fast rendering.
     *        This is the EXPENSIVE operation that measures text, wraps words, calculates
     *        positions, and builds the display list. Should be called ONLY when content
     *        changes (SetContent) or window resizes (WM_SIZE), NOT on every paint.
     * 
     * @param hwnd The window handle (needed for GetDC to create measurement DC).
     * @param clientRect The client area dimensions (content must fit within this).
     *                   Typically obtained via GetClientRect(hwnd, &rect).
     * 
     * OPERATIONS (Single-Pass Algorithm):
     *   1. Create temporary DC for text measurement (GetDC/CreateCompatibleDC)
     *   2. Initialize Y cursor to MARGIN_TOP
     *   3. For each block in m_blocks:
     *      a. Determine block type and select appropriate font
     *      b. Calculate text bounds using DrawText with DT_CALCRECT flag
     *         (measures WITHOUT drawing, accounts for word-wrap)
     *      c. Create RenderItem with computed RECT bounds
     *      d. If BLOCK_A, add clickable area to m_clickableAreas
     *      e. Advance Y cursor by block height + spacing
     *   4. Store total content height in m_totalContentHeight
     *   5. Clear layout dirty flag (m_layoutDirty = false)
     * 
     * COORDINATE SYSTEM:
     *   - Origin (0, 0) at top-left of client area (standard Win32)
     *   - X range: [MARGIN_LEFT, clientRect.right - MARGIN_RIGHT]
     *   - Y range: [MARGIN_TOP, MARGIN_TOP + totalContentHeight]
     *   - Positions are in DOCUMENT coordinates (absolute, independent of scroll)
     * 
     * WORD-WRAPPING:
     *   Uses DrawText with DT_WORDBREAK | DT_CALCRECT flags:
     *     - DT_WORDBREAK: Breaks lines at word boundaries (no mid-word breaks)
     *     - DT_CALCRECT: Returns bounding rect without drawing (measurement only)
     *     - Respects content width: (clientRect.right - MARGIN_LEFT - MARGIN_RIGHT)
     *   EXAMPLE: 500-char text in 400px wide window might wrap to 8 lines, 120px tall
     * 
     * PERFORMANCE:
     *   - Time Complexity: O(n) where n = number of blocks (single pass)
     *   - Cost per block: ~1-2ms for DrawText measurement
     *   - Total cost: 100 blocks = ~150ms, 500 blocks = ~800ms on 200MHz Pentium
     *   - Bottleneck: DrawText is slowest operation (GDI text measurement)
     * 
     * OPTIMIZATION OPPORTUNITIES:
     *   - Cache font metrics (avoid SelectObject per block if same font)
     *   - Estimate height without DrawText for simple TEXT blocks (faster but less accurate)
     *   - Limit layout to visible region + buffer (virtualization for huge pages)
     * 
     * STATE CHANGES:
     *   - m_displayList populated with RenderItem structs (cleared first)
     *   - m_clickableAreas populated with BLOCK_A regions (cleared first)
     *   - m_totalContentHeight set to sum of all block heights + spacing
     *   - m_layoutDirty set to false (layout now valid)
     * 
     * USAGE PATTERN:
     *   ```cpp
     *   // Call after SetContent or WM_SIZE
     *   case WM_SIZE: {
     *       if (wParam != SIZE_MINIMIZED) {
     *           RECT clientRect;
     *           GetClientRect(hwnd, &clientRect);
     *           g_renderer.CalculateLayout(hwnd, clientRect);
     *           
     *           // Update scrollbar range
     *           SCROLLINFO si = {0};
     *           si.cbSize = sizeof(SCROLLINFO);
     *           si.fMask = SIF_RANGE | SIF_PAGE;
     *           si.nMin = 0;
     *           si.nMax = g_renderer.GetTotalContentHeight();
     *           si.nPage = clientRect.bottom - clientRect.top;
     *           SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
     *           
     *           InvalidateRect(hwnd, NULL, TRUE);
     *       }
     *       return 0;
     *   }
     *   ```
     * 
     * @note MUST call this before Render() after SetContent() (else nothing displays).
     * @note Safe to call multiple times (idempotent if content/size unchanged).
     * @note Automatically sets m_layoutDirty to false when complete.
     */
    void CalculateLayout(HWND hwnd, const RECT& clientRect);

    // ========================================================================
    // PUBLIC API - RENDERING
    // ========================================================================

    /**
     * @brief Renders visible content to the screen, using pre-computed layout.
     *        This is the FAST operation called on every WM_PAINT. Uses double-buffering
     *        to eliminate flicker and clips to visible region for performance.
     * 
     * @param hwnd The window handle (needed for GetDC if creating offscreen DC).
     * @param hdc The device context to render into (from BeginPaint).
     *            MUST be valid (non-NULL); returns RENDER_INVALID_DC if NULL.
     * @param visibleRect The invalidated region (from PAINTSTRUCT.rcPaint).
     *                    Only blocks intersecting this rect are drawn (optimization).
     *                    Pass full client rect to force redraw everything.
     *                    IMPLEMENTATION NOTE: For accurate clipping, use Win32 IntersectRect
     *                    to find intersection between visibleRect and each RenderItem's bounds
     *                    (after translating to client coordinates). Only draw items where
     *                    IntersectRect returns TRUE (non-empty intersection). This ensures
     *                    partially visible blocks are rendered correctly without overdraw.
     * 
     * @return RenderResult containing status, error message, and warnings.
     *         CHECK result.status before assuming success.
     * 
     * STATEFUL DESIGN NOTE:
     *   This method uses internal scroll state (m_scrollY) maintained by OnScroll().
     *   No need to pass scroll position as parameter - renderer manages its own state.
     *   RATIONALE: Follows stateful architecture principle - renderer owns all rendering
     *   state including scroll position. Simplifies caller interface and ensures consistency.
     * 
     * OPERATIONS (Double-Buffering Algorithm):
     *   1. VALIDATION:
     *      - Check if hdc is valid (non-NULL)
     *      - Check if layout is computed (m_layoutDirty == false)
     *      - If invalid, return error immediately (no drawing)
     *   
     *   2. OFFSCREEN BUFFER SETUP:
     *      - Create/reuse offscreen DC (m_memDC) compatible with screen DC
     *      - Create/reuse offscreen bitmap (m_offscreenBmp) matching client size
     *      - Select offscreen bitmap into offscreen DC
     *      - Clear offscreen DC to BACKGROUND_COLOR (white)
     *   
     *   3. CLIPPING CALCULATION:
     *      - Determine visible range using internal m_scrollY: [m_scrollY, m_scrollY + visibleRect.bottom]
     *      - Filter m_displayList to only blocks intersecting visible range
     *      - Optimization: Early-out for blocks entirely above/below visible area
     *   
     *   4. BLOCK RENDERING (Single Pass):
     *      For each visible RenderItem in display list:
     *        a. Translate coordinates: clientY = documentY - m_scrollY (internal state)
     *        b. Select appropriate font based on block type
     *        c. Set text color (blue for BLOCK_A, black for others)
     *        d. Render based on type:
     *           - TEXT/P/H1/H2/H3: TextOut or DrawText for multi-line
     *           - BLOCK_A: TextOut with underline + blue color
     *           - BLOCK_IMG: BitBlt cached image OR TextOut alt text if no image
     *           - BLOCK_BR: Skip (just advances Y, no visual output)
     *        e. Store actual rendered bounds for debugging (optional)
     *   
     *   5. FINAL BLIT:
     *      - BitBlt offscreen DC -> screen DC (atomic, flicker-free copy)
     *      - Timing: Single BitBlt is ~1-3ms for 800x600 window
     *   
     *   6. CLEANUP:
     *      - Deselect objects from offscreen DC (restore previous state)
     *      - Do NOT delete offscreen DC/bitmap (cache for next paint)
     * 
     * COORDINATE TRANSFORM EXAMPLE:
     *   - Block has document bounds: {top: 500, bottom: 520}
     *   - Current scrollY: 300
     *   - Client Y position: 500 - 300 = 200 (rendered at Y=200 on screen)
     *   - If scrollY = 600, client Y = 500 - 600 = -100 (above visible area, skipped)
     * 
     * PERFORMANCE:
     *   - Time Complexity: O(v) where v = visible blocks (typically v << n)
     *   - Cost per block: ~2-5ms for TextOut/BitBlt
     *   - Total cost: 20 visible blocks = ~50ms on 200MHz Pentium (20fps capable)
     *   - Target: <16ms for 60fps (achievable with 10-15 visible blocks)
     * 
     * DOUBLE-BUFFERING BENEFIT:
     *   Without: Flicker visible (background clear, then text draw, causes flash)
     *   With: Smooth (render offscreen, single atomic blit to screen)
     *   Cost: 1-2MB extra RAM for offscreen bitmap (acceptable trade-off)
     * 
     * CLIPPING BENEFIT:
     *   Without: Render all 500 blocks even if only 20 visible (25x slower)
     *   With: Render only 20 visible blocks (fast scrolling)
     *   Example: Page with 1000 blocks, scrolled to middle, only 25 blocks drawn
     * 
     * ERROR HANDLING:
     *   - NULL hdc: Return RENDER_INVALID_DC immediately
     *   - Layout not computed: Return RENDER_NOT_INITIALIZED
     *   - CreateCompatibleDC fails: Fall back to direct rendering (no double-buffer)
     *   - CreateCompatibleBitmap fails: Try smaller size, or skip offscreen buffer
     *   - TextOut/BitBlt fails: Log warning, continue to next block (partial render)
     * 
     * USAGE PATTERN:
     *   ```cpp
     *   case WM_PAINT: {
     *       PAINTSTRUCT ps;
     *       HDC hdc = BeginPaint(hwnd, &ps);
     *       
     *       // Render visible content (scroll position managed internally by renderer)
     *       Renderer::RenderResult result = g_renderer.Render(hwnd, hdc, ps.rcPaint);
     *       
     *       if (result.status != Renderer::RENDER_SUCCESS) {
     *           // Display error message in window
     *           SetTextColor(hdc, RGB(255, 0, 0)); // Red error text
     *           TextOut(hdc, 10, 10, result.errorMessage.c_str(), 
     *                   result.errorMessage.length());
     *       }
     *       
     *       // Log non-critical warnings (debug builds only)
     *       #ifdef _DEBUG
     *       for (size_t i = 0; i < result.warnings.size(); ++i) {
     *           OutputDebugString(result.warnings[i].c_str());
     *           OutputDebugString("\n");
     *       }
     *       #endif
     *       
     *       EndPaint(hwnd, &ps);
     *       return 0;
     *   }
     *   ```
     * 
     * @note IDEMPOTENT: Safe to call multiple times with same parameters (no state change).
     * @note FAST PATH: Optimized for repeated calls during scrolling/animation.
     * @note Requires CalculateLayout() called first (checked via m_layoutDirty flag).
     * @note STATEFUL: Uses internal m_scrollY managed by OnScroll() - no scroll param needed.
     */
    RenderResult Render(HWND hwnd, HDC hdc, const RECT& visibleRect);

    // ========================================================================
    // PUBLIC API - METRICS AND QUERIES
    // ========================================================================

    /**
     * @brief Returns the total height of all content (sum of all block heights).
     *        Used to set scrollbar range and determine maximum scroll position.
     * 
     * @return Total content height in pixels (document coordinates).
     *         Returns 0 if no content loaded or layout not computed.
     * 
     * USAGE:
     *   - Set scrollbar range: SetScrollInfo(hwnd, SB_VERT, nMax = GetTotalContentHeight())
     *   - Clamp scroll position: scrollY = min(scrollY, GetTotalContentHeight() - clientHeight)
     *   - Detect scrollable content: if (GetTotalContentHeight() > clientHeight) { show scrollbar }
     * 
     * COMPUTED IN:
     *   CalculateLayout() - sums all block heights + spacing
     * 
     * TYPICAL VALUES:
     *   - Simple page: 500-2000 pixels (fits in ~600px window, minimal scrolling)
     *   - Article page: 3000-10000 pixels (requires scrolling)
     *   - Very long page: 50000+ pixels (thousands of blocks)
     * 
     * EXAMPLE:
     *   ```cpp
     *   RECT clientRect;
     *   GetClientRect(hwnd, &clientRect);
     *   int clientHeight = clientRect.bottom - clientRect.top;
     *   int contentHeight = g_renderer.GetTotalContentHeight();
     *   
     *   if (contentHeight > clientHeight) {
     *       // Content exceeds window, show scrollbar
     *       ShowScrollBar(hwnd, SB_VERT, TRUE);
     *       
     *       SCROLLINFO si = {0};
     *       si.cbSize = sizeof(SCROLLINFO);
     *       si.fMask = SIF_RANGE | SIF_PAGE;
     *       si.nMin = 0;
     *       si.nMax = contentHeight - 1; // -1 because nMax is inclusive
     *       si.nPage = clientHeight;
     *       SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
     *   } else {
     *       // Content fits entirely, hide scrollbar
     *       ShowScrollBar(hwnd, SB_VERT, FALSE);
     *   }
     *   ```
     * 
     * @note Returns cached value (fast O(1) lookup, no recalculation).
     * @note Value is 0 if CalculateLayout() not called yet (layout not computed).
     */
    int GetTotalContentHeight() const;

    // ========================================================================
    // PUBLIC API - INTERACTION HANDLING
    // ========================================================================

    /**
     * @brief Performs hit-testing to determine if a mouse click landed on a hyperlink.
     *        Searches clickable regions (computed during layout) and returns the href
     *        URL if a link was clicked, or empty string if clicked on non-interactive content.
     * 
     * @param x Mouse X coordinate in CLIENT coordinates (from WM_LBUTTONDOWN lParam).
     *          Extract via: x = GET_X_LPARAM(lParam) or LOWORD(lParam).
     * @param y Mouse Y coordinate in CLIENT coordinates (from WM_LBUTTONDOWN lParam).
     *          Extract via: y = GET_Y_LPARAM(lParam) or HIWORD(lParam).
     * @param outHref [OUT] The href URL of the clicked link (only valid if return true).
     *                Passed by reference to avoid string copy overhead.
     *                OWNERSHIP: Caller receives copy of URL string (safe to use after return).
     * 
     * @return True if click landed on a hyperlink (outHref contains URL).
     *         False if click landed on non-link content (outHref unchanged).
     * 
     * ALGORITHM:
     *   1. Translate client coordinates to document coordinates:
     *      documentY = clientY + m_scrollY (accounts for scroll offset)
     *   2. Iterate m_clickableAreas vector (populated during CalculateLayout)
     *   3. For each clickable area:
     *      - Check if point (x, documentY) is inside area.rect (PtInRect)
     *      - If inside: Set outHref = area.href, return true
     *   4. If no match found: Return false (outHref unchanged)
     * 
     * OPTIMIZATION:
     *   - Current: Linear search O(n) where n = number of links
     *   - Fast for typical pages (<100 links): ~0.1ms per click
     *   - Slow for link-heavy pages (>1000 links): ~10ms per click
     *   - Future: Use spatial index (R-tree, quadtree) for O(log n) lookup
     * 
     * OVERLAPPING LINKS:
     *   If multiple links occupy the same position (stacked):
     *     - Returns LAST link in m_clickableAreas (topmost in z-order)
     *     - Rationale: Last drawn link is visually on top, should be clickable
     * 
     * EDGE CASES:
     *   - Click outside window bounds: Returns false (safe, no crash)
     *   - Click during layout: Returns false (m_clickableAreas not yet populated)
     *   - Click on link border (1px tolerance): Includes border in hit region
     * 
     * USAGE PATTERN:
     *   ```cpp
     *   case WM_LBUTTONDOWN: {
     *       int x = GET_X_LPARAM(lParam);
     *       int y = GET_Y_LPARAM(lParam);
     *       
     *       std::string clickedUrl;
     *       if (g_renderer.HandleClick(x, y, clickedUrl)) {
     *           // User clicked a hyperlink, navigate to the URL
     *           LoadUrl(clickedUrl); // Fetch new page and render
     *       } else {
     *           // Click on non-interactive content, ignore or handle differently
     *           // (e.g., text selection, context menu)
     *       }
     *       return 0;
     *   }
     *   ```
     * 
     * COORDINATE EXAMPLE:
     *   - Click at client position (100, 250)
     *   - Current scroll offset m_scrollY = 500
     *   - Document position = (100, 250 + 500) = (100, 750)
     *   - Link has bounds {left: 50, top: 700, right: 200, bottom: 720}
     *   - Point (100, 750) is inside bounds -> Return link href
     * 
     * @note Fast O(n) operation, negligible cost for typical pages.
     * @note Does NOT modify renderer state (const-safe except for scroll position).
     * @note Safe to call before layout computed (returns false, no crash).
     */
    bool HandleClick(int x, int y, std::string& outHref);

    /**
     * @brief Handles vertical scrolling in response to WM_VSCROLL or WM_MOUSEWHEEL.
     *        Updates internal scroll position, clamps to valid range, and updates
     *        the window scrollbar position. Does NOT trigger repaint (caller must invalidate).
     * 
     * @param hwnd The window handle (needed to update scrollbar with SetScrollInfo).
     * @param scrollType The scroll action type (from LOWORD(wParam) in WM_VSCROLL).
     *                   Valid values:
     *                     - SB_LINEUP:        Scroll up by one line (SCROLL_LINE_SIZE pixels)
     *                     - SB_LINEDOWN:      Scroll down by one line
     *                     - SB_PAGEUP:        Scroll up by one page (SCROLL_PAGE_SIZE pixels)
     *                     - SB_PAGEDOWN:      Scroll down by one page
     *                     - SB_THUMBTRACK:    User dragging scrollbar thumb (use scrollPos param)
     *                     - SB_THUMBPOSITION: User released scrollbar thumb (use scrollPos param)
     *                     - SB_TOP:           Scroll to top of document (scrollY = 0)
     *                     - SB_BOTTOM:        Scroll to bottom of document
     * @param scrollPos The thumb position (from HIWORD(wParam) in WM_VSCROLL).
     *                  Only used when scrollType == SB_THUMBTRACK or SB_THUMBPOSITION.
     *                  Ignored for other scroll types (calculated internally instead).
     * 
     * OPERATIONS:
     *   1. Calculate scroll delta based on scrollType:
     *      - LINE: +/- SCROLL_LINE_SIZE (20px default)
     *      - PAGE: +/- SCROLL_PAGE_SIZE (200px default)
     *      - THUMB: Set to scrollPos directly
     *      - TOP/BOTTOM: Set to 0 or max
     *   2. Update m_scrollY: m_scrollY += delta
     *   3. Clamp m_scrollY to valid range: [0, max(0, m_totalContentHeight - clientHeight)]
     *   4. Update scrollbar position: SetScrollInfo(hwnd, SB_VERT, m_scrollY)
     * 
     * SCROLL RANGE CLAMPING:
     *   - Minimum: 0 (top of document, cannot scroll above origin)
     *   - Maximum: totalContentHeight - clientHeight (bottom of document)
     *   - If content shorter than window: max = 0 (no scrolling possible)
     *   - Ensures content never scrolls past bottom edge
     * 
     * PERFORMANCE:
     *   - Cost: O(1) arithmetic + SetScrollInfo (~0.1ms)
     *   - Does NOT repaint (caller must InvalidateRect for visual update)
     *   - Rationale: Allows batching multiple scroll operations before single repaint
     * 
     * SMOOTH SCROLLING (Future Enhancement):
     *   Current: Instant jump to new position (acceptable for line/page scroll)
     *   Future: Animate m_scrollY over time (10-20 frames) for smooth visual transition
     *           Requires timer (SetTimer) and incremental InvalidateRect per frame
     * 
     * USAGE PATTERN:
     *   ```cpp
     *   case WM_VSCROLL: {
     *       g_renderer.OnScroll(hwnd, LOWORD(wParam), HIWORD(wParam));
     *       InvalidateRect(hwnd, NULL, TRUE); // Trigger repaint with new scroll
     *       return 0;
     *   }
     *   
     *   case WM_MOUSEWHEEL: {
     *       // Mouse wheel: Negative delta = scroll down, positive = scroll up
     *       int delta = GET_WHEEL_DELTA_WPARAM(wParam);
     *       int scrollAmount = -delta / WHEEL_DELTA * 40; // 40 pixels per notch
     *       
     *       // Use SB_THUMBPOSITION to set absolute position
     *       SCROLLINFO si = {0};
     *       si.cbSize = sizeof(SCROLLINFO);
     *       si.fMask = SIF_POS;
     *       GetScrollInfo(hwnd, SB_VERT, &si);
     *       int newPos = si.nPos + scrollAmount;
     *       
     *       g_renderer.OnScroll(hwnd, SB_THUMBPOSITION, newPos);
     *       InvalidateRect(hwnd, NULL, TRUE);
     *       return 0;
     *   }
     *   ```
     * 
     * @note Does NOT repaint automatically (explicit InvalidateRect required for visual update).
     * @note Safe to call multiple times rapidly (e.g., holding arrow key down).
     * @note Automatically clamps to valid range (no out-of-bounds scrolling possible).
     */
    void OnScroll(HWND hwnd, int scrollType, int scrollPos);

    /**
     * @brief Handles window resize by recalculating layout and updating scrollbar.
     *        Convenience wrapper for CalculateLayout + scrollbar setup. Called in response
     *        to WM_SIZE message. Marks layout dirty and triggers full relayout with new dimensions.
     * 
     * @param hwnd The window handle (needed for GetClientRect and SetScrollInfo).
     * 
     * OPERATIONS:
     *   1. Get new client area dimensions: GetClientRect(hwnd, &clientRect)
     *   2. If minimized (clientRect empty): Return early (no layout needed)
     *   3. Call CalculateLayout(hwnd, clientRect) to recompute positions with new width
     *   4. Recreate offscreen bitmap with new dimensions (old one deleted)
     *   5. Update scrollbar range and page size: SetScrollInfo(SIF_RANGE | SIF_PAGE)
     *   6. Clamp current scroll position to new max (in case content shortened)
     * 
     * WHY RELAYOUT ON RESIZE?
     *   - Text word-wrapping depends on available width
     *   - Wider window: Less wrapping, shorter page height
     *   - Narrower window: More wrapping, taller page height
     *   - Must recompute all block heights to maintain correct layout
     * 
     * PERFORMANCE:
     *   - Cost: Same as CalculateLayout (~150ms for 200 blocks)
     *   - User perception: Brief freeze during drag-resize (acceptable on Win98)
     *   - Optimization: Defer layout until resize complete (timer-based debounce)
     * 
     * USAGE PATTERN:
     *   ```cpp
     *   case WM_SIZE: {
     *       if (wParam != SIZE_MINIMIZED) {
     *           g_renderer.OnResize(hwnd);
     *           InvalidateRect(hwnd, NULL, TRUE); // Trigger repaint with new layout
     *       }
     *       return 0;
     *   }
     *   ```
     * 
     * @note Automatically calls CalculateLayout internally (no separate call needed).
     * @note Does NOT repaint automatically (caller must InvalidateRect).
     * @note Safe to call when minimized (early-out, no work done).
     */
    void OnResize(HWND hwnd);

    // ========================================================================
    // PUBLIC API - ASYNCHRONOUS RESOURCE MANAGEMENT
    // ========================================================================

    /**
     * @brief Notifies the renderer that an image has been loaded asynchronously.
     *        This creates an explicit "contract" for the Network/UI layer to push
     *        loaded image data into the renderer, rather than relying on implicit
     *        Windows message handling (WM_USER_IMAGE_LOADED).
     * 
     * @param src The image URL that was requested (from BLOCK_IMG attributes["src"]).
     *            Must match the URL originally passed to LoadImage internal method.
     * @param hBitmap The decoded GDI bitmap handle (result of DecodeImage).
     *                OWNERSHIP: Renderer takes ownership, will DeleteObject on cache eviction.
     *                Pass NULL if image loading/decoding failed (renderer keeps alt text).
     * 
     * OPERATIONS:
     *   1. Validate src is non-empty and hBitmap is valid (non-NULL for success)
     *   2. Add/update entry in m_imageCache: m_imageCache[src] = hBitmap
     *   3. Update LRU list: Move src to front of m_imageCacheLRU (mark as most recent)
     *   4. If cache exceeds MAX_IMAGE_CACHE_SIZE:
     *      - Evict least recently used entry (back of m_imageCacheLRU)
     *      - DeleteObject on evicted HBITMAP to prevent GDI handle leak
     *   5. If hBitmap is NULL (load failed): Store NULL in cache to prevent re-requests
     * 
     * ASYNCHRONOUS LOADING PATTERN:
     *   This method completes the async image loading workflow:
     * 
     *   PHASE 1 - Image Request (during Render):
     *     1. Render encounters BLOCK_IMG
     *     2. Calls internal LoadImage(src) -> returns NULL (cache miss)
     *     3. LoadImage posts message to network thread: "Fetch src URL"
     *     4. Render displays alt text as placeholder (continues without blocking)
     * 
     *   PHASE 2 - Image Loading (background thread):
     *     1. Network thread fetches image bytes from proxy/server
     *     2. Network thread decodes bytes -> HBITMAP (or NULL on failure)
     *     3. Network thread posts message to UI thread with result
     * 
     *   PHASE 3 - Image Caching (THIS METHOD):
     *     1. UI thread WM_USER_IMAGE_LOADED handler calls NotifyImageLoaded(src, hBitmap)
     *     2. Renderer caches bitmap and updates LRU
     *     3. UI thread calls InvalidateRect to trigger repaint
     *     4. Next Render call uses cached bitmap (LoadImage returns non-NULL)
     * 
     * ALTERNATIVE DESIGN COMPARISON:
     *   Old implicit pattern (using Windows messages directly):
     *     - Renderer handles WM_USER_IMAGE_LOADED internally
     *     - Tight coupling between renderer and message handling
     *     - Hard to test (requires posting actual Windows messages)
     * 
     *   New explicit pattern (using this method):
     *     - Renderer exposes public API for image notification
     *     - Clear contract: "When image ready, call this method"
     *     - Easy to test (call method directly, no message loop needed)
     *     - Loose coupling: Caller decides how to signal completion (message, callback, etc.)
     * 
     * USAGE PATTERN:
     *   ```cpp
     *   // In Network/UI layer after async image load completes:
     *   case WM_USER_IMAGE_LOADED: {
     *       const char* imageUrl = (const char*)lParam;
     *       HBITMAP hLoadedBitmap = (HBITMAP)wParam; // Or extract from custom struct
     *       
     *       // Push loaded image into renderer
     *       g_renderer.NotifyImageLoaded(imageUrl, hLoadedBitmap);
     *       
     *       // Trigger repaint to display newly loaded image
     *       InvalidateRect(hwnd, NULL, TRUE);
     *       
     *       return 0;
     *   }
     *   
     *   // For testing (no message loop needed):
     *   HBITMAP testBitmap = LoadBitmapFromFile("test.bmp");
     *   renderer.NotifyImageLoaded("http://example.com/logo.png", testBitmap);
     *   // Next Render() will use cached bitmap
     *   ```
     * 
     * ERROR HANDLING:
     *   - NULL hBitmap: Cached as NULL (prevents retry spam), alt text displayed permanently
     *   - Empty src: Ignored (no operation, returns silently)
     *   - Duplicate notification: Updates cache (replaces old bitmap, deletes if different)
     * 
     * PERFORMANCE:
     *   - Cost: O(log n) for map insertion + O(n) for LRU update (n = cache size, typically <50)
     *   - Timing: <1ms for typical cache operations
     *   - LRU eviction: ~2ms when cache full (delete old bitmap + remove from structures)
     * 
     * @note Thread-safe ONLY if called from UI thread (GDI limitation on Win98).
     * @note Caller must InvalidateRect after this call to trigger repaint.
     * @note Renderer takes ownership of hBitmap (will DeleteObject on eviction/destruction).
     */
    void NotifyImageLoaded(const std::string& src, HBITMAP hBitmap);

private:
    // ========================================================================
    // NON-COPYABLE DESIGN (C++98 Pattern)
    // ========================================================================
    // Disable copy constructor and assignment operator to prevent accidental
    // resource duplication (GDI handles, cached bitmaps, etc.). Copying a renderer
    // would require deep-copying all GDI resources, which is expensive and error-prone.
    // In C++11+, we would use "= delete" syntax instead.
    
    /**
     * @brief Private copy constructor (unimplemented) - prevents copying.
     * @note Declared but not defined; linker error if accidentally invoked.
     */
    HtmlRenderer(const HtmlRenderer&);
    
    /**
     * @brief Private assignment operator (unimplemented) - prevents assignment.
     * @note Declared but not defined; linker error if accidentally invoked.
     */
    HtmlRenderer& operator=(const HtmlRenderer&);

    // ========================================================================
    // PRIVATE HELPER METHODS (Implementation Details)
    // ========================================================================
    // These methods are declared here for transparency and maintainability,
    // allowing header readers to understand the module structure without
    // reading the .cpp file. They remain private to preserve encapsulation.

    /**
     * @brief Creates and caches all GDI resources (fonts, brushes, pens).
     *        Called once during construction to pre-allocate expensive resources.
     *        If creation fails, uses GetStockObject fallbacks (never NULL).
     */
    void PrepareGdiResources();

    /**
     * @brief Releases all GDI resources (DeleteObject on fonts, bitmaps, brushes).
     *        Called once during destruction to prevent resource leaks.
     *        Safe to call multiple times (checks for NULL before deleting).
     */
    void ReleaseGdiResources();

    /**
     * @brief Builds the display list from m_blocks using computed layout.
     *        Helper for CalculateLayout that populates m_displayList with RenderItem structs.
     * @param clientRect The client area dimensions (for word-wrap width calculation).
     */
    void BuildDisplayList(const RECT& clientRect);

    /**
     * @brief Renders a single block to the device context.
     *        Helper for Render that encapsulates per-block drawing logic.
     * @param hdc The device context (offscreen DC during double-buffering).
     * @param item The RenderItem to draw (contains type, bounds, content, attributes).
     * @note Uses internal m_scrollY for coordinate translation (no param needed).
     */
    void RenderBlock(HDC hdc, const RenderItem& item);

    /**
     * @brief Loads an image from URL and returns cached HBITMAP handle.
     *        Checks m_imageCache first (O(log n) map lookup). If miss, triggers
     *        asynchronous network fetch (PostMessage to UI thread with result).
     * @param src The image URL (from BLOCK_IMG attributes["src"]).
     * @return HBITMAP handle if cached, NULL if not yet loaded (displays alt text).
     * 
     * @note ASYNC DESIGN: Does NOT block on network I/O (returns NULL immediately).
     *       Background thread fetches image, posts WM_USER_IMAGE_LOADED message.
     *       On message receipt, cache bitmap and InvalidateRect to trigger repaint.
     */
    HBITMAP LoadImage(const std::string& src);

    /**
     * @brief Decodes image bytes (BMP/JPEG) into GDI-compatible HBITMAP.
     *        Uses Win98 GDI for BMP, libjpeg for JPEG (optional external lib).
     * @param imageData The raw image bytes (from network fetch).
     * @param dataSize The byte count.
     * @return HBITMAP handle on success, NULL on decode failure.
     */
    HBITMAP DecodeImage(const unsigned char* imageData, size_t dataSize);

    /**
     * @brief Selects appropriate cached font based on block type.
     *        Fast O(1) lookup from m_hFont* member variables.
     * @param blockType The type of block being rendered (H1, P, A, etc.).
     * @return HFONT handle (never NULL due to fallback to system font).
     */
    HFONT GetFontForBlockType(Parser::BlockType blockType);

    /**
     * @brief Calculates vertical spacing above a block based on its type.
     *        Different block types have different spacing multipliers.
     * @param blockType The type of block (H1, H2, P, etc.).
     * @return Spacing in pixels (BLOCK_SPACING_BASE * multiplier).
     */
    int GetBlockSpacing(Parser::BlockType blockType);

    /**
     * @brief Translates document coordinates to client coordinates.
     *        Helper for coordinate conversion during rendering and hit-testing.
     * @param documentY The Y position in document coordinates (absolute).
     * @return Client Y position (relative to window top).
     * @note Uses internal m_scrollY: clientY = documentY - m_scrollY
     */
    int DocumentToClientY(int documentY) const;

    /**
     * @brief Translates client coordinates to document coordinates.
     *        Helper for hit-testing (mouse clicks in client space).
     * @param clientY The Y position in client coordinates.
     * @return Document Y position (absolute).
     * @note Uses internal m_scrollY: documentY = clientY + m_scrollY
     */
    int ClientToDocumentY(int clientY) const;

    // ========================================================================
    // PRIVATE DATA STRUCTURES (Internal Representation)
    // ========================================================================

    /**
     * @struct RenderItem
     * @brief A single item in the display list, representing a laid-out block
     *        with computed position and size. This is the output of CalculateLayout
     *        and the input to Render. Optimized for cache locality and fast iteration.
     * 
     * DESIGN: Flat POD-like struct (no pointers, no virtuals) for vector storage.
     *         Pre-computed bounds eliminate need for layout recalculation during paint.
     */
    struct RenderItem
    {
        /// @brief The type of block (determines rendering method).
        Parser::BlockType type;

        /// @brief The bounding rectangle in DOCUMENT coordinates (absolute Y position).
        ///        During rendering, translated to client coordinates via: clientY = bounds.top - scrollY
        RECT bounds;

        /// @brief The text content to display (from HtmlBlock.content).
        ///        For BLOCK_TEXT/P/H1/H2/H3: Full text string (possibly multi-line after word-wrap)
        ///        For BLOCK_A: Link text (displayed in blue with underline)
        ///        For BLOCK_IMG: Alt text (displayed if image unavailable)
        std::string content;

        /// @brief HTML attributes from HtmlBlock.attributes (href, src, alt, etc.).
        ///        Used during rendering to fetch images (src), handle clicks (href), etc.
        std::map<std::string, std::string> attributes;

        /**
         * @brief Default constructor - safe uninitialized state.
         */
        RenderItem() : type(Parser::BLOCK_UNKNOWN)
        {
            bounds.left = bounds.top = bounds.right = bounds.bottom = 0;
        }
    };

    /**
     * @struct ClickableArea
     * @brief A clickable region (hyperlink) with associated URL.
     *        Populated during layout, used during hit-testing to map clicks to URLs.
     * 
     * DESIGN: Separate from RenderItem to allow efficient spatial indexing in future
     *         (e.g., R-tree for O(log n) hit-testing instead of O(n) linear search).
     */
    struct ClickableArea
    {
        /// @brief The bounding rectangle in DOCUMENT coordinates (absolute Y position).
        ///        Must translate click coordinates to document space for comparison.
        RECT rect;

        /// @brief The target URL (from BLOCK_A attributes["href"]).
        ///        Returned by HandleClick if click lands inside rect.
        std::string href;

        /**
         * @brief Default constructor - empty clickable area.
         */
        ClickableArea()
        {
            rect.left = rect.top = rect.right = rect.bottom = 0;
        }
    };

    // ========================================================================
    // PRIVATE MEMBER VARIABLES (Internal State)
    // ========================================================================

    // --- CONTENT STATE ---
    
    /// @brief The current HTML blocks loaded into the renderer (from Parser output).
    ///        Populated by SetContent, cleared on new page load.
    std::vector<Parser::HtmlBlock> m_blocks;

    /// @brief The computed display list (output of CalculateLayout, input to Render).
    ///        Each RenderItem has pre-computed bounds (position + size) for fast painting.
    ///        Cleared when layout dirty, rebuilt by CalculateLayout.
    std::vector<RenderItem> m_displayList;

    /// @brief The clickable regions (hyperlinks) extracted during layout.
    ///        Used by HandleClick to map mouse coordinates to URLs.
    ///        Cleared when layout dirty, rebuilt by CalculateLayout.
    std::vector<ClickableArea> m_clickableAreas;

    // --- INITIALIZATION STATE ---

    /// @brief Flag indicating renderer has been successfully initialized.
    ///        Set to false by constructor, set to true by Initialize() on success.
    ///        Checked by all public methods (except Initialize) to ensure proper setup.
    ///        If false, methods return RENDER_NOT_INITIALIZED error.
    bool m_initialized;

    // --- LAYOUT STATE ---

    /// @brief Flag indicating layout needs recalculation (content or size changed).
    ///        Set to true by SetContent and OnResize.
    ///        Set to false by CalculateLayout after successful computation.
    bool m_layoutDirty;

    /// @brief The total height of all content (sum of all block heights + spacing).
    ///        Computed by CalculateLayout, used for scrollbar range and max scroll.
    int m_totalContentHeight;

    /// @brief The cached client area dimensions from last CalculateLayout.
    ///        Used to detect resize events (if new rect != cached rect, relayout needed).
    RECT m_cachedClientRect;

    // --- SCROLL STATE ---

    /// @brief The current vertical scroll position (in pixels from document top).
    ///        Range: [0, max(0, m_totalContentHeight - clientHeight)]
    ///        Modified by OnScroll, used by Render for coordinate translation.
    int m_scrollY;

    // --- GDI RESOURCE CACHE (Expensive to create, reused across paints) ---

    /// @brief Cached font for <h1> headings (large, bold).
    ///        Created in constructor, deleted in destructor.
    HFONT m_hFontH1;

    /// @brief Cached font for <h2> headings (medium, bold).
    HFONT m_hFontH2;

    /// @brief Cached font for <h3> headings (slightly large, bold).
    HFONT m_hFontH3;

    /// @brief Cached font for <p> and default text (normal size, regular weight).
    HFONT m_hFontDefault;

    /// @brief Cached font for <a> hyperlinks (normal size, underlined).
    HFONT m_hFontLink;

    /// @brief Offscreen device context for double-buffering.
    ///        Created on first Render, deleted on resize or destruction.
    HDC m_memDC;

    /// @brief Offscreen bitmap for double-buffering (same dimensions as client area).
    ///        Created on first Render or resize, deleted on resize or destruction.
    HBITMAP m_offscreenBmp;

    /// @brief Previous bitmap selected into m_memDC (for restoration on cleanup).
    ///        Saved during SelectObject, restored before DeleteDC.
    HBITMAP m_hOldBitmap;

    /// @brief Brush for clearing background (white).
    ///        Created in constructor, deleted in destructor.
    HBRUSH m_hBackgroundBrush;

    /// @brief Pen for drawing link underlines (blue, 1px solid).
    ///        Created in constructor, deleted in destructor.
    HPEN m_hLinkPen;

    // --- IMAGE CACHE (LRU eviction when MAX_IMAGE_CACHE_SIZE exceeded) ---

    /// @brief Cache of downloaded/decoded images (URL -> HBITMAP).
    ///        Populated by LoadImage on demand, evicted when cache full.
    ///        Key: Image URL (src attribute), Value: GDI bitmap handle.
    std::map<std::string, HBITMAP> m_imageCache;

    /// @brief LRU (Least Recently Used) access order tracking for image cache eviction.
    ///        STRUCTURE: Vector ordered by recency of access
    ///          - Front (index 0): MRU (Most Recently Used) - last accessed/loaded image
    ///          - Back (index size-1): LRU (Least Recently Used) - oldest unused image
    ///        OPERATIONS:
    ///          - On access: Move URL to front (erase from current position, push_front)
    ///          - On eviction: Remove back entry (pop_back), then DeleteObject cached bitmap
    ///        INVARIANT: Every URL in m_imageCache MUST exist in this vector (1:1 mapping)
    ///        COMPLEXITY: O(n) for move-to-front (linear search + erase), acceptable for n<50
    ///        ALTERNATIVE: Use std::list for O(1) move, but requires iterator storage in map
    std::vector<std::string> m_imageCacheLRU;

}; // class HtmlRenderer

} // namespace Renderer

#endif // RENDERER_H

// ============================================================================
// END OF HEADER
// ============================================================================
// IMPLEMENTATION NOTES (for renderer.cpp development):
//
// FONT CREATION EXAMPLE:
//   m_hFontH1 = CreateFont(
//       -MulDiv(FONT_SIZE_H1, GetDeviceCaps(hdc, LOGPIXELSY), 72), // Height in pixels
//       0,                          // Width (0 = auto)
//       0,                          // Escapement (rotation)
//       0,                          // Orientation
//       FW_BOLD,                    // Weight (bold)
//       FALSE,                      // Italic
//       FALSE,                      // Underline
//       FALSE,                      // Strikeout
//       DEFAULT_CHARSET,            // Charset
//       OUT_DEFAULT_PRECIS,         // Output precision
//       CLIP_DEFAULT_PRECIS,        // Clipping precision
//       DEFAULT_QUALITY,            // Quality (no antialiasing on Win98)
//       DEFAULT_PITCH | FF_DONTCARE,// Pitch and family
//       "Arial"                     // Font name (fallback to system if unavailable)
//   );
//   if (!m_hFontH1) {
//       m_hFontH1 = (HFONT)GetStockObject(SYSTEM_FONT); // Fallback
//   }
//
// DOUBLE-BUFFERING SETUP:
//   HDC hdcScreen = GetDC(hwnd);
//   m_memDC = CreateCompatibleDC(hdcScreen);
//   m_offscreenBmp = CreateCompatibleBitmap(hdcScreen, width, height);
//   m_hOldBitmap = (HBITMAP)SelectObject(m_memDC, m_offscreenBmp);
//   ReleaseDC(hwnd, hdcScreen);
//
// WORD-WRAPPING EXAMPLE:
//   RECT textRect = {MARGIN_LEFT, currentY, clientRect.right - MARGIN_RIGHT, currentY + 1000};
//   SelectObject(hdc, m_hFontDefault);
//   int height = DrawText(hdc, block.content.c_str(), -1, &textRect, 
//                         DT_WORDBREAK | DT_CALCRECT); // Measures only, doesn't draw
//   // textRect.bottom now contains calculated height after wrapping
//
// IMAGE LOADING (ASYNC PATTERN):
//   In LoadImage():
//     if (m_imageCache.find(src) != m_imageCache.end()) return m_imageCache[src];
//     PostMessage(hwnd, WM_USER_IMAGE_LOAD_REQUEST, 0, (LPARAM)src.c_str());
//     return NULL; // Return immediately, don't block
//   In WndProc:
//     case WM_USER_IMAGE_LOADED: {
//       char* url = (char*)lParam;
//       HBITMAP hBmp = ...; // Received from background thread
//       g_renderer.CacheImage(url, hBmp);
//       InvalidateRect(hwnd, NULL, TRUE); // Repaint to show new image
//     }
//
// HIT-TESTING EXAMPLE:
//   int docY = clientY + m_scrollY;
//   for (size_t i = 0; i < m_clickableAreas.size(); ++i) {
//       const ClickableArea& area = m_clickableAreas[i];
//       if (x >= area.rect.left && x < area.rect.right &&
//           docY >= area.rect.top && docY < area.rect.bottom) {
//           outHref = area.href;
//           return true;
//       }
//   }
//   return false;
//
// CRITICAL CLEANUP (Destructor):
//   if (m_hOldBitmap) SelectObject(m_memDC, m_hOldBitmap); // Restore original
//   if (m_offscreenBmp) DeleteObject(m_offscreenBmp);
//   if (m_memDC) DeleteDC(m_memDC);
//   if (m_hFontH1) DeleteObject(m_hFontH1);
//   // ... delete all other GDI objects ...
//   for (map::iterator it = m_imageCache.begin(); it != m_imageCache.end(); ++it) {
//       if (it->second) DeleteObject(it->second);
//   }
//
// INTEGRATION CHECKLIST:
//   [ ] Compiles with VC++ 6.0 (no C++11 features, no GDI+)
//   [ ] Links against gdi32.lib (via #pragma comment in stdafx.h)
//   [ ] Consumes Parser::HtmlBlock directly (tight coupling by design)
//   [ ] WM_CREATE calls Initialize(hwnd) and checks result before proceeding
//   [ ] UI layer calls SetContent -> CalculateLayout -> Render in sequence
//   [ ] Render() uses internal m_scrollY (no scroll param passed)
//   [ ] Scrollbar range updated after CalculateLayout via GetTotalContentHeight
//   [ ] Hit-testing integrated with WM_LBUTTONDOWN for navigation
//   [ ] Async image loading uses NotifyImageLoaded() instead of WM_USER messages
//   [ ] Unit tests pass (see TESTING RECOMMENDATIONS section)
//   [ ] Memory leaks checked via GetGuiResources (before/after counts match)
//   [ ] Performance verified: <200ms layout, <50ms render on Win98 VM
//   [ ] Double-buffering eliminates flicker during scroll
// ============================================================================