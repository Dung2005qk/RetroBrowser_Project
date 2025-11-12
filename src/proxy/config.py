from typing import List, Dict, Optional, Set
import os


# ============================================================================
# ENVIRONMENT PROFILES - Cấu hình Theo Môi Trường
# ============================================================================
# Hệ thống profile cho phép chuyển đổi nhanh giữa các môi trường dev/test/prod.
# Override toàn bộ profile bằng: export APP_ENV=production

APP_ENV: str = os.environ.get('APP_ENV', 'development')
# Môi trường hiện tại: 'development' (mặc định), 'testing', 'production'
# Tác động: Tự động điều chỉnh LOG_LEVEL, DEBUG_MODE, caching, etc.
# Override: export APP_ENV=production


# ============================================================================
# NETWORK CONFIGURATION - Cấu hình Mạng Lõi
# ============================================================================
# Section này điều khiển cách proxy lắng nghe connections từ browser Win98 VM
# và giao tiếp với upstream servers. Quan trọng cho kết nối Host-VM.

PROXY_HOST: str = os.environ.get('PROXY_HOST', '0.0.0.0')
# Địa chỉ IP proxy lắng nghe.
# - '0.0.0.0': Lắng nghe TẤT CẢ interfaces (cần thiết để VM Win98 qua host-only network kết nối)
# - '127.0.0.1': CHỈ localhost, sẽ FAIL với VM vì VM không thể connect qua loopback
# Tác động: Nếu sai, browser không kết nối được proxy -> không load được web
# Mặc định: '0.0.0.0' để VM có thể kết nối (ví dụ: 192.168.56.1:8080)

PROXY_PORT: int = int(os.environ.get('PROXY_PORT', '8080'))
# Cổng proxy lắng nghe (phải khớp với browser C++ config).
# - 8080: Cổng phổ biến cho proxy, tránh conflict với HTTP (80), HTTPS (443)
# - Phải match với URL browser gửi (ví dụ: http://192.168.56.1:8080/)
# Tác động: Browser C++ phải hardcode/config cùng port này
# Giá trị hợp lệ: 1024-65535 (unprivileged ports), tránh <1024 cần root
# Override: export PROXY_PORT=9090

PROXY_BASE_URL: str = f"http://127.0.0.1:{PROXY_PORT}"
# Base URL để rewrite image src khi ENABLE_IMAGE_PROXYING=True.
# - Browser Win98 cần absolute URLs để fetch images qua proxy
# - Format: http://<browser_visible_ip>:<port>
# - 127.0.0.1: Dùng khi browser chạy trên cùng máy với proxy
# - 192.168.x.x: Dùng khi browser chạy trên VM, proxy trên host
# Tác động: Image tags sẽ có src="http://127.0.0.1:8080/image?url=..."
# Override: export PROXY_BASE_URL=http://192.168.56.1:8080

MAX_CONNECTIONS: int = 5
# Hàng đợi backlog cho socket.listen() - số connections chờ xử lý đồng thời.
# - Dự án nhỏ (1 browser client): 5 là đủ, chống overload nếu test nhiều
# - Quá thấp (<3): Có thể refuse connection nếu browser retry nhanh
# - Quá cao (>20): Lãng phí resources, không cần cho học thuật
# Tác động: Giới hạn số requests pending, bảo vệ proxy khỏi DoS đơn giản
# Mặc định: 5 (cân bằng cho single-client + buffer)

REQUEST_TIMEOUT: float = 30.0
# Timeout (giây) cho requests.get() khi fetch upstream web.
# - 30s: Đủ cho sites chậm, tránh browser Win98 treo chờ mãi (poor UX)
# - Quá ngắn (<10s): Sites hợp lệ nhưng chậm sẽ fail
# - Quá dài (>60s): User nghĩ browser crash, không phù hợp retro experience
# Tác động: Nếu web không response trong 30s -> proxy trả lỗi timeout cho browser
# Quan trọng: Win98 VM có tài nguyên thấp, timeout ngắn cải thiện responsiveness

BUFFER_SIZE: int = 4096
# Kích thước buffer (bytes) cho đọc/ghi socket giữa browser và proxy.
# - 4KB: Cân bằng hiệu năng (ít syscalls) và bộ nhớ thấp Win98
# - Quá nhỏ (512B): Nhiều syscalls, chậm cho HTML lớn
# - Quá lớn (64KB): Lãng phí RAM trên Win98 VM (16-64MB total RAM)
# Tác động: Ảnh hưởng tốc độ truyền data browser <-> proxy
# Lưu ý: Browser C++ cũng nên dùng buffer tương tự (4KB) cho consistency


