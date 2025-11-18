# Requirements Document - README Update

## Introduction

Dự án RetroBrowser là một trình duyệt web retro cho Windows 98 với nhiều tính năng nâng cao đã được phát triển. README hiện tại cần được cập nhật để phản ánh chính xác các cải tiến về xử lý hình ảnh, CSS styling, HTML parsing và các tính năng khác. Đồng thời cần tối ưu cấu trúc thư mục để tập trung vào code chính.

## Glossary

- **RetroBrowser**: Trình duyệt web retro chạy trên Windows 98
- **Parser Module**: Module phân tích HTML thành các block có cấu trúc
- **Renderer Module**: Module vẽ nội dung HTML lên màn hình bằng GDI
- **Network Module**: Module xử lý kết nối HTTP qua Winsock
- **CSS Styling**: Hỗ trợ inline CSS styles và legacy HTML 3.2 color attributes
- **Image Support**: Khả năng xử lý và hiển thị hình ảnh
- **Project Structure**: Cấu trúc thư mục của dự án

## Requirements

### Requirement 1

**User Story:** Là một developer đọc README, tôi muốn thấy thông tin chính xác về khả năng xử lý hình ảnh, để tôi hiểu được các định dạng ảnh được hỗ trợ.

#### Acceptance Criteria

1. WHEN README mô tả image support THEN hệ thống SHALL liệt kê BMP là định dạng chính được hỗ trợ native trên Win98
2. WHEN README đề cập đến image loading THEN hệ thống SHALL mô tả cơ chế async multi-threaded image loading
3. WHEN README giải thích image rendering THEN hệ thống SHALL nêu rõ việc sử dụng GDI BitBlt và image caching với LRU eviction
4. WHEN README mô tả image features THEN hệ thống SHALL đề cập đến placeholder rendering và alt text fallback
5. WHEN README liệt kê limitations THEN hệ thống SHALL nêu rõ giới hạn kích thước ảnh và memory constraints trên Win98

### Requirement 2

**User Story:** Là một developer đọc README, tôi muốn hiểu rõ khả năng CSS styling của browser, để tôi biết được mức độ hỗ trợ CSS.

#### Acceptance Criteria

