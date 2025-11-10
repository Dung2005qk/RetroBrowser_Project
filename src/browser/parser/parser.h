// ============================================================================
// parser.h - Public Interface for the HTML Parser Module
// ============================================================================
// MODULE PURPOSE:
// Provides a robust, single-pass, HTML-to-blocks transformation engine for
// the Win98 Retro Browser. This module sits at the critical junction between
// raw network data and visual rendering, converting arbitrary HTML (often
// malformed, legacy, or modern) into a simplified, flat list of drawable
// blocks that the GDI-based Renderer can directly consume.
//
// ARCHITECTURE OVERVIEW:
//
//   +----------------------+       +-------------------+       +------------------+
//   | Network Module       | ----> | Parser Module     | ----> | Renderer Module  |
//   | [HttpResponse.body]  |       | [HtmlBlock list]  |       | [GDI TextOut/    |
//   | std::vector<char>    |       | Flat, no nesting  |       |  BitBlt calls]   |
//   +----------------------+       +-------------------+       +------------------+
//         ^                               ^                            |
//         |                               |                            |
//         |                        This Module                         |
//         |                        (Stateless,                         |
//         |                         Single-Pass,                       |
//         |                         Forgiving)                         |
//         |                                                            |
//   Input: Binary HTML  ---->  Output: Vector of Typed Blocks  -----> Paint!
//
// DESIGN PHILOSOPHY (6 Core Principles):
//
// 1. SIMPLICITY OVER COMPLETENESS - NO DOM TREE:
//    The parser does NOT build a hierarchical Document Object Model (DOM) with
//    parent-child relationships, nested structures, or recursive traversal.
//    Instead, it produces a FLAT std::vector<HtmlBlock> representing content
//    in display order (top-to-bottom flow layout).
//    
//    RATIONALE:
//      - Memory efficiency: No pointers, no tree nodes, minimal allocation.
//      - Rendering simplicity: Renderer uses a single for-loop, no recursion.
//      - Win98 constraints: Avoids deep recursion (stack overflow risk on 1MB stack).
//      - Project scope: 8-week timeline requires focus on core functionality.
//    
//    EXAMPLE TRANSFORMATION:
//      Input:  <div><p>Text 1</p><p>Text 2</p></div>
//      Output: [{type: P, content: "Text 1"}, {type: P, content: "Text 2"}]
//              (The <div> container is discarded; only content blocks remain.)
//
// 2. ROBUSTNESS THROUGH IGNORANCE - BEST-EFFORT PARSING:
//    The parser adopts a "forgiving" stance, handling real-world HTML chaos:
//      - Unclosed tags (<p>Text with no </p>)
//      - Missing quotes (<a href=link.html>)
//      - Unknown/unsupported tags (<marquee>, <blink>, <svg>)
//      - Embedded scripts/styles (<script>alert(1)</script>)
//      - Malformed attributes (href="unclosed)
//    
//    STRATEGY: Silently skip unparseable content, extract maximum displayable
//              text, fallback to TEXT blocks when in doubt. NEVER throw exceptions
//              or crash. Prioritize stability over correctness.
//    
//    RATIONALE:
//      - Real web is broken: 90% of HTML violates W3C standards.
//      - Win98 stability: Crashes unacceptable in constrained environment.
//      - User experience: Show something (partial page) rather than nothing (blank).
//
// 3. STATELESS, PURE, RE-ENTRANT DESIGN:
//    The HtmlParser class contains NO member variables that persist between
//    Parse() calls. Each invocation is fully independent and deterministic
//    (identical input always produces identical output, no side effects).
//    
//    BENEFITS:
//      - Thread-safe foundation: Although not multi-threaded now, design is ready.
//      - Testability: Unit tests are trivial (no setup/teardown state).
//      - Debuggability: No hidden state to track; all data flows through parameters.
//      - Reusability: Single parser instance can handle multiple documents safely.
//
// 4. PERFORMANCE IS KING - RENDERER-FRIENDLY OUTPUT:
//    Single-pass parsing with O(n) time complexity, where n = HTML byte count.
//    State machine advances a cursor through input, never backtracking.
//    Memory allocations minimized via reserve() hints and stack buffers.
//    
//    OUTPUT OPTIMIZATION:
//      - BlockType enum enables fast switch statements in renderer (no strcmp).
//      - Attributes stored in std::map for O(log n) lookup (href, src, alt).
//      - Lowercase-normalized keys ("href" not "HREF") for case-insensitive access.
//      - HTML entities pre-decoded (&amp; → &) so renderer doesn't re-process.
//    
//    TARGET: Parse 100KB HTML in <100ms on 200MHz Pentium MMX (Win98 typical CPU).
//
// 5. MINIMALIST INTERFACE - STRICT ENCAPSULATION:
//    Public API exposes ONLY:
//      - One method: Parse(const std::vector<char>&)
//      - Two structs: HtmlBlock (data), ParseResult (output)
//      - Two enums: BlockType (content types), ParseStatus (error codes)
//    
//    ALL implementation details (tokenization, attribute parsing, entity decoding)
//    are private and hidden in the .cpp file. This allows internal refactoring
//    (e.g., switching from manual parsing to a faster algorithm) without breaking
//    dependent modules (UI, Renderer).
//    
//    NO EXTERNAL DEPENDENCIES: Only #include "stdafx.h". No regex (unavailable in
//    VC++6.0), no libxml (too heavy), no Boost (not C++98 compatible).
//
// 6. DOCUMENTATION-DRIVEN DEVELOPMENT:
//    Every enum value, struct field, and method is exhaustively documented with:
//      - WHAT it does (function signature)
//      - WHY design decisions were made (rationale comments)
//      - HOW to use it correctly (usage examples)
//      - WHEN it fails (error conditions)
//    
//    This header serves as the SINGLE SOURCE OF TRUTH for parser integration.
//    Developers should understand the module WITHOUT reading parser.cpp.
//
// USAGE PATTERN (Production-Ready Integration Example):
//     ```cpp
//     #include "network.h"
//     #include "parser.h"
//     
//     // Fetch HTML from network
//     Network::NetworkManager netMgr;
//     Network::HttpResponse resp = netMgr.FetchUrl("http://example.com");
//     
//     if (resp.status != Network::SUCCESS) {
//         MessageBox(NULL, "Network error!", "Error", MB_ICONERROR);
//         return;
//     }
//     
//     // Parse HTML (no conversion needed - direct binary input)
//     Parser::HtmlParser parser;
//     Parser::ParseResult result = parser.Parse(resp.body);
//     
//     if (result.status != Parser::PARSE_SUCCESS) {
//         MessageBox(NULL, result.errorMessage.c_str(), "Parse Error", MB_ICONWARNING);
//         // Optionally: Fall back to displaying raw text
//     }
//     
//     // Log warnings (non-fatal issues like skipped <script> tags)
//     for (size_t i = 0; i < result.warnings.size(); ++i) {
//         OutputDebugString(result.warnings[i].c_str());
//     }
//     
//     // Pass blocks to renderer
//     Renderer::RenderBlocks(hwnd, hdc, result.blocks);
//     
//     // Example: Access specific block data
//     for (size_t i = 0; i < result.blocks.size(); ++i) {
//         const Parser::HtmlBlock& block = result.blocks[i];
//         
//         switch (block.type) {
//             case Parser::BLOCK_A: {
//                 // Hyperlink: Check if clicked
//                 std::map<std::string, std::string>::const_iterator it = 
//                     block.attributes.find("href");
//                 if (it != block.attributes.end()) {
//                     std::string url = it->second;
//                     // ... handle navigation ...
//                 }
//                 break;
//             }
//             case Parser::BLOCK_IMG: {
//                 // Image: Download and display (with alt text fallback)
//                 std::map<std::string, std::string>::const_iterator it = 
//                     block.attributes.find("src");
//                 if (it != block.attributes.end()) {
//                     // ... fetch image via network ...
//                     // If fetch fails and block.content is not empty (has alt text):
//                     //   TextOut(hdc, x, y, block.content.c_str(), block.content.length());
//                     // This provides graceful fallback without additional map lookup
//                 }
//                 break;
//             }
//             // ... handle other block types ...
//         }
//     }
//     ```
//
// TESTING RECOMMENDATIONS:
//     UNIT TESTS (parser_test.cpp):
//       VALID HTML:
//         - Simple paragraph: "<p>Hello World</p>" -> 1 P block
//         - Multiple blocks: "<h1>Title</h1><p>Body</p>" -> 2 blocks (H1, P)
//         - Hyperlink: "<a href='link.html'>Click</a>" -> A block with href attribute
//         - Image with alt: "<img src='pic.jpg' alt='Photo'>" -> IMG block with:
//             * attributes["src"] = "pic.jpg"
//             * attributes["alt"] = "Photo" 
//             * content = "Photo" (duplicated for fast fallback access)
//         - Image without alt: "<img src='pic.jpg'>" -> IMG block with empty content
//         - HTML entities: "<p>A &amp; B</p>" -> content = "A & B"
//         - Nested tags: "<div><p>Text</p></div>" -> 1 P block (div ignored)
//       
//       MALFORMED HTML:
//         - Unclosed tags: "<p>No closing" -> 1 P block (auto-closed)
//         - Missing quotes: "<a href=link>X</a>" -> A block with href="link"
//         - Invalid nesting: "<p><div>X</div></p>" -> Extract "X" as P or TEXT
//         - Broken attributes: "<a href='unclosed>Link</a>" -> Best-effort extraction
//       
//       EDGE CASES:
//         - Empty input: "" -> EMPTY_INPUT status, 0 blocks
//         - Whitespace only: "   \n\t   " -> EMPTY_INPUT or 0 blocks
//         - No tags: "Plain text" -> 1 TEXT block
//         - Large input: 1MB HTML -> Check INPUT_TOO_LARGE or SUCCESS with cap
//         - Binary data: "\x00\xFF..." -> Handle via size, not null-termination
//         - Unsupported tags: "<script>alert(1)</script>" -> Skipped, logged warning
//       
//       PERFORMANCE:
//         - 100KB HTML: Parse in <100ms (verify with GetTickCount() timing)
//         - 1000 blocks: Ensure no memory leaks (run in loop, monitor Task Manager)
//     
//     INTEGRATION TESTS:
//       - Load real websites (Wikipedia simple page, news site) via network + parser
//       - Verify end-to-end: Network fetch -> Parse -> Render -> Display correct text
//       - Test proxy-modified HTML (e.g., stripped JS) renders properly
//     
//     STRESS TESTS:
//       - Rapid repeated parsing (50 calls/second) for memory leak detection
//       - Deeply nested HTML (100 levels) to verify no stack overflow
//       - Pathological cases: <a<a<a>>> or <<<<<>>> (should not crash)
//
// THREAD SAFETY:
//     NOT THREAD-SAFE. The HtmlParser class is designed for single-threaded use
//     within the Win98 browser's main message loop. Do NOT call Parse() from
//     multiple threads simultaneously on the same parser instance.
//     
//     If multi-threading is required in the future (e.g., background parsing on
//     a worker thread), create one HtmlParser instance per thread. The stateless
//     design makes this safe as long as each thread uses its own instance.
//
// DEPENDENCIES:
//     - stdafx.h: Provides ALL system headers (windows.h, STL) and project macros.
//                 This is the ONLY include. No external libraries required.
//     - Network Module: Supplies HttpResponse.body (std::vector<char>) as input.
//                       Parser does NOT depend on network.h directly (loose coupling).
//     - Renderer Module: Consumes ParseResult.blocks (std::vector<HtmlBlock>) as input.
//                        Parser does NOT depend on renderer.h (separation of concerns).
//
// LIMITATIONS (By Design - Out of Scope for 8-Week Project):
//     - No CSS parsing or stylesheet support (inline styles in "style" attr ignored)
//     - No JavaScript execution or DOM manipulation APIs
//     - No complex layout (tables, floats, positioning, flexbox, grid)
//     - No form input handling (<input>, <select>, <textarea> ignored)
//     - No frame/iframe support
//     - No SVG, MathML, or XML namespace handling
//     - No character encoding detection (assumes UTF-8 or ASCII from proxy)
//     - No HTML validation or repair (best-effort only)
//     - No accessibility features (ARIA attributes ignored)
//
// MAINTAINABILITY:
//     To add support for a new HTML tag (e.g., <ul> for lists):
//       1. Add BLOCK_UL to BlockType enum
//       2. Update GetBlockType() in parser.cpp to recognize "ul"
//       3. Add switch case in Parse() to extract list items
//       4. Update renderer.cpp to handle BLOCK_UL rendering
//       No changes needed to this header (enum is extensible).
// ============================================================================

