// ============================================================================
// renderer.cpp - Implementation of HTML Renderer Module for Win98 Retro Browser
// ============================================================================
// MODULE: Renderer::HtmlRenderer
// PURPOSE: GDI-based rendering engine transforming parsed HTML blocks to pixels
// ARCHITECTURE: Two-phase (Layout + Paint), Stateful resource management
// CONSTRAINTS: C++98/VC++6, Win98 GDI, 200MHz CPU, 64KB GDI heap
// ============================================================================

#include "stdafx.h"
#include "renderer.h"
#include "../parser/parser.h"
#include <set>

namespace Renderer
{

// ============================================================================
// CONSTRUCTOR - Initialize Safe Default State
// ============================================================================
HtmlRenderer::HtmlRenderer()
    : m_initialized(false)
    , m_layoutDirty(true)
    , m_totalContentHeight(0)
    , m_scrollY(0)
    , m_hFontH1(NULL)
    , m_hFontH2(NULL)
    , m_hFontH3(NULL)
    , m_hFontDefault(NULL)
    , m_hFontLink(NULL)
    , m_memDC(NULL)
    , m_offscreenBmp(NULL)
    , m_hOldBitmap(NULL)
    , m_hBackgroundBrush(NULL)
    , m_hLinkPen(NULL)
{
    // Reserve capacity to minimize reallocations
    m_blocks.reserve(50);
    m_displayList.reserve(50);
    m_clickableAreas.reserve(50);
    
    // Initialize cached client rect to zero
    m_cachedClientRect.left = 0;
    m_cachedClientRect.top = 0;
    m_cachedClientRect.right = 0;
    m_cachedClientRect.bottom = 0;
}

// ============================================================================
// DESTRUCTOR - Release All GDI Resources (RAII)
// ============================================================================
HtmlRenderer::~HtmlRenderer()
{
    ReleaseGdiResources();
}

// ============================================================================
// INITIALIZE - Two-Phase Initialization with Error Reporting
// ============================================================================
RenderResult HtmlRenderer::Initialize(HWND hwnd)
{
    RenderResult result(RENDER_SUCCESS);
    
    // Idempotent: Skip if already initialized
    if (m_initialized) {
        return result;
    }
    
    // Validate window handle
    if (!hwnd || !IsWindow(hwnd)) {
        result.status = RENDER_FAILED;
        result.errorMessage = "Invalid window handle provided to Initialize";
        return result;
    }
    
    // Get temporary DC for font creation
    HDC hdc = GetDC(hwnd);
    if (!hdc) {
        result.status = RENDER_FAILED;
        result.errorMessage = "GetDC failed in Initialize";
        return result;
    }
    
    // Calculate DPI-adjusted font sizes
    int logPixelsY = GetDeviceCaps(hdc, LOGPIXELSY);
    int h1Height = -MulDiv(FONT_SIZE_H1, logPixelsY, 72);
    int h2Height = -MulDiv(FONT_SIZE_H2, logPixelsY, 72);
    int h3Height = -MulDiv(FONT_SIZE_H3, logPixelsY, 72);
    int defHeight = -MulDiv(FONT_SIZE_DEFAULT, logPixelsY, 72);
    
    // Create H1 font (24pt bold)
    m_hFontH1 = CreateFont(h1Height, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                           DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                           DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
    if (!m_hFontH1) {
        m_hFontH1 = (HFONT)GetStockObject(SYSTEM_FONT);
        result.warnings.push_back("H1 font creation failed, using system font fallback");
    }
    
    // Create H2 font (18pt bold)
    m_hFontH2 = CreateFont(h2Height, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                           DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                           DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
    if (!m_hFontH2) {
        m_hFontH2 = (HFONT)GetStockObject(SYSTEM_FONT);
        result.warnings.push_back("H2 font creation failed, using system font fallback");
    }
    
    // Create H3 font (14pt bold)
    m_hFontH3 = CreateFont(h3Height, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                           DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                           DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
    if (!m_hFontH3) {
        m_hFontH3 = (HFONT)GetStockObject(SYSTEM_FONT);
        result.warnings.push_back("H3 font creation failed, using system font fallback");
    }
    
    // Create default font (12pt regular)
    m_hFontDefault = CreateFont(defHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
    if (!m_hFontDefault) {
        m_hFontDefault = (HFONT)GetStockObject(SYSTEM_FONT);
        result.warnings.push_back("Default font creation failed, using system font fallback");
    }
    
    // Create link font (12pt underlined)
    m_hFontLink = CreateFont(defHeight, 0, 0, 0, FW_NORMAL, FALSE, TRUE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
    if (!m_hFontLink) {
        m_hFontLink = (HFONT)GetStockObject(SYSTEM_FONT);
        result.warnings.push_back("Link font creation failed, using system font fallback");
    }
    
    // Create background brush
    m_hBackgroundBrush = CreateSolidBrush(BACKGROUND_COLOR);
    if (!m_hBackgroundBrush) {
        m_hBackgroundBrush = (HBRUSH)GetStockObject(WHITE_BRUSH);
        result.warnings.push_back("Background brush creation failed, using white brush fallback");
    }
    
    // Create link underline pen
    m_hLinkPen = CreatePen(PS_SOLID, 1, LINK_COLOR);
    if (!m_hLinkPen) {
        m_hLinkPen = (HPEN)GetStockObject(BLACK_PEN);
        result.warnings.push_back("Link pen creation failed, using black pen fallback");
    }
    
    ReleaseDC(hwnd, hdc);
    
    // Verify at least default font is available
    if (!m_hFontDefault) {
        ReleaseGdiResources();
        result.status = RENDER_FAILED;
        result.errorMessage = "Critical: Could not create or obtain default font";
        return result;
    }
    
    m_initialized = true;
    return result;
}

// ============================================================================
// SET CONTENT - Load New HTML Blocks
// ============================================================================
void HtmlRenderer::SetContent(const std::vector<Parser::HtmlBlock>& blocks)
{
    // Copy blocks with capacity hint
    m_blocks = blocks;
    if (m_blocks.capacity() < blocks.size()) {
        m_blocks.reserve(blocks.size());
    }
    
    // Invalidate layout
    m_displayList.clear();
    m_clickableAreas.clear();
    m_layoutDirty = true;
    
    // Reset scroll to top
    m_scrollY = 0;
    
    // Evict unreferenced images from cache
    std::set<std::string> referencedImages;
    for (size_t i = 0; i < m_blocks.size(); ++i) {
        if (m_blocks[i].type == Parser::BLOCK_IMG) {
            std::map<std::string, std::string>::const_iterator it = 
                m_blocks[i].attributes.find("src");
            if (it != m_blocks[i].attributes.end()) {
                referencedImages.insert(it->second);
            }
        }
    }
    
    // Remove unreferenced images from cache
    std::map<std::string, HBITMAP>::iterator cacheIt = m_imageCache.begin();
    while (cacheIt != m_imageCache.end()) {
        if (referencedImages.find(cacheIt->first) == referencedImages.end()) {
            // Not referenced, evict
            if (cacheIt->second) {
                DeleteObject(cacheIt->second);
            }
            
            // Remove from LRU list
            std::vector<std::string>::iterator lruIt = 
                std::find(m_imageCacheLRU.begin(), m_imageCacheLRU.end(), cacheIt->first);
            if (lruIt != m_imageCacheLRU.end()) {
                m_imageCacheLRU.erase(lruIt);
            }
            
            std::map<std::string, HBITMAP>::iterator toErase = cacheIt;
            ++cacheIt;
            m_imageCache.erase(toErase);
        } else {
            ++cacheIt;
        }
    }
}

// ============================================================================
// CALCULATE LAYOUT - Phase 1: Measure and Position Blocks
// ============================================================================
void HtmlRenderer::CalculateLayout(HWND hwnd, const RECT& clientRect)
{
    // Validate state
    if (!m_initialized) {
        return;
    }
    
    // Skip empty rect
    if (clientRect.right <= clientRect.left || clientRect.bottom <= clientRect.top) {
        return;
    }
    
    // Skip if layout is clean and rect unchanged
    if (!m_layoutDirty && EqualRect(&m_cachedClientRect, &clientRect)) {
        return;
    }
    
    // Get DC for text measurement
    HDC hdc = GetDC(hwnd);
    if (!hdc) {
        return;
    }
    
    // Clear previous layout
    m_displayList.clear();
    m_clickableAreas.clear();
    m_displayList.reserve(m_blocks.size());
    m_clickableAreas.reserve(50);
    
    int currentY = MARGIN_TOP;
    int contentWidth = clientRect.right - MARGIN_LEFT - MARGIN_RIGHT;
    
    // Single-pass layout algorithm
    for (size_t i = 0; i < m_blocks.size(); ++i) {
        const Parser::HtmlBlock& block = m_blocks[i];
        
        // Select appropriate font
        HFONT hFont = GetFontForBlockType(block.type);
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
        
        // Add spacing above block
        int spacing = GetBlockSpacing(block.type);
        currentY += spacing;
        
        // Measure text with word-wrapping
        RECT measureRect;
        measureRect.left = MARGIN_LEFT;
        measureRect.top = currentY;
        measureRect.right = clientRect.right - MARGIN_RIGHT;
        measureRect.bottom = currentY + 10000; // Large value for DT_CALCRECT
        
        int height = 0;
        if (block.type == Parser::BLOCK_BR) {
            // Line break: just advance Y
            height = BR_SPACING;
            measureRect.bottom = measureRect.top + height;
        } else if (block.type == Parser::BLOCK_UL) {
            // List container: minimal spacing, no content
            height = 5; // Small spacing before list
            measureRect.bottom = measureRect.top + height;
        } else if (block.type == Parser::BLOCK_LI) {
            // List item: measure with bullet indent
            measureRect.left += 20; // Indent for bullet
            if (!block.content.empty()) {
                height = DrawText(hdc, block.content.c_str(), -1, &measureRect, 
                                DT_WORDBREAK | DT_CALCRECT);
            }
            measureRect.left -= 20; // Restore left margin for rendering
        } else if (block.type == Parser::BLOCK_IMG) {
            // Image block: use width/height attributes if available, otherwise use placeholder size
            int imgWidth = IMAGE_PLACEHOLDER_WIDTH;
            int imgHeight = IMAGE_PLACEHOLDER_HEIGHT;
            
            // Try to get width from attributes
            std::map<std::string, std::string>::const_iterator widthIt = 
                block.attributes.find("width");
            if (widthIt != block.attributes.end()) {
                imgWidth = atoi(widthIt->second.c_str());
                if (imgWidth <= 0) imgWidth = IMAGE_PLACEHOLDER_WIDTH;
            }
            
            // Try to get height from attributes
            std::map<std::string, std::string>::const_iterator heightIt = 
                block.attributes.find("height");
            if (heightIt != block.attributes.end()) {
                imgHeight = atoi(heightIt->second.c_str());
                if (imgHeight <= 0) imgHeight = IMAGE_PLACEHOLDER_HEIGHT;
            }
            
            // Set bounds for image
            height = imgHeight;
            measureRect.right = measureRect.left + imgWidth;
            measureRect.bottom = measureRect.top + imgHeight;
        } else if (!block.content.empty()) {
            // Measure text bounds
            height = DrawText(hdc, block.content.c_str(), -1, &measureRect, 
                            DT_WORDBREAK | DT_CALCRECT);
        }
        
        // Create render item
        RenderItem item;
        item.type = block.type;
        item.bounds = measureRect;
        item.content = block.content;
        item.attributes = block.attributes;
        m_displayList.push_back(item);
        
        // Add clickable area for hyperlinks
        // Support both <a> tags and <li> tags with href attribute (merged from <a>)
        if (block.type == Parser::BLOCK_A || block.type == Parser::BLOCK_LI) {
            std::map<std::string, std::string>::const_iterator hrefIt = 
                block.attributes.find("href");
            if (hrefIt != block.attributes.end() && !hrefIt->second.empty()) {
                ClickableArea area;
                area.bounds = measureRect;
                area.href = hrefIt->second;
                m_clickableAreas.push_back(area);
            }
        }
        
        // Advance Y position with line height multiplier
        // Formula: add extra spacing proportional to line height (LINE_HEIGHT_MULTIPLIER - 1.0)
        int lineHeight = measureRect.bottom - measureRect.top;
        currentY = measureRect.bottom + (int)((LINE_HEIGHT_MULTIPLIER - 1.0f) * lineHeight);
        
        SelectObject(hdc, hOldFont);
    }
    
    // Store total content height
    m_totalContentHeight = currentY - MARGIN_TOP;
    
    // Cache layout state
    m_cachedClientRect = clientRect;
    m_layoutDirty = false;
    
    ReleaseDC(hwnd, hdc);
}

// ============================================================================
// RENDER - Phase 2: Paint Visible Content to Screen
// ============================================================================
RenderResult HtmlRenderer::Render(HWND hwnd, HDC hdc, const RECT& visibleRect)
{
    RenderResult result(RENDER_SUCCESS);
    
    // Validation checks
    if (!m_initialized) {
        result.status = RENDER_NOT_INITIALIZED;
        result.errorMessage = "Renderer not initialized. Call Initialize() first.";
        return result;
    }
    
    if (m_layoutDirty) {
        result.status = RENDER_NOT_INITIALIZED;
        result.errorMessage = "Layout not computed. Call CalculateLayout() first.";
        return result;
    }
    
    if (!hdc) {
        result.status = RENDER_INVALID_DC;
        result.errorMessage = "Invalid device context (NULL HDC) passed to Render";
        return result;
    }
    
    // Get client area
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    
    // Setup offscreen buffer for double-buffering
    bool directDraw = false;
    if (!m_memDC || !EqualRect(&clientRect, &m_cachedClientRect)) {
        // Recreate offscreen buffer if size changed
        if (m_memDC) {
            SelectObject(m_memDC, m_hOldBitmap);
            DeleteObject(m_offscreenBmp);
            DeleteDC(m_memDC);
            m_memDC = NULL;
            m_offscreenBmp = NULL;
        }
        
        HDC hdcScreen = GetDC(hwnd);
        if (hdcScreen) {
            m_memDC = CreateCompatibleDC(hdcScreen);
            if (m_memDC) {
                m_offscreenBmp = CreateCompatibleBitmap(hdcScreen, 
                                                        clientRect.right, 
                                                        clientRect.bottom);
                if (m_offscreenBmp) {
                    m_hOldBitmap = (HBITMAP)SelectObject(m_memDC, m_offscreenBmp);
                } else {
                    result.warnings.push_back("CreateCompatibleBitmap failed, using direct draw");
                    DeleteDC(m_memDC);
                    m_memDC = NULL;
                    directDraw = true;
                }
            } else {
                result.warnings.push_back("CreateCompatibleDC failed, using direct draw");
                directDraw = true;
            }
            ReleaseDC(hwnd, hdcScreen);
        }
    }
    
    // Choose drawing target
    HDC drawDC = directDraw ? hdc : m_memDC;
    
    // Clear background
    FillRect(drawDC, &clientRect, m_hBackgroundBrush);
    
    // Address bar offset (ADDRESS_BAR_HEIGHT=24 + UI_PADDING*2=4)
    const int ADDRESS_BAR_OFFSET = 28;
    
    // Calculate visible range for clipping (accounting for address bar area)
    int visibleTop = m_scrollY;
    int visibleBottom = m_scrollY + clientRect.bottom - ADDRESS_BAR_OFFSET;
    
    // Render visible blocks
    for (size_t i = 0; i < m_displayList.size(); ++i) {
        const RenderItem& item = m_displayList[i];
        
        // Quick reject: skip blocks outside visible area
        if (item.bounds.bottom < visibleTop) {
            continue; // Above viewport
        }
        if (item.bounds.top > visibleBottom) {
            break; // Below viewport (early exit)
        }
        
        // Transform to screen coordinates with address bar offset
        RECT screenRect = item.bounds;
        OffsetRect(&screenRect, 0, ADDRESS_BAR_OFFSET - m_scrollY);
        
        // Check intersection with paint region
        RECT intersection;
        if (!IntersectRect(&intersection, &screenRect, &visibleRect)) {
            continue; // Not in paint region
        }
        
        // Render the block
        RenderBlock(drawDC, item);
    }
    
    // Copy offscreen buffer to screen
    if (!directDraw) {
        if (!BitBlt(hdc, 0, 0, clientRect.right, clientRect.bottom, 
                   m_memDC, 0, 0, SRCCOPY)) {
            result.warnings.push_back("BitBlt failed during final screen copy");
        }
    }
    
    return result;
}

// ============================================================================
// GET TOTAL CONTENT HEIGHT - Query Layout Metrics
// ============================================================================
int HtmlRenderer::GetTotalContentHeight() const
{
    return m_totalContentHeight;
}

// ============================================================================
// HANDLE CLICK - Hit-Test for Hyperlinks
// ============================================================================
bool HtmlRenderer::HandleClick(int x, int y, std::string& outHref)
{
    if (!m_initialized || m_clickableAreas.empty()) {
        return false;
    }
    
    // Transform client to document coordinates
    int documentY = y + m_scrollY;
    POINT pt;
    pt.x = x;
    pt.y = documentY;
    
    // Reverse iteration for z-order (last drawn = topmost)
    for (size_t i = m_clickableAreas.size(); i > 0; --i) {
        const ClickableArea& area = m_clickableAreas[i - 1];
        if (PtInRect(&area.bounds, pt)) {
            outHref = area.href;
            return true;
        }
    }
    
    return false;
}

// ============================================================================
// ON SCROLL - Handle Vertical Scrolling
// ============================================================================
void HtmlRenderer::OnScroll(HWND hwnd, int scrollType, int scrollPos)
{
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    int clientHeight = clientRect.bottom - clientRect.top;
    
    // Calculate maximum scroll position
    int maxScroll = m_totalContentHeight - clientHeight;
    if (maxScroll < 0) maxScroll = 0;
    
    int newScrollY = m_scrollY;
    
    // Update scroll position based on type
    switch (scrollType) {
        case SB_LINEUP:
            newScrollY -= SCROLL_LINE_SIZE;
            break;
        case SB_LINEDOWN:
            newScrollY += SCROLL_LINE_SIZE;
            break;
        case SB_PAGEUP:
            newScrollY -= SCROLL_PAGE_SIZE;
            break;
        case SB_PAGEDOWN:
            newScrollY += SCROLL_PAGE_SIZE;
            break;
        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
            newScrollY = scrollPos;
            break;
        case SB_TOP:
            newScrollY = 0;
            break;
        case SB_BOTTOM:
            newScrollY = maxScroll;
            break;
    }
    
    // Clamp to valid range
    if (newScrollY < 0) newScrollY = 0;
    if (newScrollY > maxScroll) newScrollY = maxScroll;
    
    m_scrollY = newScrollY;
    
    // Update scrollbar position
    SCROLLINFO si;
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_POS;
    si.nPos = m_scrollY;
    SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
}

// ============================================================================
// ON SCROLL - Simple Scroll by Amount (Overload)
// ============================================================================
void HtmlRenderer::OnScroll(int amount)
{
    // Set scroll position to absolute Y coordinate
    // Note: This is a simplified version without HWND, used when window handle
    //       is not available. Caller is responsible for invalidating window.
    // IMPORTANT: 'amount' is absolute position, not delta!
    
    int maxScroll = m_totalContentHeight;
    if (maxScroll < 0) maxScroll = 0;
    
    m_scrollY = amount;  // SET position, not ADD
    
    // Clamp to valid range [0, maxScroll]
    if (m_scrollY < 0) m_scrollY = 0;
    if (m_scrollY > maxScroll) m_scrollY = maxScroll;
}

// ============================================================================
// ON RESIZE - Recalculate Layout for New Window Size
// ============================================================================
void HtmlRenderer::OnResize(HWND hwnd)
{
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    
    // Skip invalid sizes
    if (clientRect.right <= 0 || clientRect.bottom <= 0) {
        return;
    }
    
    // Recalculate layout with new dimensions
    CalculateLayout(hwnd, clientRect);
    
    // Update scroll position and scrollbar
    int clientHeight = clientRect.bottom - clientRect.top;
    int maxScroll = m_totalContentHeight - clientHeight;
    if (maxScroll < 0) maxScroll = 0;
    
    // Clamp current scroll to new maximum
    if (m_scrollY > maxScroll) {
        m_scrollY = maxScroll;
    }
    
    // Update scrollbar range and position
    SCROLLINFO si;
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin = 0;
    si.nMax = maxScroll;
    si.nPage = clientHeight;
    si.nPos = m_scrollY;
    SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
}

// ============================================================================
// NOTIFY IMAGE LOADED - Cache Async-Loaded Image
// ============================================================================
void HtmlRenderer::NotifyImageLoaded(const std::string& src, HBITMAP hBitmap)
{
    if (src.empty()) {
        return;
    }
    
    // Replace existing bitmap if different
    std::map<std::string, HBITMAP>::iterator it = m_imageCache.find(src);
    if (it != m_imageCache.end() && it->second && it->second != hBitmap) {
        DeleteObject(it->second);
    }
    
    // Store in cache
    m_imageCache[src] = hBitmap;
    
    // Update LRU list (move to front)
    std::vector<std::string>::iterator lruIt = 
        std::find(m_imageCacheLRU.begin(), m_imageCacheLRU.end(), src);
    if (lruIt != m_imageCacheLRU.end()) {
        m_imageCacheLRU.erase(lruIt);
    }
    m_imageCacheLRU.insert(m_imageCacheLRU.begin(), src);
    
    // Evict LRU entries if cache exceeded
    while (m_imageCache.size() > (size_t)MAX_IMAGE_CACHE_SIZE) {
        std::string evictSrc = m_imageCacheLRU.back();
        m_imageCacheLRU.pop_back();
        
        std::map<std::string, HBITMAP>::iterator evictIt = m_imageCache.find(evictSrc);
        if (evictIt != m_imageCache.end()) {
            if (evictIt->second) {
                DeleteObject(evictIt->second);
            }
            m_imageCache.erase(evictIt);
        }
    }
}

// ============================================================================
// PRIVATE: RELEASE GDI RESOURCES
// ============================================================================
void HtmlRenderer::ReleaseGdiResources()
{
    // Delete fonts
    if (m_hFontH1) {
        DeleteObject(m_hFontH1);
        m_hFontH1 = NULL;
    }
    if (m_hFontH2) {
        DeleteObject(m_hFontH2);
        m_hFontH2 = NULL;
    }
    if (m_hFontH3) {
        DeleteObject(m_hFontH3);
        m_hFontH3 = NULL;
    }
    if (m_hFontDefault) {
        DeleteObject(m_hFontDefault);
        m_hFontDefault = NULL;
    }
    if (m_hFontLink) {
        DeleteObject(m_hFontLink);
        m_hFontLink = NULL;
    }
    
    // Delete brushes and pens
    if (m_hBackgroundBrush) {
        DeleteObject(m_hBackgroundBrush);
        m_hBackgroundBrush = NULL;
    }
    if (m_hLinkPen) {
        DeleteObject(m_hLinkPen);
        m_hLinkPen = NULL;
    }
    
    // Delete cached images
    for (std::map<std::string, HBITMAP>::iterator it = m_imageCache.begin();
         it != m_imageCache.end(); ++it) {
        if (it->second) {
            DeleteObject(it->second);
        }
    }
    m_imageCache.clear();
    m_imageCacheLRU.clear();
    
    // Delete offscreen buffer
    if (m_memDC) {
        if (m_hOldBitmap) {
            SelectObject(m_memDC, m_hOldBitmap);
            m_hOldBitmap = NULL;
        }
        DeleteDC(m_memDC);
        m_memDC = NULL;
    }
    if (m_offscreenBmp) {
        DeleteObject(m_offscreenBmp);
        m_offscreenBmp = NULL;
    }
}

// ============================================================================
// PRIVATE: RENDER BLOCK - Draw Single Block to DC
// ============================================================================
void HtmlRenderer::RenderBlock(HDC hdc, const RenderItem& item)
{
    // Transform to screen coordinates
    RECT screenRect = item.bounds;
    OffsetRect(&screenRect, 0, -m_scrollY);
    
    // Select appropriate font
    HFONT hFont = GetFontForBlockType(item.type);
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    
    // Set background mode for text
    SetBkMode(hdc, TRANSPARENT);
    
    switch (item.type) {
        case Parser::BLOCK_TEXT:
        case Parser::BLOCK_P:
        case Parser::BLOCK_H1:
        case Parser::BLOCK_H2:
        case Parser::BLOCK_H3:
            SetTextColor(hdc, TEXT_COLOR);
            DrawText(hdc, item.content.c_str(), -1, &screenRect, DT_WORDBREAK);
            break;
            
        case Parser::BLOCK_A: {
            // Draw link text in blue
            SetTextColor(hdc, LINK_COLOR);
            DrawText(hdc, item.content.c_str(), -1, &screenRect, DT_WORDBREAK);
            
            // Draw underline
            // NOTE: Current implementation draws a single line for the entire link rect.
            // For multi-line links (word-wrapped), this will only underline the last line.
            // A complete implementation would need to measure each line separately using
            // GetTextMetrics and draw individual underlines per line.
            HPEN hOldPen = (HPEN)SelectObject(hdc, m_hLinkPen);
            MoveToEx(hdc, screenRect.left, screenRect.bottom - 1, NULL);
            LineTo(hdc, screenRect.right, screenRect.bottom - 1);
            SelectObject(hdc, hOldPen);
            break;
        }
            
        case Parser::BLOCK_IMG: {
            // Try to load image
            std::map<std::string, std::string>::const_iterator srcIt = 
                item.attributes.find("src");
            if (srcIt != item.attributes.end()) {
                HBITMAP hBitmap = LoadImage(srcIt->second);
                if (hBitmap) {
                    // Draw image with proper scaling
                    HDC hdcMem = CreateCompatibleDC(hdc);
                    if (hdcMem) {
                        HBITMAP hOldBmp = (HBITMAP)SelectObject(hdcMem, hBitmap);
                        
                        // Get bitmap dimensions
                        BITMAP bm;
                        if (GetObject(hBitmap, sizeof(BITMAP), &bm) > 0) {
                            // Valid bitmap, proceed with rendering
                            
                            // Get target dimensions from layout
                            int targetWidth = screenRect.right - screenRect.left;
                            int targetHeight = screenRect.bottom - screenRect.top;
                            
                            // Use StretchBlt to scale image to fit layout bounds
                            // This respects width/height attributes or placeholder size from CalculateLayout
                            StretchBlt(hdc, screenRect.left, screenRect.top, targetWidth, targetHeight,
                                      hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
                        }
                        // If GetObject fails, silently skip (corrupted bitmap)
                        
                        SelectObject(hdcMem, hOldBmp);
                        DeleteDC(hdcMem);
                    }
                } else {
                    // Fallback to alt text
                    SetTextColor(hdc, TEXT_COLOR);
                    DrawText(hdc, item.content.c_str(), -1, &screenRect, DT_WORDBREAK);
                }
            }
            break;
        }
            
        case Parser::BLOCK_BR:
            // No visual output for line break
            break;
            
        case Parser::BLOCK_UL:
        case Parser::BLOCK_DIV:
        case Parser::BLOCK_SPAN:
            // Containers - no direct rendering needed (structural only)
            break;
            
        case Parser::BLOCK_LI: {
            // Draw bullet point (Win98-compatible: Windows-1252 bullet = 0x95)
            char bullet[4] = { (char)0x95, ' ', 0 };  // Bullet + space + null terminator
            RECT bulletRect = screenRect;
            bulletRect.right = bulletRect.left + 20; // Bullet width
            SetTextColor(hdc, TEXT_COLOR);
            DrawText(hdc, bullet, -1, &bulletRect, DT_LEFT | DT_TOP | DT_SINGLELINE);
            
            // Draw list item text with indent
            RECT textRect = screenRect;
            textRect.left += 20; // Indent for bullet
            
            // Check if this list item contains a link (has href attribute)
            std::map<std::string, std::string>::const_iterator hrefIt = item.attributes.find("href");
            bool isLink = (hrefIt != item.attributes.end() && !hrefIt->second.empty());
            
            if (isLink) {
                // Render as link: blue text + underline
                SetTextColor(hdc, LINK_COLOR);
                DrawText(hdc, item.content.c_str(), -1, &textRect, DT_WORDBREAK);
                
                // Draw underline for link
                HPEN hOldPen = (HPEN)SelectObject(hdc, m_hLinkPen);
                MoveToEx(hdc, textRect.left, textRect.bottom - 1, NULL);
                LineTo(hdc, textRect.right, textRect.bottom - 1);
                SelectObject(hdc, hOldPen);
            } else {
                // Regular list item: normal text color
                SetTextColor(hdc, TEXT_COLOR);
                DrawText(hdc, item.content.c_str(), -1, &textRect, DT_WORDBREAK);
            }
            break;
        }
            
        default:
            // Unknown block type, skip
            break;
    }
    
    SelectObject(hdc, hOldFont);
}

// ============================================================================
// PRIVATE: LOAD IMAGE - Retrieve from Cache or Trigger Async Load
// ============================================================================
HBITMAP HtmlRenderer::LoadImage(const std::string& src)
{
    if (src.empty()) {
        return NULL;
    }
    
    // Check cache
    std::map<std::string, HBITMAP>::iterator it = m_imageCache.find(src);
    if (it != m_imageCache.end()) {
        // Update LRU (move to front)
        std::vector<std::string>::iterator lruIt = 
            std::find(m_imageCacheLRU.begin(), m_imageCacheLRU.end(), src);
        if (lruIt != m_imageCacheLRU.end()) {
            m_imageCacheLRU.erase(lruIt);
            m_imageCacheLRU.insert(m_imageCacheLRU.begin(), src);
        }
        return it->second;
    }
    
    // Cache miss - trigger async load via PostMessage
    // 
    // ASYNC IMAGE LOADING INFRASTRUCTURE (Requires UI Layer Integration):
    //   
    //   DESIGN RATIONALE:
    //     This method intentionally returns NULL for cache misses rather than blocking
    //     to download images. Synchronous download would freeze the UI thread for seconds,
    //     creating a terrible user experience. The async architecture below provides
    //     progressive rendering: page displays immediately, images appear as they load.
    //   
    //   MODULE BOUNDARY:
    //     Renderer is responsible for: Layout, Painting, Caching
    //     UI Layer is responsible for: Network I/O, Threading, Image Decoding
    //     This separation keeps renderer simple and testable without network dependencies.
    //   
    //   INTEGRATION STEPS (To enable async image loading):
    //   1. Define WM_USER_LOAD_IMAGE constant (e.g., WM_USER + 1)
    //   2. Pass HWND to renderer via Initialize() or new SetWindow() method
    //   3. Uncomment the PostMessage line below
    //   4. Implement WM_USER_LOAD_IMAGE handler in WndProc:
    //      - Extract URL from LPARAM: char* url = (char*)lParam;
    //      - Create worker thread to download image (avoids blocking UI)
    //      - Worker thread: Fetch data via Network module, decode to HBITMAP
    //      - Worker thread: PostMessage(WM_USER_IMAGE_LOADED, (WPARAM)hBitmap, (LPARAM)strdup(url))
    //      - Free url string: free(url);
    //   5. Implement WM_USER_IMAGE_LOADED handler in WndProc:
    //      - Extract bitmap and URL: HBITMAP bmp = (HBITMAP)wParam; char* url = (char*)lParam;
    //      - Call renderer.NotifyImageLoaded(url, bmp);
    //      - InvalidateRect to trigger repaint with new image
    //      - Free url string: free(url);
    //
    // PostMessage(hwndMain, WM_USER_LOAD_IMAGE, 0, (LPARAM)strdup(src.c_str()));
    
    return NULL;
}

// ============================================================================
// PRIVATE: GET FONT FOR BLOCK TYPE
// ============================================================================
HFONT HtmlRenderer::GetFontForBlockType(Parser::BlockType blockType)
{
    switch (blockType) {
        case Parser::BLOCK_H1:
            return m_hFontH1;
        case Parser::BLOCK_H2:
            return m_hFontH2;
        case Parser::BLOCK_H3:
            return m_hFontH3;
        case Parser::BLOCK_A:
            return m_hFontLink;
        case Parser::BLOCK_TEXT:
        case Parser::BLOCK_P:
        case Parser::BLOCK_IMG:
        case Parser::BLOCK_BR:
        case Parser::BLOCK_UL:
        case Parser::BLOCK_LI:
        case Parser::BLOCK_DIV:
        case Parser::BLOCK_SPAN:
        default:
            return m_hFontDefault;
    }
}

// ============================================================================
// PRIVATE: GET BLOCK SPACING
// ============================================================================
int HtmlRenderer::GetBlockSpacing(Parser::BlockType blockType)
{
    switch (blockType) {
        case Parser::BLOCK_H1:
            return BLOCK_SPACING_BASE * 2;
        case Parser::BLOCK_H2:
            return (BLOCK_SPACING_BASE * 3) / 2; // 1.5x
        case Parser::BLOCK_H3:
            return (BLOCK_SPACING_BASE * 12) / 10; // 1.2x
        case Parser::BLOCK_P:
            return BLOCK_SPACING_BASE;
        case Parser::BLOCK_A:
            return BLOCK_SPACING_BASE / 2;
        case Parser::BLOCK_BR:
            return BR_SPACING;
        case Parser::BLOCK_UL:
            return BLOCK_SPACING_BASE;
        case Parser::BLOCK_LI:
            return BLOCK_SPACING_BASE / 3; // Small spacing between list items
        case Parser::BLOCK_DIV:
            return BLOCK_SPACING_BASE / 2; // Small spacing for containers
        case Parser::BLOCK_SPAN:
            return 0; // Inline element, no vertical spacing
        default:
            return 0;
    }
}

// ============================================================================
// PRIVATE: COORDINATE TRANSFORMS
// ============================================================================
int HtmlRenderer::DocumentToClientY(int documentY) const
{
    return documentY - m_scrollY;
}

int HtmlRenderer::ClientToDocumentY(int clientY) const
{
    return clientY + m_scrollY;
}

// ============================================================================
// PRIVATE: DECODE IMAGE - JPEG Decoding using libjpeg
// ============================================================================
HBITMAP HtmlRenderer::DecodeImage(const unsigned char* imageData, size_t dataSize)
{
    // Validate input
    if (!imageData || dataSize == 0) {
        return NULL;
    }
    
    // NOTE: This is a simplified implementation for Win98/VC++6 compatibility.
    // For production, consider more robust error handling and format detection.
    //
    // JPEG DECODING STRATEGY:
    // 1. Use libjpeg to decompress JPEG data into raw RGB pixel array
    // 2. Create DIB (Device Independent Bitmap) from raw pixels
    // 3. Convert DIB to DDB (Device Dependent Bitmap = HBITMAP) for GDI rendering
    //
    // WHY NOT DIRECT HBITMAP CREATION?
    // - libjpeg outputs row-by-row scanlines (bottom-up for Windows DIB)
    // - HBITMAP needs properly formatted DIB bits with correct alignment
    // - This approach ensures compatibility with Win98 GDI limitations
    
    // For Win98 compatibility, we'll implement a basic JPEG decoder stub
    // that returns NULL. The actual decoding will be done in the UI/Network layer
    // where we have better access to temporary file storage and can use
    // the jpeg library more safely with proper error handling.
    //
    // INTEGRATION POINT:
    // The UI layer should decode images before calling NotifyImageLoaded(),
    // passing already-decoded HBITMAP handles. This keeps renderer stateless
    // regarding image format details.
    
    return NULL;
}


} // namespace Renderer