# ============================================================================
# UPSTREAM REQUEST CONFIGURATION - Cấu hình Yêu Cầu Upstream & Định Danh
# ============================================================================
# Section này điều khiển cách proxy tự nhận diện khi fetch web, và headers gửi đi.
# Quan trọng vì nhiều site block User-Agent cũ hoặc thiếu headers chuẩn.

USER_AGENTS: Dict[str, str] = {
    'IE5_WIN98': 'Mozilla/4.0 (compatible; MSIE 5.0; Windows 98)',
    'MODERN_CHROME': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36',
    'FIREFOX_LATEST': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:89.0) Gecko/20100101 Firefox/89.0'
}
# Dictionary chứa nhiều User-Agent strings để thử nghiệm tương thích.
# - 'IE5_WIN98': Retro authentic, match với project theme, nhưng NHIỀU site block (cũ 20+ năm)
# - 'MODERN_CHROME': Hiện đại, ít bị block, nhưng mất tính retro
# - 'FIREFOX_LATEST': Lựa chọn trung gian
# Lý do thay User-Agent: Browser Win98 gửi IE5, nhưng proxy thay bằng modern để tránh:
#   + Sites từ chối serve nội dung (403, block cũ browsers)
#   + Serve nội dung khác (mobile version, degraded)
# Tác động: Quyết định site nào accessible, nội dung gì nhận được
# Dev tip: Test với IE5 trước, nếu fail switch sang MODERN_CHROME

ACTIVE_USER_AGENT_KEY: str = os.environ.get('USER_AGENT_KEY', 'IE5_WIN98')
# Key của User-Agent sẽ được sử dụng từ dictionary USER_AGENTS.
# - Cho phép switch nhanh giữa các danh tính chỉ bằng env var
# - Giá trị hợp lệ: 'IE5_WIN98', 'MODERN_CHROME', 'FIREFOX_LATEST'
# Override: export USER_AGENT_KEY=MODERN_CHROME
# Tác động: Thay đổi toàn bộ danh tính proxy khi fetch upstream
# Lưu ý: Nếu key không tồn tại, _validate_config() sẽ raise error

# DEFAULT_USER_AGENT sẽ được set sau khi validate ACTIVE_USER_AGENT_KEY
# (xem _set_default_user_agent() ở cuối file)

DEFAULT_HEADERS: Dict[str, str] = {
    'Accept': 'text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8',
    'Accept-Language': 'en-US,en;q=0.5',
    'Accept-Encoding': 'gzip, deflate',  # requests tự decompress
    'Connection': 'keep-alive',
    'Upgrade-Insecure-Requests': '1'
}
# Headers mặc định gửi với mọi upstream requests (kèm User-Agent từ trên).
# - 'Accept': Chỉ định nhận HTML/XML, fallback mọi loại (*/*;q=0.8)
# - 'Accept-Language': Ưu tiên tiếng Anh (giảm encoding issues với Win98)
# - 'Accept-Encoding': Cho phép compression (requests lib tự handle), giảm bandwidth
# - 'Connection': keep-alive tăng tốc nếu fetch nhiều resources từ cùng domain
# Tác động: Servers serve nội dung phù hợp format/language
# Lưu ý: Proxy sẽ merge với {'User-Agent': DEFAULT_USER_AGENT} khi gửi

DEFAULT_ENCODING: str = 'utf-8'
# Encoding mặc định khi decode response text từ upstream.
# - 'utf-8': Phổ biến, hỗ trợ đa ngôn ngữ
# - Win98 fonts hạn chế: Có thể lỗi font với Unicode phức tạp
# - Fallback: Nếu utf-8 fail, thử 'latin-1' hoặc detect từ response.encoding
# Tác động: Quyết định text hiển thị đúng hay toàn ký tự lỗi
# Dev tip: Nếu thấy ���, check encoding hoặc filter non-ASCII

PROXY_UPSTREAM: Optional[Dict[str, str]] = None
# Upstream proxy nếu máy Host nằm sau corporate proxy/firewall.
# - None: Kết nối trực tiếp Internet (default cho dev)
# - Dict: {'http': 'http://corporate-proxy:8080', 'https': '...'}
# Cách dùng: requests.get(..., proxies=PROXY_UPSTREAM)
# Tác động: Nếu mạng restricted, không config này -> fail kết nối
# Ví dụ: PROXY_UPSTREAM = {'http': 'http://10.0.0.1:3128', 'https': 'http://10.0.0.1:3128'}