#pragma once

#ifndef PARSER_H
#define PARSER_H

#include "stdafx.h" // Provides: windows.h, STL (string, vector, map), Win32 types

// ============================================================================
//  NAMESPACE: Parser
//  Encapsulates all HTML parsing functionality to prevent global namespace
//  pollution and provide clear module boundaries.
// ============================================================================
namespace Parser
{
    /**
     * @enum BlockType
     * @brief Defines the types of renderable content blocks extracted from HTML.
     *        Each type corresponds to a specific HTML element or content category.
     *        The renderer uses this enum to determine rendering behavior (font size,
     *        color, clickability, etc.) via a switch statement.
     * 
     * DESIGN DECISION: Why enum instead of class hierarchy (e.g., TextBlock, ImageBlock)?
     *   - Simpler: No vtables, no polymorphism overhead, just integer comparison.
     *   - Faster: Switch statements compile to jump tables (O(1) dispatch).
     *   - Win98-friendly: Minimal memory footprint, no dynamic dispatch.
     *   - Extensible: Adding new types requires only enum + switch case updates.
     */
    enum BlockType
    {
        /// @brief Plain text content, not enclosed in any recognized tag.
        ///        RENDERING: Default font, black color, word-wrapped.
        ///        USAGE: Fallback for unsupported tags or bare text nodes.
        ///        EXAMPLE: "Hello World" between <div> tags.
        BLOCK_TEXT,

