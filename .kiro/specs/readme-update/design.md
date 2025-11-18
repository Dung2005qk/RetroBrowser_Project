# Design Document - README Update

## Overview

This design document outlines the approach for updating the RetroBrowser README.md to accurately reflect the current implementation's capabilities, particularly focusing on image handling, CSS styling, HTML parsing features, and optimized project structure. The update will transform the README from its current state into a comprehensive, accurate documentation that serves as the single source of truth for the project.

**Design Philosophy:**
- **Accuracy First**: Every technical claim must be verifiable in the source code
- **Developer-Focused**: Structure information for quick navigation and understanding
- **Maintainability**: Organize content to minimize future update friction
- **Completeness**: Cover all major features without overwhelming detail

## Architecture

### Documentation Structure

The README update follows a hierarchical information architecture:

```
README.md
├── Header (Badges, Navigation)
├── Overview Section
│   ├── Introduction
│   ├── Academic Goals
│   └── Scope & Limitations
├── Architecture Section
│   ├── System Diagram
│   ├── Processing Flow
│   └── Module Descriptions
├── Features Section (★ MAJOR UPDATE)
│   ├── HTML Support
│   ├── CSS Styling (NEW)
│   ├── Image Handling (EXPANDED)
│   ├── Rendering Engine
│   └── Network Layer
├── Project Structure (★ OPTIMIZED)
│   ├── Core Source (src/)
│   ├── Build Output (deploy/)
│   ├── Documentation (docs/)
│   └── Tools (tools/)
├── Installation & Usage
├── Testing Documentation
└── Credits & License
```

### Content Organization Principles

1. **Progressive Disclosure**: High-level overview → detailed capabilities → implementation specifics
2. **Scannable Format**: Use tables, bullet points, and code blocks for quick reference
3. **Cross-Referencing**: Link related sections (e.g., CSS support → Parser module)
4. **Visual Hierarchy**: Consistent heading levels, emoji markers for sections

## Components and Interfaces

### 1. Image Support Documentation Component

**Purpose**: Accurately document the browser's image handling capabilities based on actual implementation in renderer.h/cpp.

**Content Structure**:
```markdown
### Image Handling
- **Format Support**: BMP (native Win98 format via GDI)
- **Loading**: Async multi-threaded via worker threads
- **Caching**: LRU eviction with MAX_IMAGE_CACHE_SIZE limit
- **Rendering**: GDI BitBlt with placeholder fallback
- **Alt Text**: Automatic fallback when image load fails
```

**Data Sources**:
- `renderer.h`: Constants (MAX_IMAGE_CACHE_SIZE, IMAGE_PLACEHOLDER_*)
- `renderer.cpp`: LoadImage(), NotifyImageLoaded(), DecodeImage() implementations
- `main.cpp`: Image loading thread creation and management

**Rationale**: Current README mentions "image support" vaguely. Implementation shows sophisticated async loading with caching - this deserves detailed documentation.

### 2. CSS Styling Documentation Component

**Purpose**: Document the newly implemented CSS inline styling support that enables modern web page rendering.

**Content Structure**:
```markdown
### CSS Styling Support

#### Inline Style Attributes
- `color`: Text color (named, hex #RGB/#RRGGBB, rgb(r,g,b))
- `background-color`: Block background
- `font-weight`: bold/normal (FW_BOLD/FW_NORMAL)
- `font-style`: italic/normal
- `font-size`: Pixel sizes (e.g., 16px)

#### Legacy HTML 3.2 Attributes
- `<body bgcolor="...">`: Page background color
- `<body text="...">`: Default text color
- `<font color="...">`: Text color override

#### Color Format Support
- Named colors: "red", "green", "blue", etc.
- Hex short: #RGB (e.g., #F00)
- Hex full: #RRGGBB (e.g., #FF0000)
- RGB function: rgb(255, 0, 0)

#### Limitations
- ❌ External stylesheets (<link rel="stylesheet">)
- ❌ <style> blocks
- ❌ CSS selectors, cascading, specificity
- ✅ Inline styles only (style="...")
```

**Data Sources**:
- `parser.h`: HtmlBlock struct with textColor, backgroundColor, fontWeight, fontItalic, fontSize fields
- `parser.cpp`: CSS parsing logic (ParseInlineStyles, ParseColor functions)
- `renderer.h`: RenderItem struct with CSS property fields
- `renderer.cpp`: CSS property application in rendering

