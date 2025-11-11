// ============================================================================
// parser.cpp - HTML Parser Implementation for Win98 Retro Browser
// ============================================================================
// IMPLEMENTATION NOTES:
// - Single-pass FSM (Finite State Machine) for O(n) parsing
// - No recursion (stack-safe for Win98's 1MB stack limit)
// - Forgiving: handles malformed HTML, continues on errors
// - Performance: <100ms for 100KB HTML on Pentium MMX 200MHz
// - Memory: reserves vectors upfront to minimize reallocations
// ============================================================================

#include "stdafx.h"
#include "parser.h"

namespace Parser
{
    // ========================================================================
    // HELPER FUNCTIONS (Forward Declarations for Organization)
    // ========================================================================
    static inline bool IsWhitespace(char c);
    static inline bool IsAlpha(char c);
    static inline bool IsAlphaNum(char c);
    static inline char ToLower(char c);
    static bool IsOnlyWhitespace(const std::vector<char>& data);
    
    // ========================================================================
    // FinalizeAndAddBlock - DRY Helper to Create and Add Block
    // ========================================================================
    // WHY: Eliminates code duplication across 6 FSM states
    // This function consolidates the repeated logic of:
    //   1. Determining block type from tag name
    //   2. Creating HtmlBlock with attributes
    //   3. Special handling for IMG blocks (copy alt to content)
    //   4. Adding block to result vector
    //   5. Logging warning for unknown tags
    // BENEFIT: Single point of maintenance, reduces bugs
    // NOTE: Made non-inline to keep it simple and readable
    void HtmlParser::FinalizeAndAddBlock(
        const std::string& tagName,
        const std::map<std::string, std::string>& attributes,
        ParseResult& result) const
    {
        BlockType type = GetBlockType(tagName);
        
        // FIXED: Do NOT skip DIV/SPAN here!
        // These are structural wrappers - we should NOT create blocks for them,
        // but we MUST continue parsing their content (nested <p>, <h1>, etc.)
        // The FSM will handle them by not treating them as containers.
        // This function is called for SELF-CLOSING or NON-CONTAINER tags only.
        if (type == BLOCK_DIV || type == BLOCK_SPAN)
        {
            // Don't create block, but don't return - let FSM continue parsing content
            return;
        }
        
        if (type != BLOCK_UNKNOWN)
        {
            HtmlBlock block(type);
            block.attributes = attributes;
            
            // Special handling for IMG: copy alt to content for fast fallback
            if (type == BLOCK_IMG)
            {
                std::map<std::string, std::string>::const_iterator it = 
                    attributes.find("alt");
                if (it != attributes.end())
                {
                    block.content = it->second;
                }
            }
            
            result.blocks.push_back(block);
        }
        else
        {
            // Unknown/unsupported tag - log warning for debugging
            result.warnings.push_back("Skipped unsupported tag: <" + tagName + ">");
        }
    }