        /// @brief Heading level 1 - typically page title or main section header.
        ///        RENDERING: Large bold font (e.g., 24pt Arial Bold), extra vertical spacing.
        ///        HTML SOURCE: <h1>Title</h1>
        BLOCK_H1,

        /// @brief Heading level 2 - subsection headers.
        ///        RENDERING: Medium bold font (e.g., 18pt Arial Bold), moderate spacing.
        ///        HTML SOURCE: <h2>Subtitle</h2>
        BLOCK_H2,

        /// @brief Heading level 3 - minor headers or list titles.
        ///        RENDERING: Slightly larger font (e.g., 14pt Arial Bold), small spacing.
        ///        HTML SOURCE: <h3>Section</h3>
        BLOCK_H3,

        /// @brief Paragraph - standard body text block.
        ///        RENDERING: Normal font (e.g., 12pt Arial), paragraph spacing above/below.
        ///        HTML SOURCE: <p>This is a paragraph.</p>
        BLOCK_P,

        /// @brief Hyperlink - clickable text that navigates to another URL.
        ///        RENDERING: Blue underlined text (or visited color: purple).
        ///        INTERACTION: On WM_LBUTTONDOWN, extract "href" attribute and navigate.
        ///        HTML SOURCE: <a href="http://example.com">Click here</a>
        ///        ATTRIBUTES: Must contain "href" key in attributes map.
        BLOCK_A,

        /// @brief Image - embedded bitmap or raster graphic.
        ///        RENDERING: Load image via network (fetch "src"), display via BitBlt.
        ///                   If image load fails, render content field (alt text) as fallback.
        ///        HTML SOURCE: <img src="logo.png" alt="Company Logo">
        ///        ATTRIBUTES: Must contain "src" key; "alt" is stored in BOTH attributes map
        ///                    AND content field for convenient fallback rendering.
        ///        NOTE: Content field holds alt text (if present) for direct renderer access
        ///              when image cannot be loaded. This avoids map lookup in error path.
        BLOCK_IMG,

