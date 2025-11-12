// ============================================================================
// renderer.h - Public Interface for the HTML Renderer Module
// ============================================================================
// MODULE PURPOSE:
// GDI-based rendering engine that transforms parsed HTML blocks into visible
// content on Win98. Uses two-phase architecture: Layout (expensive, on resize)
// and Paint (fast, on every frame). Manages fonts, images, scrolling, and clicks.
//
// KEY FEATURES:
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
// For detailed architecture documentation, see docs/renderer_architecture.md
// ============================================================================

#pragma once
#ifndef RENDERER_H
#define RENDERER_H

#include "stdafx.h"
#include "../parser/parser.h"

namespace Renderer
{

// Configuration Constants
const int MAX_FONT_CACHE_SIZE = 20;
const int MAX_IMAGE_CACHE_SIZE = 50;
const int MARGIN_LEFT = 10;
const int MARGIN_RIGHT = 10;
const int MARGIN_TOP = 10;
const float LINE_HEIGHT_MULTIPLIER = 1.5f;
const int BLOCK_SPACING_BASE = 5; // Reduced from 10 for more compact layout
const int BR_SPACING = 5;
const int IMAGE_PLACEHOLDER_WIDTH = 150;
const int IMAGE_PLACEHOLDER_HEIGHT = 100;
const int SCROLL_LINE_SIZE = 20;
const int SCROLL_PAGE_SIZE = 200;

// Color Constants
const COLORREF LINK_COLOR = RGB(0, 0, 255);
const COLORREF VISITED_LINK_COLOR = RGB(85, 26, 139);
const COLORREF TEXT_COLOR = RGB(0, 0, 0);
const COLORREF BACKGROUND_COLOR = RGB(255, 255, 255);

// Font Size Constants (in points)
const int FONT_SIZE_H1 = 24;
const int FONT_SIZE_H2 = 18;
const int FONT_SIZE_H3 = 14;
const int FONT_SIZE_DEFAULT = 12;

// ============================================================================
// RENDER STATUS AND RESULTS
// ============================================================================

enum RenderStatus
{
    RENDER_SUCCESS,
    RENDER_EMPTY,
    RENDER_INVALID_DC,
    RENDER_NOT_INITIALIZED,
    RENDER_FAILED,
    RENDER_UNKNOWN_ERROR
};

struct RenderResult
{
    RenderStatus status;
    std::string errorMessage;
    std::vector<std::string> warnings;
    
    RenderResult()
        : status(RENDER_UNKNOWN_ERROR),
          errorMessage("Uninitialized RenderResult")
    {
    }
    
    explicit RenderResult(RenderStatus s)
        : status(s),
          errorMessage(s == RENDER_SUCCESS || s == RENDER_EMPTY ? "" : "Unspecified error")
    {
    }
};

// ============================================================================
// RENDER ITEM - Display List Element
// ============================================================================

struct RenderItem
{
    Parser::BlockType type;
    RECT bounds;
    std::string content;
    std::map<std::string, std::string> attributes;
    
    // CSS styling properties (parsed from HtmlBlock)
    int textColor;          // -1 = default, else COLORREF
    int backgroundColor;    // -1 = default, else COLORREF
    int fontWeight;         // FW_NORMAL or FW_BOLD
    BOOL fontItalic;        // TRUE/FALSE
    int fontSize;           // 0 = default, else pixel size
    
    RenderItem()
        : type(Parser::BLOCK_UNKNOWN)
        , textColor(-1)
        , backgroundColor(-1)
        , fontWeight(FW_NORMAL)
        , fontItalic(FALSE)
        , fontSize(0)
    {
        bounds.left = bounds.top = bounds.right = bounds.bottom = 0;
    }
};

// ============================================================================
// CLICKABLE AREA - Link Hit Testing
// ============================================================================

struct ClickableArea
{
    RECT bounds;
    std::string href;
};

// ============================================================================
// HTML RENDERER CLASS
// ============================================================================

class HtmlRenderer
{
public:
    // Lifecycle
    HtmlRenderer();
    ~HtmlRenderer();
    RenderResult Initialize(HWND hwnd);
    
    // Content Management
    void SetContent(const std::vector<Parser::HtmlBlock>& blocks);
    void CalculateLayout(HWND hwnd, const RECT& clientRect);
    
    // Rendering
    RenderResult Render(HWND hwnd, HDC hdc, const RECT& visibleRect);
    
    // Interaction
    int GetTotalContentHeight() const;
    bool HandleClick(int x, int y, std::string& outHref);
    void OnScroll(HWND hwnd, int scrollType, int scrollPos);
    void OnResize(HWND hwnd);
    void OnScroll(int amount);
    
    // Image Management
    void NotifyImageLoaded(const std::string& src, HBITMAP hBitmap);

private:
    // Non-copyable
    HtmlRenderer(const HtmlRenderer&);
    HtmlRenderer& operator=(const HtmlRenderer&);
    
    // Helper Methods
    void ReleaseGdiResources();
    void RenderBlock(HDC hdc, const RenderItem& item);
    HBITMAP LoadImage(const std::string& src);
    HFONT GetFontForBlockType(Parser::BlockType blockType);
    int GetBlockSpacing(Parser::BlockType blockType);
    int DocumentToClientY(int documentY) const;
    int ClientToDocumentY(int clientY) const;
    HBITMAP DecodeImage(const unsigned char* imageData, size_t dataSize);
    
    // State
    bool m_initialized;
    bool m_layoutDirty;
    int m_totalContentHeight;
    int m_scrollY;
    RECT m_cachedClientRect;
    
    // Page-level CSS from <body> tag
    COLORREF m_pageBackgroundColor;  // From <body bgcolor> or default
    COLORREF m_pageTextColor;        // From <body text> or default
    
    // Content
    std::vector<Parser::HtmlBlock> m_blocks;
    std::vector<RenderItem> m_displayList;
    std::vector<ClickableArea> m_clickableAreas;
    
    // GDI Resources
    HFONT m_hFontH1;
    HFONT m_hFontH2;
    HFONT m_hFontH3;
    HFONT m_hFontDefault;
    HFONT m_hFontLink;
    HDC m_memDC;
    HBITMAP m_offscreenBmp;
    HBITMAP m_hOldBitmap;
    HBRUSH m_hBackgroundBrush;
    HPEN m_hLinkPen;
    
    // Image Cache
    std::map<std::string, HBITMAP> m_imageCache;
    std::vector<std::string> m_imageCacheLRU;
};

} // namespace Renderer

#endif // RENDERER_H