# ============================================================================
# CONTENT PROCESSING & SANITIZATION - Cấu hình Xử Lý & Lọc Nội Dung
# ============================================================================
# Section TỐI QUAN TRỌNG - bảo vệ browser Win98 khỏi nội dung phức tạp/độc hại.
# Parser C++ đơn giản không handle JS/CSS/DOM phức tạp -> cần lọc ở proxy.

ENABLE_CONTENT_FILTERING: bool = True
# TỔNG CÔNG TẮC bật/tắt toàn bộ sanitization pipeline.
# - True: Áp dụng tất cả rules dưới (ALLOWED_TAGS, STRIP_SCRIPTS, etc.)
# - False: Trả HTML thô nguyên bản (CHỈ dùng khi debug parser, nguy hiểm!)
# Tác động: False -> browser nhận JS/CSS/tags lạ -> CRASH parser/renderer
# Mặc định: True (an toàn), chỉ tắt khi cần test raw HTML

ALLOWED_HTML_TAGS: Set[str] = {
    # CRITICAL: Document structure tags (MUST preserve for parser!)
    'html', 'head', 'body', 'title',           # Essential HTML structure
    
    # Text structure
    'p', 'h1', 'h2', 'h3', 'h4', 'h5', 'h6',  
    
    # Links, images, breaks
    'a', 'img', 'br', 'hr',                    
    
    # Lists
    'ul', 'ol', 'li',                          
    
    # Text formatting basic
    'b', 'i', 'u', 'strong', 'em',            
    
    # LEGACY HTML 3.2 TAGS (for retro sites like textfiles.com)
    'font', 'center',                          # <font color="..."> and <center>
    
    # Containers (simple layout)
    'div', 'span',
    
    # Semantic HTML5 containers (preserve structure, prevent auto-wrapping)
    'section', 'article', 'header', 'footer', 'nav', 'main', 'aside',
    
    # Tables (nếu renderer hỗ trợ)
    'table', 'tr', 'td', 'th'                  
}
# Whitelist các HTML tags AN TOÀN browser C++ render được.
# - Dùng Set (không phải List) cho O(1) lookup: if tag in ALLOWED_HTML_TAGS
# - Tags ngoài list này sẽ bị STRIP (loại bỏ tag + content)
# - Cân nhắc: Thêm 'table', 'tr', 'td' nếu renderer support, bỏ nếu không
# Tác động: Quyết định nội dung nào hiển thị, nào bị cắt
# Ví dụ: <nav>, <footer> không trong list -> bị loại bỏ hoàn toàn

ALLOWED_HTML_ATTRIBUTES: Dict[str, Set[str]] = {
    # Document structure - NOW WITH LEGACY HTML 3.2 COLOR ATTRIBUTES!
    # CRITICAL: textfiles.com uses <BODY BGCOLOR="#000000" TEXT="00FF00">
    'html': set(), 
    'head': set(), 
    'body': {'bgcolor', 'text', 'link', 'alink', 'vlink', 'style'},  # Legacy + modern
    'title': set(),
    
    # Links and images
    'a': {'href', 'title', 'style'},
    'img': {'src', 'alt', 'width', 'height', 'style'},
    
    # Text containers - NOW WITH STYLE SUPPORT for colors/fonts!
    'p': {'style'},
    'div': {'style'},
    'span': {'style'},
    'h1': {'style'}, 'h2': {'style'}, 'h3': {'style'},
    'h4': {'style'}, 'h5': {'style'}, 'h6': {'style'},
    
    # LEGACY FONT TAG (HTML 3.2) - used by textfiles.com!
    'font': {'color', 'face', 'size'},
    
    # Lists with styling
    'ul': {'style'}, 'ol': {'style'}, 'li': {'style'},
    
    # Semantic HTML5 containers with styling
    'section': {'style'}, 'article': {'style'}, 
    'header': {'style'}, 'footer': {'style'}, 
    'nav': {'style'}, 'main': {'style'}, 'aside': {'style'},
    
    # Text formatting
    'b': {'style'}, 'i': {'style'}, 'u': {'style'}, 
    'strong': {'style'}, 'em': {'style'},
    
    # Tables WITH LEGACY BGCOLOR!
    'table': {'border', 'style', 'bgcolor', 'width'},
    'tr': {'style', 'bgcolor'},
    'td': {'colspan', 'rowspan', 'style', 'bgcolor', 'align', 'valign'},
    'th': {'colspan', 'rowspan', 'style', 'bgcolor', 'align', 'valign'},
    
    # CENTER tag (deprecated but used in old HTML)
    'center': set()
}
# Whitelist attributes AN TOÀN per tag - chặn onclick, style, onerror, etc.
# - Key: tag name, Value: Set[str] attributes cho phép
# - Attributes KHÔNG trong list sẽ bị XÓA (ví dụ: <a onclick="hack()"> -> <a>)
# - 'href' cho <a>: Bắt buộc để hyperlinks hoạt động
# - 'src' cho <img>: Bắt buộc load ảnh
# - Chặn 'style' toàn bộ: Tránh CSS inline phức tạp (có thể thêm nếu cần màu đơn giản)
# Tác động: Loại bỏ vectors tấn công (XSS qua onerror, style: expression())
# Ví dụ: <img src="a.jpg" onerror="alert(1)"> -> <img src="a.jpg">