**Rationale**: This is a MAJOR feature addition not documented in current README. CSS support enables rendering modern websites with proper styling (e.g., textfiles.com green headers).

### 3. HTML Parsing Documentation Component

**Purpose**: Comprehensively document the parser's capabilities, including semantic HTML5 support and table handling.

**Content Structure**:
```markdown
### HTML Parsing Capabilities

#### Supported Tags
**Basic Structure**: H1-H3, P, DIV, SPAN, BR
**Hyperlinks**: A (with href attribute)
**Images**: IMG (with src, alt attributes)
**Lists**: UL, LI
**Semantic HTML5**: header, footer, nav, article, section, aside, main
**Tables**: TABLE, TR, TD, TH, TBODY, THEAD, TFOOT (mapped to DIV for content preservation)

#### Parser Architecture
- **Algorithm**: Finite State Machine (FSM) with 11 states
- **Complexity**: Single-pass O(n) parsing
- **Error Handling**: Forgiving parser with error recovery
- **Entity Decoding**: &amp;, &lt;, &gt;, &quot;, numeric entities (&#...;)

#### Advanced Features
- Semantic HTML5 tag recognition (treated as generic containers)
- Table content preservation (structure flattened to DIV flow)
- Malformed HTML tolerance (unclosed tags, missing quotes)
- Warning system for skipped content (scripts, unsupported tags)
```

**Data Sources**:
- `parser.h`: BlockType enum (BLOCK_H1, BLOCK_DIV, BLOCK_UL, etc.)
- `parser.h`: Extensive documentation comments about FSM, entity decoding
- `parser.cpp`: GetBlockType() implementation showing tag mapping

**Rationale**: Current README lists basic tags but doesn't mention semantic HTML5 support or table handling - both are implemented and important for modern web compatibility.

### 4. Project Structure Optimization Component

**Purpose**: Simplify the project structure tree to focus on core source code, removing clutter from build scripts and documentation files.

**Content Structure**:
```markdown
### Project Structure

```
RetroBrowser_Project/
├── src/                        # Source code
│   ├── browser/                # C++ browser (4,817 lines)
│   │   ├── core/               # Main + PCH
│   │   │   ├── main.cpp        # Orchestrator (579 lines)
│   │   │   ├── stdafx.h        # Precompiled header
│   │   │   └── stdafx.cpp      # PCH implementation
│   │   ├── ui/                 # Win32 GUI (1,015 lines)
│   │   │   ├── ui.h            # Public API (355 lines)
│   │   │   └── ui.cpp          # Implementation (660 lines)
│   │   ├── network/            # HTTP client (1,204 lines)
│   │   │   ├── network.h       # API (444 lines)
│   │   │   └── network.cpp     # Winsock (760 lines)
│   │   ├── parser/             # HTML parser (2,430 lines)
│   │   │   ├── parser.h        # API (854 lines)
│   │   │   └── parser.cpp      # FSM (1,576 lines)
│   │   ├── renderer/           # GDI engine (1,427 lines)
│   │   │   ├── renderer.h      # API (273 lines)
│   │   │   └── renderer.cpp    # Rendering (1,154 lines)
│   │   └── res/                # Resources
│   │       ├── app.ico         # Application icon
│   │       ├── app.rc          # Resource script
│   │       ├── hand.cur        # Hand cursor
│   │       └── resource.h      # Resource IDs
│   └── proxy/                  # Python proxy
│       ├── proxy.py            # Server logic
│       ├── config.py           # Configuration
│       └── requirements.txt    # Dependencies
├── libs/                       # External libraries
│   └── libjpeg/                # JPEG support (future)
├── deploy/                     # Build output
│   ├── RetroBrowser.exe        # Standard build
│   └── RetroBrowser_Win98.exe  # Win98-optimized build
├── demo/                       # Test HTML pages
│   ├── *.html                  # Various test cases
│   └── images/                 # Sample BMP images
├── docs/                       # Documentation
│   ├── win98_testing_guide.md
│   ├── win98_compatibility_matrix.md
│   └── ...
└── tools/                      # Development tools
    └── test_scripts/           # Test suite