1. WHEN README mô tả CSS support THEN hệ thống SHALL liệt kê inline style attributes được hỗ trợ (color, background-color, font-weight, font-style, font-size)
2. WHEN README giải thích color parsing THEN hệ thống SHALL mô tả các format được hỗ trợ (named colors, hex #RGB/#RRGGBB, rgb(r,g,b))
3. WHEN README đề cập legacy HTML THEN hệ thống SHALL nêu rõ hỗ trợ HTML 3.2 attributes (BGCOLOR, TEXT, COLOR trên BODY/FONT tags)
4. WHEN README mô tả CSS limitations THEN hệ thống SHALL nêu rõ không hỗ trợ external stylesheets và CSS selectors
5. WHEN README giải thích styling architecture THEN hệ thống SHALL mô tả việc parser extract CSS properties vào HtmlBlock structures

### Requirement 3

**User Story:** Là một developer đọc README, tôi muốn thấy thông tin chính xác về HTML parsing capabilities, để tôi hiểu được các HTML tags và features được hỗ trợ.

#### Acceptance Criteria

1. WHEN README liệt kê supported tags THEN hệ thống SHALL bao gồm H1-H3, P, A, IMG, BR, UL, LI, DIV, SPAN và semantic HTML5 tags (header, footer, nav, article, section, aside, main)
2. WHEN README mô tả table support THEN hệ thống SHALL nêu rõ TABLE, TR, TD, TH, TBODY, THEAD, TFOOT được map thành DIV để preserve content flow
3. WHEN README giải thích parser architecture THEN hệ thống SHALL mô tả FSM (Finite State Machine) với 11 states cho single-pass O(n) parsing
4. WHEN README đề cập entity decoding THEN hệ thống SHALL liệt kê các HTML entities được hỗ trợ (&amp;, &lt;, &gt;, &quot;, numeric entities &#..;)
5. WHEN README mô tả error handling THEN hệ thống SHALL nêu rõ forgiving parser với error recovery và warning system

### Requirement 4

**User Story:** Là một developer đọc README, tôi muốn thấy cấu trúc thư mục được tối ưu, để tôi dễ dàng navigate trong source code chính.

#### Acceptance Criteria

1. WHEN README hiển thị project structure THEN hệ thống SHALL chỉ bao gồm các thư mục source code chính (src/, libs/, deploy/, demo/)
2. WHEN README mô tả src/ directory THEN hệ thống SHALL chi tiết các subdirectories (browser/core, browser/ui, browser/network, browser/parser, browser/renderer, browser/res, proxy/)
3. WHEN README liệt kê files THEN hệ thống SHALL loại bỏ các file .bat và documentation files khỏi cây thư mục chính
4. WHEN README mô tả resource directory THEN hệ thống SHALL bao gồm src/browser/res/ với app.ico, app.rc, hand.cur, resource.h
5. WHEN README giải thích structure THEN hệ thống SHALL nhóm docs/ và tools/ riêng biệt khỏi core source structure

### Requirement 5

**User Story:** Là một developer đọc README, tôi muốn thấy thông tin chính xác về rendering capabilities, để tôi hiểu được cách browser vẽ nội dung.

#### Acceptance Criteria

1. WHEN README mô tả rendering architecture THEN hệ thống SHALL giải thích two-phase approach (Layout calculation + Paint)
2. WHEN README đề cập performance THEN hệ thống SHALL nêu rõ double-buffering với offscreen DC để flicker-free rendering
3. WHEN README giải thích font management THEN hệ thống SHALL mô tả font caching cho H1/H2/H3/P/A với DPI-aware sizing
4. WHEN README mô tả scrolling THEN hệ thống SHALL nêu rõ vertical scrolling support với scrollbar integration
5. WHEN README đề cập click handling THEN hệ thống SHALL giải thích hit-testing cho hyperlinks với clickable areas tracking

### Requirement 6

**User Story:** Là một developer đọc README, tôi muốn thấy code statistics chính xác, để tôi hiểu được quy mô của dự án.

#### Acceptance Criteria

1. WHEN README hiển thị code statistics THEN hệ thống SHALL cập nhật số dòng code chính xác cho từng module
2. WHEN README liệt kê parser module THEN hệ thống SHALL hiển thị parser.h (854 lines) và parser.cpp (1576 lines)
3. WHEN README liệt kê renderer module THEN hệ thống SHALL hiển thị renderer.h (273 lines) và renderer.cpp (1154 lines)
4. WHEN README liệt kê network module THEN hệ thống SHALL hiển thị network.h (444 lines) và network.cpp (760 lines)
5. WHEN README tính tổng THEN hệ thống SHALL cập nhật tổng số dòng code phản ánh chính xác quy mô dự án

### Requirement 7

**User Story:** Là một developer đọc README, tôi muốn thấy feature highlights được cập nhật, để tôi biết được các tính năng nổi bật mới nhất.

#### Acceptance Criteria

1. WHEN README liệt kê core features THEN hệ thống SHALL bao gồm CSS inline styling support và legacy HTML 3.2 color attributes
2. WHEN README mô tả parsing features THEN hệ thống SHALL đề cập semantic HTML5 tags support và table content preservation
3. WHEN README liệt kê rendering features THEN hệ thống SHALL bao gồm custom text/background colors per block và page-level styling from BODY tag
4. WHEN README đề cập image features THEN hệ thống SHALL nêu rõ async loading, caching, và placeholder rendering
5. WHEN README mô tả technical highlights THEN hệ thống SHALL cập nhật với CSS color parsing, entity decoding, và two-phase rendering

### Requirement 8

**User Story:** Là một developer đọc README, tôi muốn thấy phần "Tính Năng Nổi Bật" được tổ chức rõ ràng, để tôi nhanh chóng nắm bắt được capabilities của browser.

#### Acceptance Criteria

1. WHEN README tổ chức features THEN hệ thống SHALL nhóm thành các categories (HTML Support, CSS Styling, Image Handling, Rendering Engine, Network)
2. WHEN README mô tả HTML support THEN hệ thống SHALL liệt kê tags, semantic HTML5, tables, và entity decoding
3. WHEN README mô tả CSS styling THEN hệ thống SHALL chi tiết inline styles, color formats, và legacy attributes
4. WHEN README mô tả image handling THEN hệ thống SHALL giải thích async loading, caching, và format support
5. WHEN README mô tả rendering THEN hệ thống SHALL nêu rõ two-phase architecture, double-buffering, và font management