BLACKLISTED_TAGS: List[str] = [
    'script', 'style', 'iframe', 'frame', 'frameset',
    'video', 'audio', 'canvas', 'svg', 'object', 'embed',
    'applet', 'base', 'form', 'input', 'button'
]
# Tags TUYỆT ĐỐI CẤM - loại bỏ tag VÀ toàn bộ content bên trong.
# - 'script', 'style': Chặn JS/CSS hoàn toàn
# - 'iframe', 'frame': Chống load external content, nguy hiểm
# - 'video', 'audio', 'canvas': Browser không support multimedia phức tạp
# - 'form', 'input': Không hỗ trợ POST (chỉ GET), bỏ để tránh confusion
# - 'base': Không cần tag này, nguy hiểm cho URL resolution
# NOTE: 'link' và 'meta' được pre-remove bằng regex TRƯỚC khi parse BeautifulSoup
#       để tránh bug mis-nesting (BeautifulSoup coi content là children của <link>)
# Tác động: Tags này + nội dung BỊ XÓA HOÀN TOÀN trước khi trả browser
# Khác ALLOWED_TAGS: BLACKLISTED ưu tiên cao hơn, xóa content (không chỉ tag)

STRIP_SCRIPTS: bool = True
# Xóa mọi thẻ <script>...</script> và inline JS (onclick, onload, etc.).
# - True: Remove tất cả JS (an toàn, browser không parse JS)
# - False: Giữ lại (CHỈ debug, nguy hiểm)
# Tác động: True là BẮT BUỘC vì browser C++ không có JS engine
# Implementation: Regex/parser remove <script>, strip attributes bắt đầu 'on*'

STRIP_STYLES: bool = True
# Xóa mọi thẻ <style>...</style>, <link rel="stylesheet">, và inline style="...".
# - True: Remove CSS (đơn giản hóa, browser chỉ render text thuần)
# - False: Giữ lại (có thể parse màu cơ bản nếu renderer nâng cao)
# Tác động: True -> không màu/font custom, chỉ text đen/trắng
# Nâng cao: False + parse simple CSS (color: red) nếu GDI hỗ trợ

STRIP_HTML_COMMENTS: bool = True
# Xóa <!-- HTML comments --> để đơn giản hóa parser.
# - True: Loại bỏ comments (giảm size, dễ parse)
# - False: Giữ lại (không ảnh hưởng render nhưng tăng complexity)
# Tác động: True giảm ~5-10% HTML size, tăng tốc parse
# Lý do: Comments không cần thiết cho display, có thể chứa code cũ/debug info

ALLOWED_CONTENT_TYPES: List[str] = [
    'text/html',
    'text/plain',
    'application/xhtml+xml',
    'image/jpeg',
    'image/png',
    'image/gif',
    'image/bmp',
    'image/svg+xml',  # SVG vector images (will be converted to BMP by proxy)
    'image/webp'      # Modern WebP format (will be converted to BMP by proxy)
]
# Whitelist MIME types cho phép proxy trả về.
# - Filter theo header 'Content-Type' của upstream response
# - KHÔNG trong list -> proxy trả lỗi 415 Unsupported Media Type
# - 'text/html': Chính, browser cần
# - 'image/*': Nếu ENABLE_IMAGE_PROXYING=True, proxy fetch images riêng
#   + image/svg+xml: SVG (vector) - proxy convert to BMP via cairosvg
#   + image/webp: Modern format - proxy convert to BMP via PIL
# - Chặn: 'application/javascript', 'text/css', 'video/*', etc.
# Tác động: Chống browser nhận junk data (PDFs, binaries) -> crash
# Ví dụ: User browse PDF link -> proxy trả lỗi thay vì binary garbage