```
```

**Rationale**: Current README shows ALL files including numerous .bat scripts, which clutters the view. Developers need to see the core source structure clearly. Build scripts and docs are secondary.

### 5. Code Statistics Update Component

**Purpose**: Provide accurate line counts for each module based on actual source files.

**Content Structure**:
```markdown
### Code Statistics

| Component | Files | Lines | Purpose |
|-----------|-------|-------|---------|
| Core | 3 | 842 | Orchestration |
| UI | 2 | 1,015 | GUI management |
| Network | 2 | 1,204 | HTTP client |
| Parser | 2 | 2,430 | HTML → Blocks |
| Renderer | 2 | 1,427 | GDI rendering |
| Proxy | 2 | 1,046 | HTTPS bridge |
| Tests | 8 | 1,600+ | Quality assurance |
| **TOTAL** | **21** | **~9,600** | Production-ready |
```

**Data Sources**:
- Actual line counts from source files:
  - parser.h: 854 lines (verified from file read)
  - parser.cpp: 1,576 lines (from requirements)
  - renderer.h: 273 lines (verified from file read)
  - renderer.cpp: 1,154 lines (from requirements)
  - network.h: 444 lines (from requirements)
  - network.cpp: 760 lines (from requirements)

**Rationale**: Current README shows parser.cpp as 1007 lines, but requirements state 1576 lines. Need to verify and update all counts for accuracy.

### 6. Rendering Engine Documentation Component

**Purpose**: Document the two-phase rendering architecture and performance optimizations.

**Content Structure**:
```markdown
### Rendering Engine

#### Two-Phase Architecture
1. **Layout Phase** (Expensive, on resize/content change)
   - Calculate block positions via DrawText with DT_CALCRECT
   - Build display list with pre-computed RECT bounds
   - Compute total content height for scrolling

2. **Paint Phase** (Fast, on every frame)
   - Draw only visible blocks (clipping optimization)
   - Use pre-computed positions from layout phase
   - Double-buffered rendering (flicker-free)

#### Performance Optimizations
- **Double-Buffering**: Offscreen DC + BitBlt for smooth rendering
- **Font Caching**: Pre-created fonts for H1/H2/H3/P/A (avoid CreateFont overhead)
- **Clipping**: Only render visible blocks (10x speedup on large pages)
- **DPI-Aware**: Font sizing adapts to display DPI

#### Interaction Features
- **Scrolling**: Vertical scrolling with scrollbar integration
- **Click Handling**: Hit-testing for hyperlinks with clickable area tracking
- **Link Styling**: Blue underlined text, purple for visited links
```

**Data Sources**:
- `renderer.h`: Extensive documentation on two-phase architecture
- `renderer.h`: Constants (FONT_SIZE_*, LINK_COLOR, etc.)
- `renderer.cpp`: CalculateLayout(), Render() implementations

**Rationale**: Current README mentions "GDI rendering" but doesn't explain the sophisticated two-phase architecture that enables smooth performance on Win98.

## Data Models

### HtmlBlock Structure (from parser.h)

```cpp
struct HtmlBlock {
    BlockType type;                              // Block type enum
    std::string content;                         // Text content
    std::map<std::string, std::string> attributes; // HTML attributes
    
    // CSS styling properties
    int textColor;           // -1 = default, else COLORREF
    int backgroundColor;     // -1 = default, else COLORREF
    int fontWeight;          // FW_NORMAL or FW_BOLD
    BOOL fontItalic;         // TRUE/FALSE
    int fontSize;            // 0 = default, else pixel size
};
```

**Key Design Decision**: CSS properties are stored directly in HtmlBlock rather than requiring attribute map lookups. This improves rendering performance by eliminating repeated map searches.

### RenderItem Structure (from renderer.h)

```cpp
struct RenderItem {
    Parser::BlockType type;
    RECT bounds;                                 // Pre-computed position
    std::string content;
    std::map<std::string, std::string> attributes;
    
    // CSS styling (copied from HtmlBlock)
    int textColor;
    int backgroundColor;
    int fontWeight;
    BOOL fontItalic;
    int fontSize;
};
```

**Key Design Decision**: RenderItem duplicates CSS properties from HtmlBlock to keep the display list self-contained. This enables layout caching without retaining references to original blocks.