        /// @brief Line break - forces a new line in text flow.
        ///        RENDERING: Advance Y position by one line height, no text drawn.
        ///        HTML SOURCE: <br> or <br/> (self-closing)
        BLOCK_BR,

        /// @brief Unknown or unsupported tag type.
        ///        USAGE: Placeholder for tags parser doesn't recognize (<marquee>, <blink>).
        ///        RENDERING: Typically ignored or treated as BLOCK_TEXT fallback.
        ///        This type should rarely appear in final output (parser tries to extract text).
        BLOCK_UNKNOWN
    };

    /**
     * @struct HtmlBlock
     * @brief A self-contained, immutable representation of a single renderable content
     *        block extracted from HTML. This is the fundamental data unit passed from
     *        the parser to the renderer.
     * 
     * DESIGN DECISIONS:
     *   - VALUE SEMANTICS: Designed to be copied/assigned safely (no pointers to manage).
     *     std::vector<HtmlBlock> can be passed by value using RVO (Return Value Optimization).
     *   
     *   - IMMUTABILITY (by convention): Once created, blocks should not be modified.
     *     Renderer treats them as read-only. This simplifies reasoning and enables caching.
     *   
     *   - FLAT STRUCTURE: No parent/child pointers, no nesting. Each block is independent.
     *   
     *   - ATTRIBUTES MAP: Why std::map instead of fixed fields (href, src, alt)?
     *     * Flexibility: Easy to add new attributes (title, id, class) without struct changes.
     *     * Memory efficiency: Only allocates space for attributes that exist.
     *     * Lookup performance: O(log n), acceptable for typical 1-5 attributes per block.
     *   
     *   - LOWERCASE KEYS: All attribute keys are normalized to lowercase ("href" not "HREF")
     *     for case-insensitive access. HTML is case-insensitive per spec, but C++ maps are not.
     */
    struct HtmlBlock
    {
        /// @brief The type of this content block. Determines rendering behavior.
        ///        CHECK THIS FIRST in renderer switch statements.
        BlockType type;

        /// @brief The textual content of the block (if applicable).
        ///        POPULATED FOR: TEXT, H1, H2, H3, P, A (link text), IMG (alt text), BR (empty)
        ///        
        ///        SPECIAL HANDLING FOR IMG BLOCKS:
        ///          For BLOCK_IMG, this field holds the alt attribute value (if present).
        ///          This design choice simplifies renderer fallback logic: when an image
        ///          fails to load, renderer can directly display block.content without
        ///          needing to search attributes map. The alt text is ALSO stored in
        ///          attributes["alt"] for completeness, but content field is preferred
        ///          for rendering fallback due to faster access (no map lookup).
        ///        
        ///        PROCESSING:
        ///          - HTML entities are DECODED: "&amp;" -> "&", "&lt;" -> "<"
        ///          - Whitespace is NORMALIZED: Multiple spaces/newlines collapsed to single space.
        ///          - Trimmed: Leading/trailing whitespace removed.
        ///        
        ///        ENCODING: UTF-8 or ASCII (depends on proxy normalization). Renderer must
        ///                  handle multi-byte characters if Win98 has appropriate codepage.
        std::string content;

        /// @brief HTML attributes associated with this block.
        ///        KEYS: Lowercase normalized attribute names (e.g., "href", "src", "alt", "style")
        ///        VALUES: Trimmed, unescaped attribute values (quotes removed)
        ///        
        ///        COMMON ATTRIBUTES BY BLOCK TYPE:
        ///          - BLOCK_A:   {"href": "http://example.com", "title": "Tooltip text"}
        ///          - BLOCK_IMG: {"src": "image.jpg", "alt": "Description", "width": "100"}
        ///          - ALL TYPES: {"style": "color:red"} (if inline style present, future use)
        ///        
        ///        EMPTY: For blocks with no attributes (most TEXT, H1, P blocks).
        ///        
        ///        USAGE PATTERN:
        ///            std::map<std::string, std::string>::const_iterator it = 
        ///                block.attributes.find("href");
        ///            if (it != block.attributes.end()) {
        ///                std::string url = it->second;
        ///                // ... use URL ...
        ///            }
        std::map<std::string, std::string> attributes;

        /**
         * @brief Default constructor - initializes to a safe "unknown" state.
         *        Ensures uninitialized blocks are detectable and won't cause UB.
         */
        HtmlBlock() : type(BLOCK_UNKNOWN), content("") {}

        /**
         * @brief Convenience constructor for blocks with type and content only.
         * @param t The block type (e.g., BLOCK_P, BLOCK_H1)
         * @param c The text content (default: empty string)
         * 
         * USAGE: HtmlBlock(BLOCK_TEXT, "Plain text paragraph");
         */
        HtmlBlock(BlockType t, const std::string& c = "")
            : type(t), content(c) {}

        /**
         * @brief Full constructor for blocks with type, content, and attributes.
         * @param t The block type
         * @param c The text content
         * @param a The attributes map
         * 
         * USAGE: HtmlBlock(BLOCK_A, "Click", {{"href", "http://example.com"}});
         *        Note: C++98 doesn't support initializer lists, use insert() instead:
         *          std::map<std::string, std::string> attrs;
         *          attrs["href"] = "http://example.com";
         *          HtmlBlock block(BLOCK_A, "Click", attrs);
         */
        HtmlBlock(BlockType t, const std::string& c, const std::map<std::string, std::string>& a)
            : type(t), content(c), attributes(a) {}
    };