MAX_RESPONSE_SIZE_BYTES: int = 1 * 1024 * 1024  # 1MB
# Giới hạn KÍch thước (bytes) response tối đa từ upstream.
# - 1MB: Đủ cho hầu hết HTML text pages, chặn pages khổng lồ
# - Win98 có RAM thấp (16-64MB): Pages quá lớn -> tràn bộ nhớ, crash
# - Nếu vượt: Proxy trả lỗi 413 Content Too Large, không forward cho browser
# Tác động: Bảo vệ Win98 khỏi DoS (trang 100MB HTML), crash renderer
# Cân nhắc: Tăng lên 2-5MB nếu cần load news sites có nhiều text
# Lưu ý: Giới hạn này TRƯỚC sanitization (HTML thô), sau sanitize sẽ nhỏ hơn


# ============================================================================
# LOGGING & DEBUGGING - Cấu hình Ghi Log & Gỡ Lỗi
# ============================================================================
# Section này điều khiển output logs - quan trọng cho development và troubleshooting.

# Profile-based configuration overrides (áp dụng sau khi đọc biến môi trường cá nhân)
# Các giá trị này sẽ được override dựa trên APP_ENV ở cuối file trong _apply_env_profile()

LOG_LEVEL: str = os.environ.get('LOG_LEVEL', 'DEBUG')
# Cấp độ log (Python logging module levels).
# - 'DEBUG': Chi tiết mọi thứ (requests, responses, sanitization steps) - DEV
# - 'INFO': Hoạt động bình thường (client connect, URL requested) - STAGING
# - 'WARNING': Cảnh báo (blocked domain, large response) - PRODUCTION
# - 'ERROR': Chỉ lỗi nghiêm trọng (crashes, network fail) - PRODUCTION
# Tác động: Quyết định console output nhiều hay ít
# Override: export LOG_LEVEL=INFO để giảm noise khi demo
# Mặc định: 'DEBUG' cho dự án học thuật (cần hiểu flow)
# Lưu ý: Giá trị này có thể bị override bởi APP_ENV profile

LOG_TO_FILE: bool = False
# Ghi logs vào file thay vì chỉ console.
# - True: Append vào LOG_FILE_PATH (tốt cho analysis sau, không mất logs khi restart)
# - False: Chỉ console (đơn giản, realtime, đủ cho dev)
# Tác động: True tạo file log persistent, False logs mất khi đóng terminal
# Khuyến nghị: False khi dev, True khi demo/testing dài
# Lưu ý: Giá trị này có thể bị override bởi APP_ENV profile

LOG_FILE_PATH: Optional[str] = 'proxy.log'
# Đường dẫn file log nếu LOG_TO_FILE=True.
# - Relative path: Tạo trong thư mục chạy proxy.py
# - Absolute path: Chỉ định cụ thể (ví dụ: '/tmp/proxy.log')
# - None: Bỏ qua (nếu LOG_TO_FILE=False)
# Tác động: File này chứa toàn bộ history logs, có thể lớn sau nhiều requests
# Lưu ý: Cần .gitignore file này, rotation nếu chạy lâu

LOG_FORMAT: str = '%(asctime)s - [%(levelname)s] - %(name)s - %(message)s'
# Format string cho logging.Formatter().
# - %(asctime)s: Timestamp (YYYY-MM-DD HH:MM:SS,mmm)
# - %(levelname)s: DEBUG/INFO/WARNING/ERROR
# - %(name)s: Logger name (thường __name__ của module)
# - %(message)s: Nội dung log
# Tác động: Quyết định log output dễ đọc/parse
# Ví dụ output: "2025-01-15 10:30:45,123 - [INFO] - proxy - Client connected from 192.168.56.101"

DEBUG_MODE: bool = True
# Bật verbose debugging output - in thêm thông tin chi tiết ngoài logs.
# - True: In requests/responses body (HTML snippets), headers, sanitization diffs
# - False: Chỉ logs chuẩn (URLs, status codes)
# Tác động: True giúp debug parser/sanitizer, thấy HTML before/after
# Khuyến nghị: True khi dev, False khi demo (tránh spam console)
# Ví dụ: DEBUG_MODE=True -> print(f"Original HTML: {html[:200]}...")
# Lưu ý: Giá trị này có thể bị override bởi APP_ENV profile