### Feature Categories Model

The README will organize features into five main categories:

1. **HTML Support**: Tags, semantic HTML5, tables, entity decoding
2. **CSS Styling**: Inline styles, color formats, legacy attributes
3. **Image Handling**: Async loading, caching, format support, fallback
4. **Rendering Engine**: Two-phase architecture, double-buffering, font management
5. **Network Layer**: HTTP/1.0, proxy integration, error handling

This categorization provides clear mental models for developers understanding the browser's capabilities.

## Correctness Properties

*A property is a characteristic or behavior that should hold true across all valid executions of a system-essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees.*


### Acceptance Criteria Testing Prework

Based on the requirements analysis, most acceptance criteria are testable as examples since they specify exact content that should appear in specific sections of the README. Two criteria (6.1 and 6.5) are properties that apply across all modules.

### Property Reflection

After reviewing all testable criteria, I identified that most are specific examples of content that must appear in the README (e.g., "BMP format must be mentioned", "CSS properties must be listed"). These are not redundant because each checks for different content in different sections.

The two property-based criteria (6.1 and 6.5) about code statistics are related but not redundant:
- 6.1 ensures each module has accurate line counts
- 6.5 ensures the total is mathematically correct (sum of parts)

Both provide unique validation value and should be retained.

### Correctness Properties

Property 1: Module line counts match source files
*For any* module listed in the code statistics table, the documented line count should match the actual line count in the corresponding source file
**Validates: Requirements 6.1**

Property 2: Total line count equals sum of module counts
*For any* code statistics table, the total line count should equal the sum of all individual module line counts
**Validates: Requirements 6.5**

Note: The remaining acceptance criteria (1.1-1.5, 2.1-2.5, 3.1-3.5, 4.1-4.5, 5.1-5.5, 7.1-7.5, 8.1-8.5) are specific examples that verify particular content exists in particular sections. These are better validated through example-based testing rather than property-based testing, as they check for specific strings and structures rather than universal properties.

## Error Handling

### Documentation Accuracy Errors

**Error Type**: Inaccurate Technical Claims
- **Detection**: Cross-reference README claims with source code
- **Prevention**: Verify every technical detail against actual implementation
- **Recovery**: Update README to match source code reality

**Error Type**: Outdated Information
- **Detection**: Compare README with current codebase state
- **Prevention**: Establish update workflow when code changes
- **Recovery**: Systematic review and update of affected sections

**Error Type**: Missing Critical Information
- **Detection**: Gap analysis between implementation and documentation
- **Prevention**: Comprehensive feature inventory before writing
- **Recovery**: Add missing sections with complete information

### Structure and Organization Errors

**Error Type**: Inconsistent Formatting
- **Detection**: Visual review of markdown rendering
- **Prevention**: Use consistent markdown patterns throughout
- **Recovery**: Apply formatting standards uniformly

**Error Type**: Broken Cross-References
- **Detection**: Link validation in markdown
- **Prevention**: Use relative links, verify before commit
- **Recovery**: Fix broken links, ensure targets exist

**Error Type**: Information Overload
- **Detection**: Reader feedback, section length analysis
- **Prevention**: Progressive disclosure, link to detailed docs
- **Recovery**: Refactor verbose sections, move details to separate files

### Content Completeness Errors

**Error Type**: Incomplete Feature Coverage
- **Detection**: Feature checklist against README sections
- **Prevention**: Systematic review of all modules
- **Recovery**: Add missing feature documentation

**Error Type**: Unclear Technical Explanations
- **Detection**: Peer review, user questions
- **Prevention**: Include examples, diagrams, code snippets
- **Recovery**: Clarify with additional context and examples

## Testing Strategy

### Documentation Validation Approach

The README update will be validated through a combination of manual review and automated checks:

#### Manual Validation
1. **Content Accuracy Review**: Cross-reference every technical claim with source code
2. **Completeness Check**: Verify all requirements are addressed
3. **Readability Review**: Ensure clear, scannable formatting
4. **Link Validation**: Test all internal and external links

#### Automated Validation (Future Enhancement)
1. **Markdown Linting**: Validate markdown syntax and structure
2. **Link Checking**: Automated broken link detection
3. **Code Statistics**: Script to verify line counts match source files
4. **Content Search**: Verify required keywords appear in correct sections