    /**
     * @enum ParseStatus
     * @brief Defines the possible outcomes of a Parse() operation.
     *        This enum provides fine-grained error reporting, allowing the caller to
     *        distinguish between different failure modes and take appropriate action.
     * 
     * ALWAYS check this field FIRST in ParseResult before accessing blocks or warnings.
     */
    enum ParseStatus
    {
        /// @brief Parsing completed successfully. The blocks vector contains extracted content.
        ///        NOTE: This does NOT guarantee the HTML was valid or complete, only that the
        ///        parser was able to extract some content without fatal errors. Check warnings
        ///        for non-critical issues (skipped tags, malformed attributes, etc.).
        PARSE_SUCCESS,

        /// @brief The input HTML was empty or contained only whitespace.
        ///        CAUSES:
        ///          - Zero-length vector passed to Parse()
        ///          - HTML containing only spaces, tabs, newlines
        ///          - Network returned empty body (rare but possible)
        ///        USER ACTION: Display "No content available" message or treat as blank page.
        ///        NOTE: blocks vector will be empty; no error message set.
        EMPTY_INPUT,

        /// @brief The input exceeded the maximum allowed size (1MB).
        ///        CAUSES:
        ///          - Network returned huge HTML (malicious or misconfigured server)
        ///          - Proxy failed to enforce size limit
        ///        USER ACTION: Inform user the page is too large to display.
        ///        RATIONALE: Win98 has limited RAM (~64-256MB typical); a 10MB HTML would
        ///                   cause severe performance degradation or OOM crash.
        ///        NOTE: Synchronized with Network module's 1MB response body cap.
        INPUT_TOO_LARGE,

        /// @brief A fatal error occurred during parsing, preventing content extraction.
        ///        CAUSES:
        ///          - Out of memory (std::vector allocation failed)
        ///          - Internal parser bug (should never happen in production)
        ///          - Catastrophic input corruption
        ///        USER ACTION: Display error message, log for debugging, fall back to raw text.
        ///        NOTE: errorMessage field will contain diagnostic details.
        PARSE_FAILED,

        /// @brief An unexpected or unhandled error occurred.
        ///        USAGE: Default state for uninitialized ParseResult objects.
        ///        CAUSES: Logic error, missing error case, or object used before Parse() called.
        ///        USER ACTION: File bug report with reproduction steps.
        UNKNOWN_ERROR
    };

    /**
     * @struct ParseResult
     * @brief A self-contained, immutable-after-return structure holding the complete
     *        result of a Parse() operation. Designed for value semantics and safety.
     * 
     * DESIGN PHILOSOPHY (mirrors HttpResponse from network.h):
     *   - Self-documenting: All information needed to handle the result is included.
     *   - Error-first: Status field must be checked before accessing other fields.
     *   - Robustness: Includes both fatal errors (status) and warnings (non-fatal issues).
     *   - Value semantics: Safe to copy/return by value (uses RVO in practice).
     * 
     * USAGE RULES:
     *   1. ALWAYS check `status` field FIRST before accessing other fields.
     *   2. If status != PARSE_SUCCESS, only `errorMessage` is valid (blocks may be empty).
     *   3. If status == PARSE_SUCCESS, all fields are valid and safe to use.
     *   4. Check `warnings` even on success to log skipped content (e.g., <script> tags).
     */
    struct ParseResult
    {
        /// @brief The overall parsing status. CHECK THIS FIRST.
        ///        Only when this equals PARSE_SUCCESS is the blocks vector reliable.
        ParseStatus status;

        /// @brief Flat list of content blocks in display order (top to bottom).
        ///        VALID ONLY IF: status == PARSE_SUCCESS (or EMPTY_INPUT, in which case empty).
        ///        MEMORY: Pre-allocated with reserve() hint to minimize reallocations.
        ///        USAGE: Iterate with for-loop, switch on block.type, access block.content/attributes.
        ///        OWNERSHIP: Caller receives full copy (or RVO optimization); parser retains nothing.
        std::vector<HtmlBlock> blocks;

        /// @brief Human-readable error message for fatal failures.
        ///        BEHAVIOR:
        ///          - If status == PARSE_SUCCESS or EMPTY_INPUT: Empty string (no error).
        ///          - If status != PARSE_SUCCESS: Detailed diagnostic message including:
        ///              * Error type ("Out of memory", "Input too large")
        ///              * Context ("at position 1234", "while parsing <a> tag")
        ///              * Suggested action ("Reduce page size" or "Retry with smaller input")
        ///        USAGE: Display in MessageBox, log to file, or show in browser error page.
        std::string errorMessage;

        /// @brief Non-fatal issues encountered during parsing (informational).
        ///        POPULATED EVEN IF: status == PARSE_SUCCESS
        ///        EXAMPLES:
        ///          - "Skipped <script> tag at position 567"
        ///          - "Ignored unsupported <marquee> element"
        ///          - "Malformed attribute in <a> tag, using best-effort parse"
        ///          - "HTML entity '&unknown;' not recognized, left as-is"
        ///        USAGE: Log to debug output (OutputDebugString) or console for troubleshooting.
        ///                Typically hidden from end users unless debug mode enabled.
        ///        NOTE: Accumulates during parsing; may contain 0-100+ entries for complex HTML.
        std::vector<std::string> warnings;