# ============================================================================
# FEATURE FLAGS - Cờ Bật/Tắt Tính Năng Thử Nghiệm
# ============================================================================
# Section này chứa các tính năng tùy chọn/nâng cao có thể được bật/tắt độc lập.
# Tách biệt khỏi security/performance settings để dễ dàng quản lý experimental features.

ENABLE_SIMPLE_CACHING: bool = False
# Bật caching in-memory hoặc disk cho responses.
# - True: Lưu responses (HTML) theo URL, serve từ cache nếu còn hạn (TTL)
# - False: Luôn fetch mới (đơn giản hơn, fresh content)
# Tác động: True tăng tốc đáng kể khi reload cùng page, giảm bandwidth
# Trade-off: True tốn RAM/disk, phức tạp invalidation
# Khuyến nghị: False cho dev (tránh cache cũ), True cho demo/testing

ENABLE_IMAGE_PROXYING: bool = True
# Bật proxy cho images - fetch <img src="..."> qua proxy thay vì browser tự fetch.
# - True: Browser request images qua proxy (http://proxy:8080/image?url=...)
# - False: Browser tự fetch images (cần Winsock mở nhiều connections)
# Tác động: True đơn giản hóa browser (chỉ 1 connection đến proxy), proxy handle images
# Trade-off: True tăng tải proxy, False phức tạp browser networking
# Khuyến nghị: True để browser đơn giản, proxy làm "smart gateway"


# ============================================================================
# SECURITY & PERFORMANCE - Cấu hình Bảo Mật & Tối Ưu Hóa
# ============================================================================
# Section này cân bằng security (chặn domains độc hại) và performance (cache, limits).

SSL_VERIFY: bool = True
# Verify SSL certificates khi fetch HTTPS upstream.
# - True: Kiểm tra cert hợp lệ, CA trusted (an toàn, chuẩn)
# - False: Bỏ qua verify (CHỈ debug với self-signed cert, NGUY HIỂM MITM)
# Tác động: False cho phép intercept traffic, không dùng production
# Cảnh báo: Nếu False, thêm warnings.filterwarnings('ignore', category=InsecureRequestWarning)
# Mặc định: True (secure by default)

WHITELISTED_DOMAINS: List[str] = []
# Whitelist domains - nếu KHÔNG RỖNG, CHỈ cho phép domains này.
# - Rỗng []: Cho phép tất cả domains (trừ BLOCKED_DOMAINS)
# - Có items: Chỉ domains trong list được phép, còn lại CHẶN
# Tác động: Kiểm soát chặt chẽ truy cập, chống lạm dụng proxy
# Ví dụ: ['example.com', 'wikipedia.org'] -> chỉ 2 sites này accessible
# Use case: Nếu deploy public, whitelist để tránh proxy abuse (fetch arbitrary sites)

BLOCKED_DOMAINS: List[str] = [
    'doubleclick.net',
    'googlesyndication.com',
    'googletagmanager.com',
    'facebook.com',
    'twitter.com',
    'analytics.google.com',
    'ads.yahoo.com'
]
# Blacklist domains - CHẶN hoàn toàn, không fetch.
# - Chặn ads/tracking: Giảm tải renderer, tăng tốc load
# - Chặn social embeds: Facebook/Twitter widgets phức tạp, không cần
# Tác động: Request đến domain này -> proxy trả lỗi 403 Forbidden
# Cách check: Extract domain từ URL, if domain in BLOCKED_DOMAINS: reject
# Khuyến nghị: Mở rộng list với các ad networks phổ biến

ENFORCE_HTTPS: bool = False
# Từ chối requests http://, bắt buộc https:// only.
# - True: Chặn http:// (an toàn hơn, force encryption)
# - False: Cho phép cả http:// và https:// (linh hoạt hơn cho test sites)
# Tác động: True -> http://example.com bị reject, phải https://example.com
# Lưu ý: Nhiều test sites đơn giản chỉ có http://, nên mặc định False
# Nâng cao: Tự động upgrade http -> https nếu possible