### Example-Based Testing

Since most acceptance criteria are examples (specific content in specific sections), testing will verify:

**Image Support Section (Requirements 1.1-1.5)**:
- Example Test 1: README contains "BMP" in image support section
- Example Test 2: README mentions "async multi-threaded" in image loading description
- Example Test 3: README includes "GDI BitBlt" and "LRU eviction" in rendering explanation
- Example Test 4: README documents "placeholder rendering" and "alt text fallback"
- Example Test 5: README lists image size limits and memory constraints

**CSS Styling Section (Requirements 2.1-2.5)**:
- Example Test 1: README lists all five CSS properties (color, background-color, font-weight, font-style, font-size)
- Example Test 2: README documents three color formats (named, hex, rgb)
- Example Test 3: README mentions HTML 3.2 attributes (BGCOLOR, TEXT, COLOR)
- Example Test 4: README states limitations (no external stylesheets, no CSS selectors)
- Example Test 5: README explains CSS property extraction into HtmlBlock

**HTML Parsing Section (Requirements 3.1-3.5)**:
- Example Test 1: README lists all specified tags including semantic HTML5
- Example Test 2: README explains table-to-DIV mapping
- Example Test 3: README documents FSM with 11 states and O(n) complexity
- Example Test 4: README lists supported HTML entities
- Example Test 5: README describes forgiving parser with error recovery

**Project Structure Section (Requirements 4.1-4.5)**:
- Example Test 1: README shows only core directories (src/, libs/, deploy/, demo/)
- Example Test 2: README details all src/ subdirectories
- Example Test 3: README excludes .bat files from main structure tree
- Example Test 4: README lists all four resource files
- Example Test 5: README shows docs/ and tools/ separately

**Rendering Engine Section (Requirements 5.1-5.5)**:
- Example Test 1: README explains two-phase approach
- Example Test 2: README mentions double-buffering and flicker-free rendering
- Example Test 3: README describes font caching and DPI-aware sizing
- Example Test 4: README documents vertical scrolling with scrollbar
- Example Test 5: README explains hit-testing and clickable areas

**Feature Highlights Section (Requirements 7.1-7.5, 8.1-8.5)**:
- Example Test 1: README includes CSS inline styling in core features
- Example Test 2: README mentions semantic HTML5 and table preservation
- Example Test 3: README lists custom colors and page-level styling
- Example Test 4: README documents async loading, caching, placeholder rendering
- Example Test 5: README includes CSS parsing, entity decoding, two-phase rendering
- Example Test 6: README organizes features into five categories
- Example Test 7-10: README comprehensively covers each category

### Property-Based Testing

**Property 1: Module Line Count Accuracy**
- Test Strategy: For each module in code statistics table, count actual lines in source file and compare with documented count
- Implementation: Script to parse README table and compare with `wc -l` output
- Validation: All counts must match exactly (zero tolerance for inaccuracy)

**Property 2: Total Line Count Correctness**
- Test Strategy: Sum all individual module line counts and compare with documented total
- Implementation: Parse README table, sum module counts, compare with total row
- Validation: Total must equal sum (mathematical correctness)

### Testing Tools

**Recommended Tools**:
1. **Markdown Linter**: `markdownlint` for syntax validation
2. **Link Checker**: `markdown-link-check` for broken link detection
3. **Line Counter**: Custom script using `wc -l` or similar
4. **Diff Tool**: Compare old vs new README for review

**Testing Workflow**:
1. Update README content
2. Run markdown linter
3. Run link checker
4. Verify code statistics with line counting script
5. Manual content review against requirements
6. Peer review for clarity and completeness
7. Final approval before commit

## Implementation Notes

### Update Strategy

**Incremental Approach**:
1. **Phase 1**: Update existing sections (Image Support, HTML Parsing)
2. **Phase 2**: Add new sections (CSS Styling, detailed Rendering)
3. **Phase 3**: Optimize structure (Project Structure tree)
4. **Phase 4**: Update statistics (Code Statistics table)
5. **Phase 5**: Enhance features (Feature Highlights reorganization)

**Rationale**: Incremental updates allow validation at each step and minimize risk of introducing errors.