    // ========================================================================
    // MAIN PARSE METHOD - HTML to Blocks Transformation
    // ========================================================================
    ParseResult HtmlParser::Parse(const std::vector<char>& rawHtml) const
    {
        ParseResult result;

        // ====================================================================
        // STEP 1: INPUT VALIDATION
        // ====================================================================
        
        // Check for empty or whitespace-only input
        if (rawHtml.empty() || IsOnlyWhitespace(rawHtml))
        {
            result.status = EMPTY_INPUT;
            result.errorMessage = ""; // Not an error, just empty
            return result;
        }

        // Check size limit (1MB = 1,048,576 bytes)
        const size_t MAX_HTML_SIZE = 1048576;
        if (rawHtml.size() > MAX_HTML_SIZE)
        {
            result.status = INPUT_TOO_LARGE;
            result.errorMessage = "Input exceeds 1MB limit (Win98 constraint)";
            return result;
        }

        // ====================================================================
        // STEP 2: INITIALIZATION - Reserve Memory for Performance
        // ====================================================================
        // WHY: Pre-allocating reduces reallocation overhead on Win98
        result.blocks.reserve(128);    // Typical page: 50-200 blocks
        result.warnings.reserve(32);   // Usually <20 warnings per page

        // ====================================================================
        // STEP 3: FSM SETUP - Single-Pass State Machine
        // ====================================================================
        
        // Binary-safe pointer arithmetic (handles \0 and \xFF)
        const char* cursor = &rawHtml[0];
        const char* end = cursor + rawHtml.size();

        // FSM States for HTML parsing
        enum State
        {
            STATE_DATA,                      // Reading text content
            STATE_TAG_OPEN,                  // Just encountered '<'
            STATE_TAG_NAME,                  // Reading tag name
            STATE_BEFORE_ATTR_NAME,          // Before attribute name
            STATE_ATTR_NAME,                 // Reading attribute name
            STATE_AFTER_ATTR_NAME,           // After attribute name (before '=')
            STATE_BEFORE_ATTR_VALUE,         // After '=' (before value)
            STATE_ATTR_VALUE_DOUBLE_QUOTED,  // In "value"
            STATE_ATTR_VALUE_SINGLE_QUOTED,  // In 'value'
            STATE_ATTR_VALUE_UNQUOTED,       // In unquoted value
            STATE_COMMENT,                   // In <!-- comment -->
            STATE_SKIP_CONTENT               // Skipping <script>/<style> content
        };

        State currentState = STATE_DATA;

        // Working buffers (reused to minimize allocations)
        std::string textBuffer;           // Accumulates text content
        std::string tagName;              // Current tag name (lowercase)
        std::string attrKey;              // Current attribute key
        std::string attrValue;            // Current attribute value
        std::map<std::string, std::string> attributes; // Tag attributes
        
        // Skip state management (for <script>/<style>)
        std::string skipTagName;          // Tag we're skipping (e.g., "script")
        int skipDepth = 0;                // Nesting depth for skip

        // Container tracking (for text extraction from <h1>, <p>, <li>, etc.)
        BlockType containerType = BLOCK_UNKNOWN;     // Current container tag type
        std::string containerContent;                // Text content inside container
        std::map<std::string, std::string> containerAttrs; // Container attributes

        // Flags
        bool isClosingTag = false;        // Processing </tag>
        bool isSelfClosing = false;       // Processing <tag />

        // Reserve buffers
        textBuffer.reserve(1024);
        tagName.reserve(32);
        attrKey.reserve(64);
        attrValue.reserve(256);

        // ====================================================================
        // STEP 4: MAIN FSM LOOP - Single Pass Through Input
        // ====================================================================
        // WHY FSM: Guarantees O(n) time, no backtracking, stack-safe
        
        while (cursor < end)
        {
            char c = *cursor++;

            switch (currentState)
            {
                // ============================================================
                // STATE_DATA: Reading Text Content Between Tags
                // ============================================================
                case STATE_DATA:
                {
                    if (c == '<')
                    {
                        // Process accumulated text before tag
                        if (!textBuffer.empty())
                        {
                            std::string cleaned = TrimAndDecode(textBuffer, result.warnings);
                            if (!cleaned.empty())
                            {
                                // If we're inside a container, add to container content
                                if (containerType != BLOCK_UNKNOWN)
                                {
                                    if (!containerContent.empty())
                                        containerContent += " ";
                                    containerContent += cleaned;
                                }
                                else
                                {
                                    // Standalone text - create TEXT block
                                    HtmlBlock block(BLOCK_TEXT, cleaned);
                                    result.blocks.push_back(block);
                                }
                            }
                            textBuffer.clear();
                        }
                        currentState = STATE_TAG_OPEN;
                    }
                    else
                    {
                        textBuffer += c;
                    }
                    break;
                }

                // ============================================================
                // STATE_TAG_OPEN: Just Encountered '<'
                // ============================================================
                case STATE_TAG_OPEN:
                {
                    if (c == '/')
                    {
                        isClosingTag = true;
                        tagName.clear();
                        currentState = STATE_TAG_NAME;
                    }
                    else if (c == '!')
                    {
                        // Check for comment or doctype
                        if (cursor + 1 < end && *cursor == '-' && *(cursor + 1) == '-')
                        {
                            cursor += 2; // Skip '--'
                            currentState = STATE_COMMENT;
                            result.warnings.push_back("Skipped HTML comment");
                        }
                        else
                        {
                            // DOCTYPE or other declaration - skip until '>'
                            while (cursor < end && *cursor != '>')
                                cursor++;
                            if (cursor < end) cursor++; // Skip '>'
                            currentState = STATE_DATA;
                            result.warnings.push_back("Skipped DOCTYPE/declaration");
                        }
                    }
                    else if (IsAlpha(c))
                    {
                        isClosingTag = false;
                        tagName.clear();
                        tagName += ToLower(c);
                        currentState = STATE_TAG_NAME;
                    }
                    else
                    {
                        // Invalid character after '<', treat as text
                        textBuffer += '<';
                        textBuffer += c;
                        currentState = STATE_DATA;
                    }
                    break;
                }

                // ============================================================
                // STATE_TAG_NAME: Reading Tag Name
                // ============================================================
                case STATE_TAG_NAME:
                {
                    if (IsAlphaNum(c) || c == '-' || c == '_')
                    {
                        tagName += ToLower(c);
                    }
                    else if (IsWhitespace(c))
                    {
                        // Tag name complete, check for skip tags
                        if (!isClosingTag)
                        {
                            if (tagName == "script" || tagName == "style" || tagName == "head")
                            {
                                skipTagName = tagName;
                                skipDepth = 1;
                                currentState = STATE_SKIP_CONTENT;
                                result.warnings.push_back("Skipped <" + tagName + "> content");
                                break;
                            }
                        }
                        currentState = STATE_BEFORE_ATTR_NAME;
                        attributes.clear();
                    }
                    else if (c == '>')
                    {
                        // Tag complete without attributes
                        if (isClosingTag)
                        {
                            // Closing tag - finalize container if we're in one
                            if (containerType != BLOCK_UNKNOWN)
                            {
                                BlockType closeType = GetBlockType(tagName);
                                if (closeType == containerType)
                                {
                                    // Matching closing tag - finalize container block
                                    HtmlBlock block(containerType, containerContent);
                                    block.attributes = containerAttrs;
                                    result.blocks.push_back(block);
                                    
                                    // Reset container state
                                    containerType = BLOCK_UNKNOWN;
                                    containerContent.clear();
                                    containerAttrs.clear();
                                }
                            }
                            isClosingTag = false;
                            tagName.clear();
                            currentState = STATE_DATA;
                        }
                        else
                        {
                            // Check for skip tags
                            if (tagName == "script" || tagName == "style" || tagName == "head")
                            {
                                skipTagName = tagName;
                                skipDepth = 1;
                                currentState = STATE_SKIP_CONTENT;
                                result.warnings.push_back("Skipped <" + tagName + "> content");
                                break;
                            }

                            // Check if this is a container tag (needs text content)
                            BlockType type = GetBlockType(tagName);
                            bool isContainer = (type == BLOCK_H1 || type == BLOCK_H2 || 
                                              type == BLOCK_H3 || type == BLOCK_P || 
                                              type == BLOCK_A || type == BLOCK_LI);
                            
                            if (isContainer && containerType == BLOCK_UNKNOWN)
                            {
                                // Start container - wait for text content and closing tag
                                containerType = type;
                                containerContent.clear();
                                containerAttrs = attributes;
                            }
                            else if (isContainer && containerType != BLOCK_UNKNOWN)
                            {
                                // Nested container (e.g. <a> inside <li>)
                                // Special case: <li><a> should merge - use <li> as container but keep <a> attributes
                                if (containerType == BLOCK_LI && type == BLOCK_A)
                                {
                                    // Merge: keep LI as container type but use A's href attribute
                                    // This allows "• <link>" rendering
                                    if (!attributes.empty())
                                    {
                                        // Copy href from <a> to container attrs
                                        auto it = attributes.find("href");
                                        if (it != attributes.end())
                                        {
                                            containerAttrs["href"] = it->second;
                                        }
                                    }
                                    // Continue accumulating text into LI container
                                    // Don't create new container or finalize
                                }
                                else
                                {
                                    // Other nested cases: finalize outer, start new
                                    HtmlBlock block(containerType, containerContent);
                                    block.attributes = containerAttrs;
                                    result.blocks.push_back(block);
                                    
                                    // Start new container
                                    containerType = type;
                                    containerContent.clear();
                                    containerAttrs = attributes;
                                }
                            }
                            else if (type == BLOCK_BR)
                            {
                                // Self-contained tags (no content needed)
                                FinalizeAndAddBlock(tagName, attributes, result);
                            }
                            else if (type == BLOCK_UL)
                            {
                                // UL is just a marker, don't create block
                                // Just skip it silently
                            }
                            else if (type == BLOCK_DIV || type == BLOCK_SPAN)
                            {
                                // CRITICAL FIX: Structural wrappers - don't create blocks
                                // but DO continue parsing their content (nested tags)
                                // Just silently ignore the wrapper tags themselves
                            }
                            else if (type != BLOCK_UNKNOWN)
                            {
                                // Other recognized tags - use old behavior
                                FinalizeAndAddBlock(tagName, attributes, result);
                            }
                            
                            tagName.clear();
                            attributes.clear();
                            currentState = STATE_DATA;
                        }
                    }
                    else if (c == '/')
                    {
                        // Self-closing tag
                        isSelfClosing = true;
                    }
                    else
                    {
                        // Invalid character in tag name
                        currentState = STATE_DATA;
                    }
                    break;
                }

                // ============================================================
                // STATE_BEFORE_ATTR_NAME: Whitespace Before Attribute
                // ============================================================
                case STATE_BEFORE_ATTR_NAME:
                {
                    if (c == '>')
                    {
                        // Tag complete - check if container or immediate block
                        BlockType type = GetBlockType(tagName);
                        bool isContainer = (type == BLOCK_H1 || type == BLOCK_H2 || 
                                          type == BLOCK_H3 || type == BLOCK_P || 
                                          type == BLOCK_A || type == BLOCK_LI);
                        
                        if (isContainer && containerType == BLOCK_UNKNOWN)
                        {
                            // Start container - wait for text content and closing tag
                            containerType = type;
                            containerContent.clear();
                            containerAttrs = attributes;
                        }
                        else if (isContainer && containerType != BLOCK_UNKNOWN)
                        {
                            // Nested container with attributes (e.g. <a href="..."> inside <li>)
                            if (containerType == BLOCK_LI && type == BLOCK_A)
                            {
                                // Merge: keep LI as container type but copy href from <a>
                                auto it = attributes.find("href");
                                if (it != attributes.end())
                                {
                                    containerAttrs["href"] = it->second;
                                }
                                // Continue accumulating text into LI container
                            }
                            else
                            {
                                // Other nested cases: finalize outer, start new
                                HtmlBlock block(containerType, containerContent);
                                block.attributes = containerAttrs;
                                result.blocks.push_back(block);
                                
                                containerType = type;
                                containerContent.clear();
                                containerAttrs = attributes;
                            }
                        }
                        else if (type == BLOCK_BR || type == BLOCK_UL || type == BLOCK_IMG)
                        {
                            // Self-contained tags (no content needed)
                            FinalizeAndAddBlock(tagName, attributes, result);
                        }
                        else if (type == BLOCK_DIV || type == BLOCK_SPAN)
                        {
                            // CRITICAL FIX: Structural wrappers - don't create blocks
                            // but DO continue parsing their content (nested tags)
                            // Just silently ignore the wrapper tags themselves
                        }
                        else if (type != BLOCK_UNKNOWN)
                        {
                            // Other recognized tags
                            FinalizeAndAddBlock(tagName, attributes, result);
                        }
                        
                        tagName.clear();
                        attributes.clear();
                        currentState = STATE_DATA;
                    }
                    else if (c == '/')
                    {
                        isSelfClosing = true;
                    }
                    else if (!IsWhitespace(c) && IsAlpha(c))
                    {
                        attrKey.clear();
                        attrKey += ToLower(c);
                        currentState = STATE_ATTR_NAME;
                    }
                    // Skip other whitespace
                    break;
                }

                // ============================================================
                // STATE_ATTR_NAME: Reading Attribute Name
                // ============================================================
                case STATE_ATTR_NAME:
                {
                    if (IsAlphaNum(c) || c == '-' || c == '_')
                    {
                        attrKey += ToLower(c);
                    }
                    else if (IsWhitespace(c))
                    {
                        currentState = STATE_AFTER_ATTR_NAME;
                    }
                    else if (c == '=')
                    {
                        currentState = STATE_BEFORE_ATTR_VALUE;
                    }
                    else if (c == '>')
                    {
                        // Boolean attribute (no value)
                        attributes[attrKey] = "";
                        
                        // Tag complete - create and add block using helper (DRY principle)
                        FinalizeAndAddBlock(tagName, attributes, result);
                        tagName.clear();
                        attributes.clear();
                        currentState = STATE_DATA;
                    }
                    break;
                }

                // ============================================================
                // STATE_AFTER_ATTR_NAME: After Attribute Name
                // ============================================================
                case STATE_AFTER_ATTR_NAME:
                {
                    if (c == '=')
                    {
                        currentState = STATE_BEFORE_ATTR_VALUE;
                    }
                    else if (!IsWhitespace(c))
                    {
                        // Boolean attribute, start new attribute
                        attributes[attrKey] = "";
                        if (IsAlpha(c))
                        {
                            attrKey.clear();
                            attrKey += ToLower(c);
                            currentState = STATE_ATTR_NAME;
                        }
                        else if (c == '>')
                        {
                            // Tag complete - create and add block using helper (DRY principle)
                            FinalizeAndAddBlock(tagName, attributes, result);
                            tagName.clear();
                            attributes.clear();
                            currentState = STATE_DATA;
                        }
                    }
                    break;
                }

                // ============================================================
                // STATE_BEFORE_ATTR_VALUE: After '=' Before Value
                // ============================================================
                case STATE_BEFORE_ATTR_VALUE:
                {
                    if (c == '"')
                    {
                        attrValue.clear();
                        currentState = STATE_ATTR_VALUE_DOUBLE_QUOTED;
                    }
                    else if (c == '\'')
                    {
                        attrValue.clear();
                        currentState = STATE_ATTR_VALUE_SINGLE_QUOTED;
                    }
                    else if (!IsWhitespace(c) && c != '>')
                    {
                        attrValue.clear();
                        attrValue += c;
                        currentState = STATE_ATTR_VALUE_UNQUOTED;
                    }
                    else if (c == '>')
                    {
                        // Malformed: no value provided
                        attributes[attrKey] = "";
                        result.warnings.push_back("Malformed attribute: " + attrKey);
                        
                        // Tag complete - create and add block using helper (DRY principle)
                        FinalizeAndAddBlock(tagName, attributes, result);
                        tagName.clear();
                        attributes.clear();
                        currentState = STATE_DATA;
                    }
                    break;
                }

                // ============================================================
                // STATE_ATTR_VALUE_DOUBLE_QUOTED: In "value"
                // ============================================================
                case STATE_ATTR_VALUE_DOUBLE_QUOTED:
                {
                    if (c == '"')
                    {
                        attributes[attrKey] = attrValue;
                        currentState = STATE_BEFORE_ATTR_NAME;
                    }
                    else
                    {
                        attrValue += c;
                    }
                    break;
                }

                // ============================================================
                // STATE_ATTR_VALUE_SINGLE_QUOTED: In 'value'
                // ============================================================
                case STATE_ATTR_VALUE_SINGLE_QUOTED:
                {
                    if (c == '\'')
                    {
                        attributes[attrKey] = attrValue;
                        currentState = STATE_BEFORE_ATTR_NAME;
                    }
                    else
                    {
                        attrValue += c;
                    }
                    break;
                }

                // ============================================================
                // STATE_ATTR_VALUE_UNQUOTED: In unquoted value
                // ============================================================
                case STATE_ATTR_VALUE_UNQUOTED:
                {
                    if (IsWhitespace(c))
                    {
                        attributes[attrKey] = attrValue;
                        currentState = STATE_BEFORE_ATTR_NAME;
                    }
                    else if (c == '>')
                    {
                        attributes[attrKey] = attrValue;
                        
                        // Tag complete - create and add block using helper (DRY principle)
                        FinalizeAndAddBlock(tagName, attributes, result);
                        tagName.clear();
                        attributes.clear();
                        currentState = STATE_DATA;
                    }
                    else
                    {
                        attrValue += c;
                    }
                    break;
                }

                // ============================================================
                // STATE_COMMENT: In <!-- comment -->
                // ============================================================
                case STATE_COMMENT:
                {
                    // Look for '-->' to end comment
                    if (c == '-' && cursor + 1 < end && 
                        *cursor == '-' && *(cursor + 1) == '>')
                    {
                        cursor += 2; // Skip '->'
                        currentState = STATE_DATA;
                    }
                    // Otherwise keep skipping
                    break;
                }

                // ============================================================
                // STATE_SKIP_CONTENT: Skipping <script>/<style> Content
                // ============================================================
                case STATE_SKIP_CONTENT:
                {
                    if (c == '<')
                    {
                        // Check if closing tag
                        if (cursor < end && *cursor == '/')
                        {
                            cursor++;
                            std::string closeTag;
                            while (cursor < end && *cursor != '>')
                            {
                                closeTag += ToLower(*cursor);
                                cursor++;
                            }
                            if (cursor < end) cursor++; // Skip '>'
                            
                            if (closeTag == skipTagName)
                            {
                                skipDepth--;
                                if (skipDepth == 0)
                                {
                                    currentState = STATE_DATA;
                                    skipTagName.clear();
                                }
                            }
                        }
                        else
                        {
                            // Check if opening same tag (nested)
                            const char* peek = cursor;
                            std::string peekTag;
                            while (peek < end && IsAlphaNum(*peek))
                            {
                                peekTag += ToLower(*peek);
                                peek++;
                            }
                            if (peekTag == skipTagName)
                            {
                                skipDepth++;
                            }
                        }
                    }
                    break;
                }
            }
        }

        // ====================================================================
        // STEP 5: POST-LOOP CLEANUP
        // ====================================================================
        // Process any remaining text buffer
        if (!textBuffer.empty())
        {
            std::string cleaned = TrimAndDecode(textBuffer, result.warnings);
            if (!cleaned.empty())
            {
                HtmlBlock block(BLOCK_TEXT, cleaned);
                result.blocks.push_back(block);
            }
        }

        // Set success status
        result.status = PARSE_SUCCESS;
        result.errorMessage = "";

        return result;
    }