CACHE_TTL_SECONDS: int = 300  # 5 phút
# Time-to-live (giây) cho cached responses nếu ENABLE_SIMPLE_CACHING=True.
# - 300s (5min): Cân bằng fresh vs efficiency, phù hợp news sites
# - Quá ngắn (<60s): Ít lợi ích cache
# - Quá dài (>3600s): Content cũ, không phù hợp dynamic sites
# Tác động: Sau TTL, cache expired -> fetch mới
# Implementation: Lưu {url: (html, timestamp)}, check time.time() - timestamp < TTL

CACHE_DIR: str = 'cache/'
# Thư mục lưu cache nếu dùng disk-based cache (thay in-memory).
# - 'cache/': Relative path, tạo trong proxy folder
# - Tự động tạo nếu không tồn tại: os.makedirs(CACHE_DIR, exist_ok=True)
# Tác động: Cache persist qua restarts, nhưng cần quản lý cleanup
# Lưu ý: Thêm vào .gitignore, xóa định kỳ nếu lớn


# ============================================================================
# TESTING & DEVELOPMENT - Cấu hình Test & Phát Triển
# ============================================================================
# Section này hỗ trợ unit testing và development workflows.

TEST_SITES: List[str] = [
    'http://example.com',
    'https://lite.cnn.com',
    'https://text.npr.org',
    'https://simple.wikipedia.org'
]
# Danh sách sites ĐƠN GIẢN, TEXT-HEAVY cho unit tests và manual testing.
# - example.com: Minimal HTML, luôn available, perfect baseline
# - lite/text versions: Lightweight news sites, ít JS/CSS
# - simple.wikipedia.org: Text focus, chuẩn cho retro browsers
# Tác động: Dùng cho test_scripts/test_request.py, đảm bảo proxy hoạt động
# Tránh: Sites phức tạp (Google, Facebook) - sẽ fail nhiều do sanitization

MAX_CLIENTS: int = 50
# Giới hạn số clients đồng thời (concurrent connections).
# - 50: Đủ cho Win98 browser với nhiều images trên 1 page (Wikipedia ~20 images)
# - Browser tạo unlimited threads, mỗi image = 1 connection đồng thời
# - Thấp hơn MAX_CONNECTIONS (socket backlog), đây là active processing limit
# Tác động: Nếu vượt, connections mới phải chờ hoặc bị reject
# Implementation: Threading/asyncio với semaphore(MAX_CLIENTS)
# FIX: Tăng từ 10 → 50 để tránh "503 Service Unavailable" khi load Wikipedia


# ============================================================================
# MODULE INITIALIZATION CHECK
# ============================================================================
# Validation logic (optional) - kiểm tra config hợp lệ khi import.

def _apply_env_profile() -> None:
    """
    Áp dụng environment profile dựa trên APP_ENV.
    Override các biến global theo từng môi trường.
    
    Gọi TRƯỚC _validate_config() để đảm bảo giá trị cuối cùng đúng.
    """
    global LOG_LEVEL, DEBUG_MODE, ENABLE_SIMPLE_CACHING, LOG_TO_FILE, PROXY_PORT
    
    if APP_ENV == 'production':
        # Production: Tối ưu performance, tắt debug, bật cache
        LOG_LEVEL = os.environ.get('LOG_LEVEL', 'INFO')
        DEBUG_MODE = False
        ENABLE_SIMPLE_CACHING = True
        LOG_TO_FILE = True
        # PROXY_PORT giữ nguyên (đã config ở NETWORK section)
        
    elif APP_ENV == 'testing':
        # Testing: Log đầy đủ, port riêng, không cache để test fresh
        LOG_LEVEL = os.environ.get('LOG_LEVEL', 'DEBUG')
        DEBUG_MODE = True
        ENABLE_SIMPLE_CACHING = False
        LOG_TO_FILE = True  # Lưu logs để analyze tests
        PROXY_PORT = int(os.environ.get('PROXY_PORT', '9999'))  # Port riêng tránh conflict
        
    else:  # 'development' (default)
        # Development: Debug max, không cache, console only
        LOG_LEVEL = os.environ.get('LOG_LEVEL', 'DEBUG')
        DEBUG_MODE = True
        ENABLE_SIMPLE_CACHING = False
        LOG_TO_FILE = False
        # PROXY_PORT giữ nguyên 8080 (default)