        /**
         * @brief Default constructor - initializes to a safe sentinel error state.
         *        Ensures uninitialized ParseResult objects are immediately detectable.
         */
        ParseResult()
            : status(UNKNOWN_ERROR),
              errorMessage("Uninitialized ParseResult - Parse() was never called")
        {
            blocks.reserve(50); // Hint: typical page has 50-200 blocks; reduces reallocations
        }
    };

    /**
     * @class HtmlParser
     * @brief Stateless HTML-to-blocks transformation engine.
     *        Provides a single public method Parse() that converts raw HTML bytes into
     *        a flat list of renderable content blocks. No persistent state between calls.
     * 
     * STATELESS DESIGN RATIONALE:
     *   - Re-entrant: Same instance can parse multiple documents safely.
     *   - Thread-ready: Could be used from multiple threads (one instance per thread).
     *   - Testable: No setup/teardown; each test is independent.
     *   - Debuggable: No hidden state to track; all data flows through parameters/return.
     * 
     * PERFORMANCE CHARACTERISTICS:
     *   - Time Complexity: O(n) single-pass, where n = input size in bytes.
     *   - Space Complexity: O(m) output size, where m = number of blocks (typically m << n).
     *   - Stack Usage: Minimal (no recursion); safe for Win98's 1MB default stack.
     *   - Allocations: Primarily in output vector; uses reserve() to minimize reallocs.
     * 
     * ALGORITHM OVERVIEW (High-Level):
     *   1. INPUT VALIDATION: Check size, emptiness, binary safety.
     *   2. STATE MACHINE SCAN: Single cursor advancing through input:
     *      - Outside tag: Accumulate text, stop at '<'.
     *      - Inside tag: Parse tag name, attributes, stop at '>'.
     *      - Special handling: Comments (<!-- -->), CDATA, doctype (skip).
     *   3. TAG PROCESSING:
     *      - Identify block type via case-insensitive tag name match.
     *      - Extract attributes (key="value" pairs), normalize keys to lowercase.
     *      - Skip unsupported tags (<script>, <style>, <head>) and their content.
     *   4. TEXT PROCESSING:
     *      - Decode HTML entities (&amp;, &lt;, &gt;, &quot;, &#39;).
     *      - Normalize whitespace (collapse \n\r\t\s+ to single space).
     *      - Trim leading/trailing whitespace.
     *   5. OUTPUT CONSTRUCTION: Append HtmlBlock structs to result vector.
     *   6. ERROR HANDLING: Catch allocation failures, malformed input; populate warnings.
     * 
     * THREAD SAFETY:
     *   NOT THREAD-SAFE for concurrent Parse() calls on the SAME instance from multiple threads.
     *   However, the stateless design means creating one instance per thread IS safe.
     *   For single-threaded Win32 message loop usage, this is not a concern.
     * 
     * USAGE CONSTRAINTS:
     *   - Input must be binary-safe (uses .size(), not null-termination).
     *   - Input size capped at 1MB (INPUT_TOO_LARGE error if exceeded).
     *   - Designed for HTML only (XML, XHTML may partially work but untested).
     *   - Does NOT validate HTML correctness (no DTD, no schema enforcement).
     *   - Does NOT repair HTML (no auto-closing tags, no error correction beyond best-effort).
     * 
     * INSTANCE LIFETIME:
     *   Typically instantiate ONCE at application startup and reuse for all parses:
     *     Parser::HtmlParser g_parser; // Global or long-lived local
     *     // ... later ...
     *     ParseResult r1 = g_parser.Parse(html1);
     *     ParseResult r2 = g_parser.Parse(html2); // Safe reuse
     */
    class HtmlParser
    {
    public:
        /**
         * @brief Default constructor - no initialization needed (stateless design).
         */
        HtmlParser() {}

        /**
         * @brief Destructor - no cleanup needed (no dynamic resources held).
         */
        ~HtmlParser() {}