    // ========================================================================
    // GetBlockType - Map Tag Name to BlockType Enum
    // ========================================================================
    // WHY: Fast O(1) dispatch via if-else chain optimized by compiler
    // INPUT: Lowercase tag name (e.g., "p", "h1", "img")
    // OUTPUT: Corresponding BlockType or BLOCK_UNKNOWN
    BlockType HtmlParser::GetBlockType(const std::string& tagName) const
    {
        // Fast path: Check first character for common tags
        if (tagName.empty())
            return BLOCK_UNKNOWN;

        char firstChar = tagName[0];
        
        switch (firstChar)
        {
            case 'p':
                if (tagName == "p") return BLOCK_P;
                break;
            case 'h':
                if (tagName == "h1") return BLOCK_H1;
                if (tagName == "h2") return BLOCK_H2;
                if (tagName == "h3") return BLOCK_H3;
                if (tagName == "header") return BLOCK_DIV; // Semantic HTML5
                break;
            case 'a':
                if (tagName == "a") return BLOCK_A;
                if (tagName == "article") return BLOCK_DIV; // Semantic HTML5
                if (tagName == "aside") return BLOCK_DIV;   // Semantic HTML5
                break;
            case 'i':
                if (tagName == "img") return BLOCK_IMG;
                break;
            case 'b':
                if (tagName == "br") return BLOCK_BR;
                break;
            case 'u':
                if (tagName == "ul") return BLOCK_UL;
                break;
            case 'l':
                if (tagName == "li") return BLOCK_LI;
                break;
            case 'd':
                if (tagName == "div") return BLOCK_DIV;
                break;
            case 's':
                if (tagName == "section") return BLOCK_DIV; // Semantic HTML5
                if (tagName == "span") return BLOCK_SPAN;
                break;
            case 'f':
                if (tagName == "footer") return BLOCK_DIV; // Semantic HTML5
                break;
            case 'n':
                if (tagName == "nav") return BLOCK_DIV;    // Semantic HTML5
                break;
            case 'm':
                if (tagName == "main") return BLOCK_DIV;   // Semantic HTML5
                break;
        }

        return BLOCK_UNKNOWN;
    }