def _validate_config() -> None:
    """
    Kiểm tra tính hợp lệ của config khi module được import.
    Raise ValueError nếu phát hiện misconfiguration nghiêm trọng.
    """
    # Validate APP_ENV
    valid_envs = ['development', 'testing', 'production']
    if APP_ENV not in valid_envs:
        raise ValueError(f"APP_ENV không hợp lệ: {APP_ENV}. Hợp lệ: {valid_envs}")
    
    # Validate port range
    if not (1024 <= PROXY_PORT <= 65535):
        raise ValueError(f"PROXY_PORT phải trong khoảng 1024-65535, nhận: {PROXY_PORT}")
    
    # Validate timeouts
    if REQUEST_TIMEOUT <= 0:
        raise ValueError(f"REQUEST_TIMEOUT phải > 0, nhận: {REQUEST_TIMEOUT}")
    
    # Validate sizes
    if MAX_RESPONSE_SIZE_BYTES <= 0:
        raise ValueError(f"MAX_RESPONSE_SIZE_BYTES phải > 0, nhận: {MAX_RESPONSE_SIZE_BYTES}")
    
    # Validate log level
    valid_levels = ['DEBUG', 'INFO', 'WARNING', 'ERROR', 'CRITICAL']
    if LOG_LEVEL.upper() not in valid_levels:
        raise ValueError(f"LOG_LEVEL không hợp lệ: {LOG_LEVEL}. Hợp lệ: {valid_levels}")
    
    # Validate User-Agent key
    if ACTIVE_USER_AGENT_KEY not in USER_AGENTS:
        raise ValueError(
            f"ACTIVE_USER_AGENT_KEY không tồn tại: {ACTIVE_USER_AGENT_KEY}. "
            f"Hợp lệ: {list(USER_AGENTS.keys())}"
        )
    
    # Validate whitelist/blacklist conflict
    if WHITELISTED_DOMAINS and BLOCKED_DOMAINS:
        overlap = set(WHITELISTED_DOMAINS) & set(BLOCKED_DOMAINS)
        if overlap:
            raise ValueError(f"Conflict: Domains vừa trong whitelist vừa blacklist: {overlap}")


def _set_default_user_agent() -> None:
    """
    Set DEFAULT_USER_AGENT sau khi validate ACTIVE_USER_AGENT_KEY.
    Phải gọi sau _validate_config() để đảm bảo key hợp lệ.
    """
    global DEFAULT_USER_AGENT
    DEFAULT_USER_AGENT = USER_AGENTS[ACTIVE_USER_AGENT_KEY]


# Auto-apply environment profile, validate, sau đó set User-Agent
_apply_env_profile()
_validate_config()
_set_default_user_agent()


# ============================================================================
# MAIN - Config Dump (Debugging)
# ============================================================================
if __name__ == '__main__':
    """
    Chạy file này trực tiếp để dump toàn bộ config (debug).
    
    Usage:
        $ python config.py
        hoặc
        $ export PROXY_PORT=9090 && python config.py
        $ export APP_ENV=production && python config.py
        $ export USER_AGENT_KEY=MODERN_CHROME && python config.py
    """
    import json
    
    print("=" * 80)
    print("WIN98 RETRO BROWSER - PROXY CONFIGURATION DUMP")
    print("=" * 80)
    print(f"\n🌍 ACTIVE ENVIRONMENT PROFILE: {APP_ENV.upper()}")
    print(f"🌐 ACTIVE USER-AGENT: {ACTIVE_USER_AGENT_KEY}")
    print()
    
    # Collect all UPPERCASE variables (config constants)
    config_vars = {
        key: value for key, value in globals().items()
        if key.isupper() and not key.startswith('_')
    }
    
    # Convert sets to lists for JSON serialization (including nested dicts)
    def _serialize_value(val):
        """Chuyển đổi sets và nested structures sang JSON-serializable."""
        if isinstance(val, set):
            return list(val)
        elif isinstance(val, dict):
            return {k: _serialize_value(v) for k, v in val.items()}
        else:
            return val
    
    config_display = {key: _serialize_value(value) for key, value in config_vars.items()}
    
    # Pretty print
    for section in [
        "ENVIRONMENT PROFILES",
        "NETWORK CONFIGURATION",
        "UPSTREAM REQUEST CONFIGURATION",
        "CONTENT PROCESSING & SANITIZATION",
        "LOGGING & DEBUGGING",
        "FEATURE FLAGS",
        "SECURITY & PERFORMANCE",
        "TESTING & DEVELOPMENT"
    ]:
        print(f"\n### {section} ###")
        print("-" * 80)
    
    # Dump all as JSON
    print(json.dumps(config_display, indent=2, ensure_ascii=False))
    print()
    print("=" * 80)
    print("Config validation: PASSED ✓")
    print("=" * 80)