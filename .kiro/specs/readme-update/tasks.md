# Implementation Plan - README Update

## Task List

- [x] 1. Verify and update code statistics





  - Count actual lines in all source files
  - Update code statistics table with accurate counts
  - Verify total matches sum of individual modules
  - _Requirements: 6.1, 6.2, 6.3, 6.4, 6.5_

- [x] 2. Create CSS Styling documentation section





  - Document inline style attributes (color, background-color, font-weight, font-style, font-size)
  - Explain color format support (named colors, hex #RGB/#RRGGBB, rgb(r,g,b))
  - Document legacy HTML 3.2 attributes (BGCOLOR, TEXT, COLOR on BODY/FONT tags)
  - List CSS limitations (no external stylesheets, no CSS selectors)
  - Explain CSS property extraction into HtmlBlock structures
  - _Requirements: 2.1, 2.2, 2.3, 2.4, 2.5_

- [x] 3. Expand and update Image Handling documentation





  - Document BMP as primary supported format (native Win98)
  - Explain async multi-threaded image loading mechanism
  - Document GDI BitBlt rendering and LRU cache eviction
  - Describe placeholder rendering and alt text fallback
  - List image size limits and Win98 memory constraints
  - _Requirements: 1.1, 1.2, 1.3, 1.4, 1.5_

- [x] 4. Update HTML Parsing documentation





  - List all supported tags including semantic HTML5 (header, footer, nav, article, section, aside, main)
  - Explain table support (TABLE, TR, TD, TH, TBODY, THEAD, TFOOT mapped to DIV)
  - Document FSM architecture (11 states, single-pass O(n) parsing)
  - List supported HTML entities (&amp;, &lt;, &gt;, &quot;, numeric entities)
  - Describe forgiving parser with error recovery and warning system
  - _Requirements: 3.1, 3.2, 3.3, 3.4, 3.5_

- [x] 5. Optimize Project Structure section





  - Simplify structure tree to show only core directories (src/, libs/, deploy/, demo/)
  - Detail src/ subdirectories (browser/core, browser/ui, browser/network, browser/parser, browser/renderer, browser/res, proxy/)
  - Remove .bat files and documentation files from main structure tree
  - Include resource directory with all files (app.ico, app.rc, hand.cur, resource.h)
  - Group docs/ and tools/ separately from core source structure
  - _Requirements: 4.1, 4.2, 4.3, 4.4, 4.5_

- [x] 6. Expand Rendering Engine documentation





  - Explain two-phase approach (Layout calculation + Paint)
  - Document double-buffering with offscreen DC for flicker-free rendering
  - Describe font caching for H1/H2/H3/P/A with DPI-aware sizing
  - Document vertical scrolling support with scrollbar integration
  - Explain hit-testing for hyperlinks with clickable areas tracking
  - _Requirements: 5.1, 5.2, 5.3, 5.4, 5.5_

- [x] 7. Update Feature Highlights section





  - Add CSS inline styling support to core features
  - Add semantic HTML5 tags support to parsing features
  - Add custom text/background colors per block to rendering features
  - Document async loading, caching, and placeholder rendering for images
  - Add CSS color parsing, entity decoding, and two-phase rendering to technical highlights
  - _Requirements: 7.1, 7.2, 7.3, 7.4, 7.5_

- [x] 8. Reorganize Features section with categories





  - Create HTML Support category (tags, semantic HTML5, tables, entity decoding)
  - Create CSS Styling category (inline styles, color formats, legacy attributes)
  - Create Image Handling category (async loading, caching, format support)
  - Create Rendering Engine category (two-phase architecture, double-buffering, font management)
  - Create Network category (HTTP/1.0, proxy integration, error handling)
  - _Requirements: 8.1, 8.2, 8.3, 8.4, 8.5_

- [x] 9. Final review and validation




  - Verify all requirements are addressed
  - Check markdown formatting consistency
  - Validate all internal links
  - Ensure code statistics accuracy
  - Review for clarity and completeness

## Task Execution Notes

**Execution Order**: Tasks should be executed in sequence (1-9) as later tasks may reference content from earlier tasks.

**Verification**: After each task, verify the changes against the specific requirements listed for that task.

**Content Sources**: Reference the design document for detailed specifications of what content should be included in each section.

**Formatting**: Maintain consistent markdown formatting throughout (use existing README style as template).

**Preservation**: Do not remove or significantly alter existing content that is still accurate (build instructions, testing documentation, credits).