    // ========================================================================
    // ParseAttributes - DEPRECATED STUB (Not Used Internally)
    // ========================================================================
    // NOTE: This method is NOT used by Parse() - attribute parsing is fully
    // integrated into the FSM for performance. This stub exists only for API
    // compatibility. If you need to parse attributes externally, you should
    // use Parse() instead which returns HtmlBlock objects with pre-parsed
    // attributes.
    std::map<std::string, std::string> HtmlParser::ParseAttributes(
        const std::string& /*attrStr*/) const
    {
        // Return empty map - this function is deprecated and not implemented
        // All attribute parsing happens inline in the Parse() FSM
        return std::map<std::string, std::string>();
    }

    // ========================================================================
    // TrimAndDecode - Whitespace Normalization and Entity Decoding
    // ========================================================================
    // WHY: Single-pass optimization for Win98 performance
    // OPERATIONS:
    //   1. Collapse multiple whitespace to single space
    //   2. Trim leading/trailing whitespace
    //   3. Decode common HTML entities (&amp; &lt; &gt; &quot; &#39;)
    // INPUT: Raw text with possible entities and excess whitespace
    // OUTPUT: Clean, decoded text ready for rendering
    std::string HtmlParser::TrimAndDecode(const std::string& text) const
    {
        if (text.empty())
            return "";

        std::string result;
        result.reserve(text.size()); // Pre-allocate

        bool inWhitespace = false;
        bool hasContent = false;
        size_t i = 0;
        size_t len = text.length();

        // Single-pass: decode entities and normalize whitespace
        while (i < len)
        {
            char c = text[i];

            // Handle HTML entities
            if (c == '&' && i + 2 < len)
            {
                // Check for common entities
                if (text[i + 1] == 'a' && i + 4 < len &&
                    text[i + 2] == 'm' && text[i + 3] == 'p' && text[i + 4] == ';')
                {
                    // &amp; -> &
                    result += '&';
                    i += 5;
                    hasContent = true;
                    inWhitespace = false;
                    continue;
                }
                else if (text[i + 1] == 'l' && i + 3 < len &&
                         text[i + 2] == 't' && text[i + 3] == ';')
                {
                    // &lt; -> 
                    result += '<';
                    i += 4;
                    hasContent = true;
                    inWhitespace = false;
                    continue;
                }
                else if (text[i + 1] == 'g' && i + 3 < len &&
                         text[i + 2] == 't' && text[i + 3] == ';')
                {
                    // &gt; -> >
                    result += '>';
                    i += 4;
                    hasContent = true;
                    inWhitespace = false;
                    continue;
                }
                else if (text[i + 1] == 'q' && i + 5 < len &&
                         text[i + 2] == 'u' && text[i + 3] == 'o' &&
                         text[i + 4] == 't' && text[i + 5] == ';')
                {
                    // &quot; -> "
                    result += '"';
                    i += 6;
                    hasContent = true;
                    inWhitespace = false;
                    continue;
                }
                else if (text[i + 1] == '#' && i + 3 < len)
                {
                    // Numeric entity: &#39; or &#x27;
                    size_t j = i + 2;
                    bool isHex = false;
                    if (text[j] == 'x' || text[j] == 'X')
                    {
                        isHex = true;
                        j++;
                    }
                    
                    int charCode = 0;
                    bool validEntity = false;
                    
                    while (j < len && text[j] != ';')
                    {
                        char digit = text[j];
                        if (isHex)
                        {
                            if (digit >= '0' && digit <= '9')
                                charCode = charCode * 16 + (digit - '0');
                            else if (digit >= 'a' && digit <= 'f')
                                charCode = charCode * 16 + (digit - 'a' + 10);
                            else if (digit >= 'A' && digit <= 'F')
                                charCode = charCode * 16 + (digit - 'A' + 10);
                            else
                                break;
                        }
                        else
                        {
                            if (digit >= '0' && digit <= '9')
                                charCode = charCode * 10 + (digit - '0');
                            else
                                break;
                        }
                        j++;
                        validEntity = true;
                    }
                    
                    if (validEntity && j < len && text[j] == ';' && charCode > 0 && charCode < 128)
                    {
                        result += static_cast<char>(charCode);
                        i = j + 1;
                        hasContent = true;
                        inWhitespace = false;
                        continue;
                    }
                }
            }

            // Handle whitespace normalization
            if (IsWhitespace(c))
            {
                if (hasContent && !inWhitespace)
                {
                    result += ' ';
                    inWhitespace = true;
                }
                i++;
            }
            else
            {
                result += c;
                hasContent = true;
                inWhitespace = false;
                i++;
            }
        }

        // Trim trailing whitespace
        while (!result.empty() && IsWhitespace(result[result.length() - 1]))
        {
            result.erase(result.length() - 1);
        }

        return result;
    }