        /**
         * @brief Parses raw HTML bytes into a flat list of renderable content blocks.
         *        This is the ONLY public method and the core functionality of the module.
         * 
         * @param rawHtml The raw HTML content as a byte vector (from Network::HttpResponse.body).
         *                MUST be binary-safe (uses .size(), not null-termination).
         *                Encoding assumed: UTF-8 or ASCII (proxy should normalize).
         *                Maximum size: 1MB (1,048,576 bytes); returns INPUT_TOO_LARGE if exceeded.
         * 
         * @return ParseResult struct containing:
         *         - status: PARSE_SUCCESS or error code (CHECK THIS FIRST)
         *         - blocks: Vector of HtmlBlock structs in display order
         *         - errorMessage: Diagnostic info if status != PARSE_SUCCESS
         *         - warnings: Non-fatal issues (e.g., skipped tags) even if status == SUCCESS
         * 
         * ALGORITHM (State Machine):
         *   - Initialize cursor to start of rawHtml.
         *   - While cursor < end:
         *     * If current char == '<':
         *       - Scan until '>' to extract full tag.
         *       - Identify tag type: opening (<p>), closing (</p>), self-closing (<br/>).
         *       - Extract tag name (e.g., "p", "a", "img"), convert to lowercase.
         *       - If supported tag: Call GetBlockType() to map name -> BlockType enum.
         *       - If opening/self-closing: Parse attributes (call ParseAttributes helper).
         *       - SPECIAL: For BLOCK_IMG, copy attributes["alt"] to content field if present.
         *         This enables fast fallback rendering without map lookup.
         *       - If unsupported/script/style: Skip tag and content until closing tag.
         *       - Append HtmlBlock to result.blocks vector.
         *     * Else (text content):
         *       - Accumulate characters until next '<'.
         *       - Call TrimAndDecode() to normalize whitespace and decode entities.
         *       - If non-empty after trimming: Append as BLOCK_TEXT.
         *   - Return ParseResult with blocks populated and status=PARSE_SUCCESS.
         * 
         * CRITICAL BEHAVIORS:
         *   - BLOCKING: This is a SYNCHRONOUS function. It will NOT return until parsing
         *     completes or errors out. Typical duration: 10-50ms for 100KB HTML on Win98.
         *   
         *   - BINARY-SAFE: Uses rawHtml.size() for length, handles embedded null bytes (\0).
         *   
         *   - SINGLE-PASS: Cursor advances forward only; no backtracking or multi-pass.
         *   
         *   - NO EXCEPTIONS: All errors reported via ParseResult.status; never throws.
         *   
         *   - BEST-EFFORT: Continues parsing after encountering malformed content, extracting
         *     whatever is salvageable. Partial results better than total failure.
         * 
         * ERROR CONDITIONS:
         *   - rawHtml.empty() || all whitespace -> status=EMPTY_INPUT, blocks empty
         *   - rawHtml.size() > 1MB -> status=INPUT_TOO_LARGE, blocks empty
         *   - Out of memory during allocation -> status=PARSE_FAILED, errorMessage set
         *   - Internal logic error (bug) -> status=UNKNOWN_ERROR, errorMessage set
         * 
         * PERFORMANCE NOTES:
         *   - Reserve hint: result.blocks.reserve(50) to minimize reallocations.
         *   - Avoid std::string copies: Use const& and .assign() where possible.
         *   - Stack buffers: Use _alloca() for temp arrays <1KB to avoid heap thrash.
         *   - Target: Parse 100KB in <100ms on 200MHz Pentium MMX.
         * 
         * WARNINGS (Non-Fatal):
         *   These populate result.warnings but do NOT change status:
         *     - Skipped <script> tag (JS not supported)
         *     - Ignored <style> tag (CSS not supported)
         *     - Unrecognized HTML entity (e.g., &unknownEntity;)
         *     - Malformed attribute (missing quotes, unclosed value)
         *     - Unsupported tag (<marquee>, <blink>, <embed>)
         * 
         * @note Designed to be called repeatedly (stateless); safe to reuse parser instance.
         * 
         * @test UNIT TESTS MUST COVER:
         *       - Empty input: Parse({}) -> EMPTY_INPUT
         *       - Simple text: Parse("Hello") -> 1 BLOCK_TEXT
         *       - Single tag: Parse("<p>Text</p>") -> 1 BLOCK_P
         *       - Multiple blocks: Parse("<h1>Title</h1><p>Body</p>") -> 2 blocks
         *       - Hyperlink: Parse("<a href='link'>Click</a>") -> BLOCK_A with href attr
         *       - Image with alt: Parse("<img src='pic.jpg' alt='Logo'>") -> BLOCK_IMG with:
         *           * src attr, alt attr in attributes map
         *           * content field = "Logo" (for fast fallback rendering)
         *       - Image no alt: Parse("<img src='pic.jpg'>") -> BLOCK_IMG with empty content
         *       - Entities: Parse("<p>A &amp; B</p>") -> content = "A & B"
         *       - Nested tags: Parse("<div><p>Text</p></div>") -> 1 BLOCK_P (div ignored)
         *       - Unclosed tag: Parse("<p>Text") -> 1 BLOCK_P (auto-closed)
         *       - Large input: Parse(1MB+1 byte) -> INPUT_TOO_LARGE
         *       - Binary data: Parse("\x00\xFF...") -> No crashes
         */
        ParseResult Parse(const std::vector<char>& rawHtml) const;

    private:
        // ====================================================================
        // NON-COPYABLE DESIGN (C++98 Pattern)
        // ====================================================================
        // Disable copy constructor and assignment operator to prevent accidental
        // copying. Although the parser is stateless (no harm in copying), disabling
        // copy makes the API clearer: "Use one instance, don't clone it."
        // In C++11+, we would use "= delete" syntax instead.
        HtmlParser(const HtmlParser&);
        HtmlParser& operator=(const HtmlParser&);

        // ====================================================================
        // PRIVATE HELPER METHODS (Implementation Details)
        // ====================================================================
        // These methods are declared here for transparency and maintainability,
        // allowing header readers to understand the module structure without
        // reading the .cpp file. They remain private to preserve encapsulation.

        /**
         * @brief Consolidates block creation logic to eliminate code duplication.
         * @param tagName The lowercase tag name (e.g., "p", "img", "a")
         * @param attributes Map of HTML attributes for this tag
         * @param result ParseResult to append block/warnings to
         * 
         * This helper eliminates 6x code duplication across FSM states where
         * tags are finalized. It handles: type lookup, block creation, IMG
         * special case (alt->content), and unknown tag warnings.
         */
        void FinalizeAndAddBlock(
            const std::string& tagName,
            const std::map<std::string, std::string>& attributes,
            ParseResult& result) const;