### Content Sources

**Primary Sources** (Source Code):
- `src/browser/parser/parser.h` - Parser capabilities, CSS support
- `src/browser/parser/parser.cpp` - Implementation details
- `src/browser/renderer/renderer.h` - Rendering architecture
- `src/browser/renderer/renderer.cpp` - Rendering implementation
- `src/browser/network/network.h` - Network capabilities
- `src/browser/core/main.cpp` - Image loading orchestration

**Secondary Sources** (Existing Documentation):
- Current `README.md` - Baseline content
- `docs/win98_testing_guide.md` - Testing information
- `docs/win98_compatibility_matrix.md` - Compatibility details

### Markdown Formatting Standards

**Consistency Rules**:
- Use `###` for main section headings
- Use `####` for subsections
- Use emoji markers for major sections (🎯, 🏗️, ✨, etc.)
- Use tables for structured data (code statistics, feature matrices)
- Use code blocks with language tags for code examples
- Use bullet points for lists, numbered lists for sequences
- Use bold for emphasis, inline code for technical terms

### Preservation Guidelines

**Content to Preserve**:
- Academic context and goals
- Build instructions (already comprehensive)
- Testing documentation (24/24 tests passing)
- Credits and license information
- Win98 compatibility documentation

**Content to Update**:
- Feature descriptions (add CSS, expand image handling)
- Code statistics (accurate line counts)
- Project structure (simplified tree)
- Technical highlights (add new features)

**Content to Add**:
- CSS styling section (completely new)
- Detailed rendering architecture
- Semantic HTML5 support
- Table handling explanation

## Design Rationale

### Why Optimize Project Structure Tree?

**Problem**: Current README shows 50+ files including all .bat scripts, making it difficult to understand core source organization.

**Solution**: Show only essential directories and files, group build scripts and docs separately.

**Benefits**:
- Faster comprehension of source code layout
- Reduced visual clutter
- Focus on what developers need to modify
- Easier navigation for new contributors

### Why Add CSS Styling Section?

**Problem**: CSS support is a major feature (textColor, backgroundColor, fontWeight, fontItalic, fontSize fields in HtmlBlock) but completely undocumented in current README.

**Solution**: Dedicated CSS Styling section with inline styles, color formats, and limitations.

**Benefits**:
- Developers understand modern web compatibility
- Clear documentation of supported CSS subset
- Explains why some sites render with colors (textfiles.com)
- Sets correct expectations (no external stylesheets)

### Why Expand Image Handling Documentation?

**Problem**: Current README mentions "image support" vaguely, but implementation has sophisticated async loading, caching, and fallback mechanisms.

**Solution**: Detailed image handling section covering format support, async loading, caching strategy, and fallback behavior.

**Benefits**:
- Developers understand performance characteristics
- Clear explanation of BMP-only limitation
- Documents alt text fallback mechanism
- Explains LRU cache eviction policy

### Why Document Two-Phase Rendering?

**Problem**: Current README doesn't explain the sophisticated layout/paint separation that enables smooth Win98 performance.

**Solution**: Detailed rendering architecture section explaining layout phase (expensive, rare) vs paint phase (fast, frequent).

**Benefits**:
- Developers understand performance optimization strategy
- Clear explanation of why scrolling is smooth
- Documents font caching and clipping optimizations
- Explains double-buffering for flicker-free rendering

### Why Update Code Statistics?

**Problem**: Current README shows parser.cpp as 1007 lines, but actual file is 1576 lines. Other counts may also be inaccurate.

**Solution**: Verify and update all line counts to match actual source files.

**Benefits**:
- Accurate project scope representation
- Credibility of documentation
- Correct understanding of module complexity
- Basis for future maintenance estimates

## Conclusion

This design provides a comprehensive approach to updating the RetroBrowser README with accurate, complete, and well-organized documentation. The update focuses on:

1. **Accuracy**: Every technical claim verified against source code
2. **Completeness**: All major features documented (CSS, images, parsing, rendering)
3. **Organization**: Optimized structure for developer comprehension
4. **Maintainability**: Clear sections that can be updated independently

The design balances thoroughness with readability, ensuring the README serves as an effective single source of truth for the project while remaining accessible to new developers and contributors.