    // ========================================================================
    // TrimAndDecode (Overloaded) - With Unknown Entity Detection
    // ========================================================================
    // WHY: Single-pass optimization - detects unknown entities while decoding
    // OPERATIONS: Same as above + logs warnings for unknown entities
    // BENEFIT: Eliminates separate DetectUnknownEntities() scan pass
    std::string HtmlParser::TrimAndDecode(const std::string& text, std::vector<std::string>& warnings) const
    {
        if (text.empty())
            return "";

        std::string result;
        result.reserve(text.size());

        bool inWhitespace = false;
        bool hasContent = false;
        size_t i = 0;
        size_t len = text.length();

        while (i < len)
        {
            char c = text[i];

            // Handle HTML entities
            if (c == '&' && i + 2 < len)
            {
                bool knownEntity = false;

                // Check &amp;
                if (text[i + 1] == 'a' && i + 4 < len &&
                    text[i + 2] == 'm' && text[i + 3] == 'p' && text[i + 4] == ';')
                {
                    result += '&';
                    i += 5;
                    knownEntity = true;
                }
                // Check &lt;
                else if (text[i + 1] == 'l' && i + 3 < len &&
                         text[i + 2] == 't' && text[i + 3] == ';')
                {
                    result += '<';
                    i += 4;
                    knownEntity = true;
                }
                // Check &gt;
                else if (text[i + 1] == 'g' && i + 3 < len &&
                         text[i + 2] == 't' && text[i + 3] == ';')
                {
                    result += '>';
                    i += 4;
                    knownEntity = true;
                }
                // Check &quot;
                else if (text[i + 1] == 'q' && i + 5 < len &&
                         text[i + 2] == 'u' && text[i + 3] == 'o' &&
                         text[i + 4] == 't' && text[i + 5] == ';')
                {
                    result += '"';
                    i += 6;
                    knownEntity = true;
                }
                // Check numeric entities &#...; or &#x...;
                else if (text[i + 1] == '#' && i + 3 < len)
                {
                    size_t j = i + 2;
                    bool isHex = false;
                    if (text[j] == 'x' || text[j] == 'X')
                    {
                        isHex = true;
                        j++;
                    }
                    
                    int charCode = 0;
                    bool validEntity = false;
                    
                    while (j < len && text[j] != ';')
                    {
                        char digit = text[j];
                        if (isHex)
                        {
                            if (digit >= '0' && digit <= '9')
                                charCode = charCode * 16 + (digit - '0');
                            else if (digit >= 'a' && digit <= 'f')
                                charCode = charCode * 16 + (digit - 'a' + 10);
                            else if (digit >= 'A' && digit <= 'F')
                                charCode = charCode * 16 + (digit - 'A' + 10);
                            else
                                break;
                        }
                        else
                        {
                            if (digit >= '0' && digit <= '9')
                                charCode = charCode * 10 + (digit - '0');
                            else
                                break;
                        }
                        j++;
                        validEntity = true;
                    }
                    
                    if (validEntity && j < len && text[j] == ';' && charCode > 0 && charCode < 128)
                    {
                        result += static_cast<char>(charCode);
                        i = j + 1;
                        knownEntity = true;
                    }
                }

                // If unknown entity, log warning and keep as-is
                if (!knownEntity)
                {
                    // Extract entity for warning message
                    size_t entityEnd = i + 1;
                    while (entityEnd < len && entityEnd < i + 20 &&
                           text[entityEnd] != ';' && !IsWhitespace(text[entityEnd]))
                    {
                        entityEnd++;
                    }
                    
                    // Only warn if it looks like an entity (has content)
                    if (entityEnd > i + 1 && entityEnd <= i + 20)
                    {
                        std::string entityName = text.substr(i, entityEnd - i + 
                            (entityEnd < len && text[entityEnd] == ';' ? 1 : 0));
                        warnings.push_back("Unknown HTML entity: " + entityName + " (kept as-is)");
                    }
                    
                    // Keep the & as-is in output
                    result += c;
                    i++;
                }

                if (knownEntity)
                {
                    hasContent = true;
                    inWhitespace = false;
                    continue;
                }
            }

            // Handle whitespace normalization
            if (IsWhitespace(c))
            {
                if (hasContent && !inWhitespace)
                {
                    result += ' ';
                    inWhitespace = true;
                }
                i++;
            }
            else
            {
                result += c;
                hasContent = true;
                inWhitespace = false;
                i++;
            }
        }

        // Trim trailing whitespace
        while (!result.empty() && IsWhitespace(result[result.length() - 1]))
        {
            result.erase(result.length() - 1);
        }

        return result;
    }