        /**
         * @brief Maps an HTML tag name (string) to a BlockType enum value.
         * @param tagName The tag name extracted from HTML (e.g., "p", "h1", "img").
         *                Expected to be LOWERCASE (parser normalizes before calling).
         * @return Corresponding BlockType enum, or BLOCK_UNKNOWN if unrecognized.
         * 
         * IMPLEMENTATION HINT:
         *   - Use case-insensitive string comparison (already lowercase from caller).
         *   - Fast path: if-else chain or switch on first character (e.g., 'p' -> "p", "pre").
         *   - Slow path: strcmp fallback for less common tags.
         * 
         * EXAMPLES:
         *   GetBlockType("p") -> BLOCK_P
         *   GetBlockType("h1") -> BLOCK_H1
         *   GetBlockType("a") -> BLOCK_A
         *   GetBlockType("marquee") -> BLOCK_UNKNOWN
         */
        BlockType GetBlockType(const std::string& tagName) const;

        /**
         * @brief Extracts attribute key-value pairs from a tag's attribute string.
         * @param attrStr The substring containing attributes (e.g., 'href="link" alt="text"').
         * @return Map of attributes with LOWERCASE keys and trimmed, unescaped values.
         * 
         * PARSING RULES:
         *   - Keys are case-insensitive: "HREF" and "href" both map to key "href".
         *   - Values can be quoted (single or double) or unquoted (stops at space).
         *   - Handles escaped quotes inside values: href="He said \"Hi\"" (future work).
         *   - Whitespace around '=' is ignored: "href = 'link'" works.
         *   - Malformed attributes (missing '=', unclosed quotes) logged as warning, skipped.
         * 
         * EXAMPLES:
         *   Input:  'href="http://example.com" title="Tooltip"'
         *   Output: { {"href", "http://example.com"}, {"title", "Tooltip"} }
         *   
         *   Input:  'src=image.jpg alt=Logo'
         *   Output: { {"src", "image.jpg"}, {"alt", "Logo"} }
         *   
         *   Input:  'href='unclosed'
         *   Output: { {"href", "unclosed"} } + warning logged
         */
        std::map<std::string, std::string> ParseAttributes(const std::string& attrStr) const;

        /**
         * @brief Trims leading/trailing whitespace and decodes HTML entities.
         * @param text The raw text string extracted from HTML (between tags).
         * @return Cleaned string with whitespace normalized and entities decoded.
         * 
         * OPERATIONS:
         *   1. Trim whitespace: Remove leading/trailing spaces, tabs, newlines.
         *   2. Collapse internal whitespace: "\n\n  text  \n" -> " text "
         *   3. Decode entities: Convert &amp; -> &, &lt; -> <, &gt; -> >, &quot; -> ", &#39; -> '
         *   4. Handle numeric entities: &#65; -> 'A', &#x41; -> 'A' (future work, optional)
         * 
         * LIMITATIONS:
         *   - Only decodes 5 most common entities (sufficient for 95% of pages).
         *   - Unknown entities (e.g., &rarr;) left as-is with warning logged.
         *   - Does NOT validate entity syntax strictly (best-effort only).
         * 
         * EXAMPLES:
         *   Input:  "  Hello &amp; World  "
         *   Output: "Hello & World"
         *   
         *   Input:  "A&lt;B"
         *   Output: "A<B"
         *   
         *   Input:  "\n\tText\n\n"
         *   Output: "Text"
         */
        std::string TrimAndDecode(const std::string& text) const;

        /**
         * @brief Overloaded version that also detects unknown entities and logs warnings.
         * @param text The raw text string extracted from HTML (between tags).
         * @param warnings Vector to append warning messages about unknown entities.
         * @return Cleaned string with whitespace normalized and entities decoded.
         * 
         * OPTIMIZATION: This single-pass version detects unknown entities during
         * the decode process, eliminating the need for a separate scan pass.
         */
        std::string TrimAndDecode(const std::string& text, std::vector<std::string>& warnings) const;
    };

} // namespace Parser

#endif // PARSER_H

// ============================================================================
// END OF HEADER
// ============================================================================
// IMPLEMENTATION NOTES (for parser.cpp development):
//   - Use const char* pointer arithmetic for scanning (faster than std::string.find).
//   - Allocate temp buffers with _alloca() for <1KB, malloc() for larger.
//   - Reserve output vector: result.blocks.reserve(estimatedBlocks).
//   - Profile hotspots: GetBlockType() and ParseAttributes() called most often.
//   - Test with malformed HTML corpus from: validator.w3.org/check (collect errors).
//   - Benchmark against real websites: Wikipedia, BBC News (text-heavy, legacy HTML).
//   
//   CRITICAL: For BLOCK_IMG tags, after calling ParseAttributes(), copy the "alt"
//   attribute value (if present) to HtmlBlock.content field. This optimization
//   allows renderer to access alt text directly without map lookup when image
//   loading fails. Example implementation:
//     if (blockType == BLOCK_IMG && attrs.find("alt") != attrs.end()) {
//         block.content = attrs["alt"];
//     }
// 
// INTEGRATION CHECKLIST:
//   [ ] Compiles with VC++ 6.0 (no C++11 features)
//   [ ] Links against stdafx.pch only (no external libs)
//   [ ] Parse() handles Network::HttpResponse.body directly
//   [ ] Renderer can iterate result.blocks in WM_PAINT handler
//   [ ] Unit tests pass (see TESTING RECOMMENDATIONS in class doc)
//   [ ] Memory leaks checked with Task Manager (repeated parse calls)
//   [ ] Performance verified: 100KB HTML in <100ms on Win98 VM
// ============================================================================