    // ========================================================================
    // STATIC HELPER FUNCTIONS
    // ========================================================================

    static inline bool IsWhitespace(char c)
    {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r';
    }

    static inline bool IsAlpha(char c)
    {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }

    static inline bool IsAlphaNum(char c)
    {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
               (c >= '0' && c <= '9');
    }

    static inline char ToLower(char c)
    {
        if (c >= 'A' && c <= 'Z')
            return c + ('a' - 'A');
        return c;
    }

    static bool IsOnlyWhitespace(const std::vector<char>& data)
    {
        for (size_t i = 0; i < data.size(); ++i)
        {
            if (!IsWhitespace(data[i]))
                return false;
        }
        return true;
    }

    // ========================================================================
    // NON-COPYABLE PATTERN (C++98) - Prevents Accidental Copying
    // ========================================================================
    // WHY: The HtmlParser is designed to be a lightweight, stateless utility
    // that should be instantiated once and reused. Copying serves no purpose
    // and would confuse API users. This explicit private declaration with
    // empty implementation is the C++98 idiom to disable copy semantics.
    // (In C++11+, we would use "= delete" instead.)
    HtmlParser::HtmlParser(const HtmlParser&)
    {
        // Intentionally left empty and private to prevent copying
    }

    HtmlParser& HtmlParser::operator=(const HtmlParser&)
    {
        // Intentionally left empty and private to prevent assignment
        return *this;
    }

} // namespace Parser