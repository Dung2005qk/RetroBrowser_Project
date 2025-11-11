#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Win98 Retro Browser - Intelligent Caching & Sanitizing Proxy

This module acts as a defensive middleware for the Win98 Retro Browser project.
It listens for simple HTTP GET requests from the C++ browser client running on a
Windows 98 VM. It then fetches the requested content from the modern web via HTTPS,
rigorously sanitizes it to remove incompatible elements like JavaScript, CSS, and
complex HTML tags, and returns a simplified, safe HTTP response.

Core Responsibilities:
- Bridge: Translates simple client HTTP to modern server HTTPS.
- Guardian: Strips all potentially harmful or incompatible content.
- Accelerator: Provides optional in-memory caching for repeated requests.
- Translator: Rewrites image URLs to be proxied, simplifying client logic.

All behavior is strictly controlled by the 'config.py' module in the same directory.

Usage:
1. Ensure 'config.py' is configured correctly.
2. Install dependencies: pip install requests beautifulsoup4
3. Run from the terminal: python proxy.py
4. Configure the Win98 browser to use this proxy's IP and port. The browser
   should send requests in the format: GET /<full_upstream_url> HTTP/1.0
   e.g., GET /https://example.com/ HTTP/1.0
"""

import socket
import socketserver
import threading
import logging
import time
from logging.handlers import RotatingFileHandler
from urllib.parse import urlparse, urljoin, parse_qs, quote
from typing import Dict, Optional, Tuple

import requests
import bs4
from bs4 import BeautifulSoup

# Import all settings from the configuration module. This is the single source
# of truth for all proxy behavior.
try:
    from config import *
except ImportError:
    print("FATAL: 'config.py' not found. This module is required for operation.")
    exit(1)

# ============================================================================
# GLOBAL SETUP
# ============================================================================

# --- Logging Configuration ---
# Set up logging based on the configuration file.
log_level_map = {
    'DEBUG': logging.DEBUG,
    'INFO': logging.INFO,
    'WARNING': logging.WARNING,
    'ERROR': logging.ERROR,
    'CRITICAL': logging.CRITICAL
}
LOG_LEVEL_ACTUAL = log_level_map.get(LOG_LEVEL.upper(), logging.INFO)

logging.basicConfig(level=LOG_LEVEL_ACTUAL, format=LOG_FORMAT)
logger = logging.getLogger('proxy')

if LOG_TO_FILE and LOG_FILE_PATH:
    # Use a rotating file handler to prevent log files from growing indefinitely.
    # 5MB max size, 5 backup files.
    file_handler = RotatingFileHandler(
        LOG_FILE_PATH, maxBytes=5*1024*1024, backupCount=5
    )
    file_handler.setFormatter(logging.Formatter(LOG_FORMAT))
    logger.addHandler(file_handler)

# --- Caching Mechanism ---
# A simple, thread-safe, in-memory cache for responses.
CACHE: Dict[str, Tuple[bytes, str, float]] = {}
CACHE_LOCK = threading.Lock()

# --- Concurrency Limiter ---
# Use a semaphore to limit the number of active request handler threads,
# protecting against overwhelming the system or upstream services.
CLIENT_SEMAPHORE = threading.Semaphore(MAX_CLIENTS)


# ============================================================================
# PROXY REQUEST HANDLER
# ============================================================================

class ProxyRequestHandler(socketserver.BaseRequestHandler):
    """
    Handles a single client connection. Each instance is run in its own thread
    by the ThreadingTCPServer.
    """

    def handle(self) -> None:
        """
        Main logic for processing a single client request.
        """
        if not CLIENT_SEMAPHORE.acquire(blocking=False):
            logger.warning("Max client limit reached. Connection rejected.")
            self._send_error_response(503, "Service Unavailable", "Max clients reached.")
            return

        try:
            self.request.settimeout(REQUEST_TIMEOUT)
            
            # 1. Receive and parse the client's request
            method, path, upstream_url = self._parse_client_request()
            
            # 2. Handle special proxy endpoints (e.g., image proxying)
            # Check both formats:
            #   - /image?url=...  (custom format with leading slash)
            #   - http://hostname/image?url=... (standard proxy format)
            is_image_endpoint = False
            if path and path.startswith('/image?url='):
                is_image_endpoint = True
            elif upstream_url and '/image?url=' in upstream_url:
                # Extract path part from full URL for image endpoint
                parsed = urlparse(upstream_url)
                if parsed.path.startswith('/image') and 'url=' in parsed.query:
                    path = parsed.path + '?' + parsed.query
                    is_image_endpoint = True
            
            if is_image_endpoint:
                client_ip, client_port = self.request.getpeername()
                logger.info(f"Image proxy request from {client_ip}:{client_port}")
                self._handle_image_proxy_request(path)
                return
            
            # 3. For normal requests, upstream_url must be present
            if not upstream_url:
                return # Error already sent by parser

            client_ip, client_port = self.request.getpeername()
            logger.info(f"Request received from {client_ip}:{client_port} for URL: {upstream_url}")

            # 4. Perform pre-flight security checks
            parsed_url = urlparse(upstream_url)
            if not self._validate_request(parsed_url):
                return # Error already sent by validator

            # 5. Check cache if enabled
            if ENABLE_SIMPLE_CACHING:
                cache_hit = self._get_from_cache(upstream_url)
                if cache_hit:
                    logger.info(f"Cache hit for {upstream_url}")
                    body, content_type = cache_hit
                    self._send_http_response(200, "OK", body, content_type)
                    return
                logger.info(f"Cache miss for {upstream_url}")

            # 5. Fetch content from the upstream server
            upstream_response = self._fetch_upstream(upstream_url)
            if not upstream_response:
                return # Error already sent by fetcher

            # 6. Sanitize the content
            content_type = upstream_response.headers.get('Content-Type', 'text/plain').split(';')[0].strip()
            
            if content_type in ['text/html', 'application/xhtml+xml'] and ENABLE_CONTENT_FILTERING:
                sanitized_body = self._sanitize_html(upstream_response.text, upstream_url)
                body_bytes = sanitized_body.encode(DEFAULT_ENCODING, errors='replace')
            else:
                body_bytes = upstream_response.content

            # 7. Update cache if enabled
            if ENABLE_SIMPLE_CACHING:
                self._set_in_cache(upstream_url, body_bytes, content_type)

            # 8. Send the final response to the client
            self._send_http_response(200, "OK", body_bytes, content_type)

        except socket.timeout:
            logger.warning("Socket timeout with client.")
            self._send_error_response(408, "Request Timeout")
        except Exception as e:
            logger.error(f"Unhandled exception in request handler: {e}", exc_info=True)
            self._send_error_response(500, "Internal Server Error")
        finally:
            CLIENT_SEMAPHORE.release()
            self.request.close()

    def _parse_client_request(self) -> Tuple[Optional[str], Optional[str], Optional[str]]:
        """
        Reads and parses the initial lines of the client request reliably.
        Accumulates data until the full header is received to handle TCP fragmentation.
        """
        raw_request = b''
        try:
            # IMPROVED: Accumulate data until we receive the complete header
            # This handles TCP fragmentation robustly
            while b'\r\n\r\n' not in raw_request:
                chunk = self.request.recv(BUFFER_SIZE)
                if not chunk:
                    # Client closed connection before sending complete request
                    logger.warning("Connection closed by client before full request was sent.")
                    return None, None, None
                raw_request += chunk
                # Protection against excessively large headers (DoS attack prevention)
                if len(raw_request) > MAX_RESPONSE_SIZE_BYTES:
                    self._send_error_response(413, "Request Entity Too Large", "Request headers are too large.")
                    return None, None, None

            request_text = raw_request.decode('utf-8', errors='ignore')
            logger.debug(f"Raw client request:\n---\n{request_text.strip()}\n---")
            
            request_line = request_text.split('\r\n')[0]
            method, path, _ = request_line.split(' ', 2)

            if method.upper() != 'GET':
                self._send_error_response(405, "Method Not Allowed", "Only GET is supported.")
                return None, None, None

            # Check for special proxy endpoints first (these don't follow URL format)
            if path.startswith('/image?url='):
                # Image proxy endpoint - return as-is
                return method, path, None  # upstream_url = None for special endpoints
            
            # Support both standard proxy format and custom format with leading slash:
            # Standard: GET http://example.com HTTP/1.1
            # Custom:   GET /http://example.com HTTP/1.1
            if path.startswith('http://') or path.startswith('https://'):
                # Standard proxy format (no leading slash)
                return method, path, path
            elif len(path) > 1 and (path.startswith('/http://') or path.startswith('/https://')):
                # Custom format with leading slash
                return method, path, path[1:]

            self._send_error_response(400, "Bad Request", "Invalid URL format in path.")
            return None, None, None
        except (ValueError, IndexError):
            self._send_error_response(400, "Bad Request", "Malformed request line.")
            return None, None, None
        except UnicodeDecodeError:
            self._send_error_response(400, "Bad Request", "Invalid character encoding in request.")
            return None, None, None

    def _validate_request(self, parsed_url: urlparse) -> bool:
        """Enforces security policies like domain whitelists/blacklists."""
        domain = parsed_url.netloc
        scheme = parsed_url.scheme

        if WHITELISTED_DOMAINS and domain not in WHITELISTED_DOMAINS:
            logger.warning(f"Domain '{domain}' is not in whitelist. Blocking request.")
            self._send_error_response(403, "Forbidden", f"Domain '{domain}' is not permitted.")
            return False

        if domain in BLOCKED_DOMAINS:
            logger.warning(f"Domain '{domain}' is in blacklist. Blocking request.")
            self._send_error_response(403, "Forbidden", f"Domain '{domain}' is blocked.")
            return False
            
        if ENFORCE_HTTPS and scheme != 'https':
            logger.warning(f"Non-HTTPS request to '{domain}' blocked by policy.")
            self._send_error_response(400, "Bad Request", "Only HTTPS requests are allowed.")
            return False
            
        return True

    def _fetch_upstream(self, url: str) -> Optional[requests.Response]:
        """Fetches content from the target URL."""
        headers = {**DEFAULT_HEADERS, 'User-Agent': DEFAULT_USER_AGENT}
        logger.info(f"Fetching upstream from {url} with User-Agent '{DEFAULT_USER_AGENT}'")

        try:
            response = requests.get(
                url,
                headers=headers,
                timeout=REQUEST_TIMEOUT,
                proxies=PROXY_UPSTREAM or None,
                verify=SSL_VERIFY,
                allow_redirects=True,
                stream=True  # stream=True to check size before downloading
            )
            
            # Check response size before downloading the whole body
            content_length = int(response.headers.get('Content-Length', 0))
            if content_length > MAX_RESPONSE_SIZE_BYTES:
                logger.warning(f"Response from {url} exceeds size limit ({content_length} > {MAX_RESPONSE_SIZE_BYTES})")
                self._send_error_response(413, "Content Too Large")
                return None

            # Now download the content
            response.raise_for_status() # Raise HTTPError for bad responses (4xx or 5xx)

            content_type = response.headers.get('Content-Type', '').split(';')[0].strip()
            if not any(allowed in content_type for allowed in ALLOWED_CONTENT_TYPES):
                 logger.warning(f"Unsupported content type '{content_type}' from {url}")
                 self._send_error_response(415, "Unsupported Media Type")
                 return None

            return response
            
        except requests.exceptions.Timeout:
            logger.error(f"Upstream timeout for {url}")
            self._send_error_response(504, "Gateway Timeout")
        except requests.exceptions.ConnectionError:
            logger.error(f"Upstream connection error for {url}")
            self._send_error_response(502, "Bad Gateway")
        except requests.exceptions.HTTPError as e:
            logger.error(f"Upstream HTTP error for {url}: {e.response.status_code}")
            self._send_error_response(e.response.status_code, e.response.reason)
        except Exception as e:
            logger.error(f"Generic upstream fetch error for {url}: {e}", exc_info=True)
            self._send_error_response(500, "Internal Server Error")
        
        return None

    def _sanitize_html(self, html_content: str, base_url: str) -> str:
        """
        The core sanitization pipeline for HTML content with robust fallback mechanism.
        If sanitization fails, returns plain text content as a safe fallback.
        """
        try:
            logger.info(f"Sanitizing HTML for {base_url}...")
            if DEBUG_MODE:
                logger.debug(f"HTML before sanitization (first 250 chars):\n---\n{html_content[:250]}\n---")

            soup = BeautifulSoup(html_content, 'html.parser')
            
            # Pipeline Step 1: Remove comments
            if STRIP_HTML_COMMENTS:
                for comment in soup.find_all(text=lambda t: isinstance(t, bs4.Comment)):
                    comment.extract()

            # Pipeline Step 2: Remove dangerous/unsupported tags AND their content
            for tag_name in BLACKLISTED_TAGS:
                for tag in soup.find_all(tag_name):
                    tag.decompose()

            # Pipeline Step 3: Iterate all remaining tags for attribute/tag whitelisting
            for tag in soup.find_all(True):
                # Whitelist tags: If not in allowed list, remove the tag but keep its content
                if tag.name not in ALLOWED_HTML_TAGS:
                    tag.unwrap()
                    continue # Tag is gone, no need to check attributes

                # Whitelist attributes: Remove any attribute not explicitly allowed for this tag
                allowed_attrs = ALLOWED_HTML_ATTRIBUTES.get(tag.name, set())
                current_attrs = dict(tag.attrs)
                for attr_name, attr_value in current_attrs.items():
                    if attr_name not in allowed_attrs:
                        del tag[attr_name]
                    # ENHANCED SECURITY: Remove 'javascript:', 'vbscript:', 'data:' and other dangerous schemes
                    elif attr_name in ['href', 'src'] and isinstance(attr_value, str):
                        attr_lower = attr_value.strip().lower()
                        dangerous_schemes = ['javascript:', 'vbscript:', 'data:', 'about:']
                        if any(attr_lower.startswith(scheme) for scheme in dangerous_schemes):
                            logger.warning(f"Removed dangerous URL scheme from {attr_name}: {attr_value[:50]}")
                            del tag[attr_name]

                # Rewrite URLs to be absolute
                if tag.name == 'a' and tag.has_attr('href'):
                    tag['href'] = urljoin(base_url, tag['href'])
                if tag.name == 'img' and tag.has_attr('src'):
                    absolute_src = urljoin(base_url, tag['src'])
                    if ENABLE_IMAGE_PROXYING:
                        # Rewrite image src to point back to our proxy
                        tag['src'] = f"/image?url={quote(absolute_src)}"
                    else:
                        tag['src'] = absolute_src

            sanitized_html = str(soup)
            
            if DEBUG_MODE:
                logger.debug(f"HTML after sanitization (first 250 chars):\n---\n{sanitized_html[:250]}\n---")
                original_len = len(html_content)
                final_len = len(sanitized_html)
                reduction = 100 * (original_len - final_len) / original_len if original_len > 0 else 0
                logger.debug(f"Sanitization complete. Size reduced by {reduction:.2f}%.")
                
            return sanitized_html
            
        except Exception as e:
            # FALLBACK MECHANISM: If sanitization fails, provide plain text content
            logger.error(f"Sanitization failed for {base_url}: {e}", exc_info=DEBUG_MODE)
            try:
                # Attempt to extract plain text as a safe fallback
                fallback_soup = BeautifulSoup(html_content, 'html.parser')
                fallback_text = fallback_soup.get_text(separator='\n', strip=True)
                fallback_html = f"""<html>
<head><title>Content Display Error</title></head>
<body>
<h1>⚠ Warning: Could not render page correctly</h1>
<p>The HTML sanitization process encountered an error. Displaying plain text content as a safe fallback.</p>
<p><strong>URL:</strong> {base_url}</p>
<hr>
<pre style="white-space: pre-wrap; word-wrap: break-word;">{fallback_text}</pre>
<hr>
<p><em>Win98 Retro Browser Proxy - Safe Mode</em></p>
</body>
</html>"""
                logger.warning(f"Serving plain text fallback for {base_url}")
                return fallback_html
            except Exception as fallback_e:
                # Ultimate fallback if even text extraction fails
                logger.critical(f"Fallback mechanism also failed for {base_url}: {fallback_e}")
                return f"""<html>
<head><title>Critical Error</title></head>
<body>
<h1>Critical Error</h1>
<p>Could not process or display content from <strong>{base_url}</strong>.</p>
<p>Please try a different URL or contact support.</p>
</body>
</html>"""

    def _handle_image_proxy_request(self, path: str) -> None:
        """Fetches an image and returns its raw binary content."""
        try:
            query = urlparse(path).query
            image_url = parse_qs(query)['url'][0]
            logger.info(f"Proxying image request for: {image_url}")

            # Fetch the image using the same robust method
            response = self._fetch_upstream(image_url)
            if response:
                content_type = response.headers.get('Content-Type', 'application/octet-stream')
                self._send_http_response(200, "OK", response.content, content_type)

        except (KeyError, IndexError):
            self._send_error_response(400, "Bad Request", "Missing 'url' parameter for image proxy.")
        except Exception as e:
            logger.error(f"Error proxying image: {e}", exc_info=True)
            self._send_error_response(500, "Internal Server Error")
    
    def _get_from_cache(self, url: str) -> Optional[Tuple[bytes, str]]:
        """Retrieves an item from the cache if it exists and is not expired."""
        with CACHE_LOCK:
            if url in CACHE:
                body, content_type, timestamp = CACHE[url]
                if (time.time() - timestamp) < CACHE_TTL_SECONDS:
                    # Update timestamp on hit to implement LRU-like behavior
                    CACHE[url] = (body, content_type, time.time())
                    return body, content_type
                else:
                    # Expired
                    del CACHE[url]
        return None

    def _set_in_cache(self, url: str, body: bytes, content_type: str) -> None:
        """Adds or updates an item in the cache."""
        with CACHE_LOCK:
            CACHE[url] = (body, content_type, time.time())
        logger.debug(f"Cached {len(body)} bytes for {url}")
            
    def _send_error_response(self, code: int, reason: str, message: Optional[str] = None) -> None:
        """Sends a simple HTML error page to the client."""
        body_text = message if message else f"The proxy encountered an error: {reason}"
        body_html = f"<html><head><title>Proxy Error {code}</title></head>" \
                    f"<body><h1>HTTP Error {code}: {reason}</h1>" \
                    f"<p>{body_text}</p><hr><i>Win98 Retro Proxy</i></body></html>"
        self._send_http_response(code, reason, body_html.encode('utf-8'), 'text/html; charset=utf-8')

    def _send_http_response(self, code: int, reason: str, body: bytes, content_type: str) -> None:
        """Constructs and sends a complete HTTP/1.1 response to the client."""
        try:
            response = bytearray()
            response.extend(f"HTTP/1.1 {code} {reason}\r\n".encode('utf-8'))
            response.extend(f"Content-Type: {content_type}\r\n".encode('utf-8'))
            response.extend(f"Content-Length: {len(body)}\r\n".encode('utf-8'))
            response.extend(b"Connection: close\r\n")
            response.extend(b"\r\n")
            response.extend(body)
            
            self.request.sendall(response)
            logger.info(f"Response sent: {code} {reason} ({len(body)} bytes, {content_type})")
        except Exception as e:
            logger.error(f"Failed to send response to client: {e}")

# ============================================================================
# MAIN EXECUTION
# ============================================================================

if __name__ == '__main__':
    try:
        # The configuration is validated automatically upon import in config.py
        
        server = socketserver.ThreadingTCPServer((PROXY_HOST, PROXY_PORT), ProxyRequestHandler)
        server.daemon_threads = True
        
        logger.info("=" * 60)
        logger.info("  Win98 Retro Browser - Intelligent Proxy Starting Up")
        logger.info("=" * 60)
        logger.info(f"Mode: {APP_ENV.upper()}")
        logger.info(f"Listening on: {PROXY_HOST}:{PROXY_PORT}")
        logger.info(f"Log Level: {LOG_LEVEL.upper()}")
        logger.info(f"User-Agent: {DEFAULT_USER_AGENT[:60]}{'...' if len(DEFAULT_USER_AGENT) > 60 else ''}")
        logger.info(f"Max Concurrent Clients: {MAX_CLIENTS}")
        logger.info(f"Request Timeout: {REQUEST_TIMEOUT}s")
        logger.info(f"Content Filtering: {'ENABLED' if ENABLE_CONTENT_FILTERING else 'DISABLED'}")
        logger.info(f"Simple Caching: {'ENABLED' if ENABLE_SIMPLE_CACHING else 'DISABLED'}")
        if ENABLE_SIMPLE_CACHING:
            logger.info(f"  Cache TTL: {CACHE_TTL_SECONDS}s")
        logger.info(f"Image Proxying: {'ENABLED' if ENABLE_IMAGE_PROXYING else 'DISABLED'}")
        logger.info(f"SSL Verification: {'ENABLED' if SSL_VERIFY else 'DISABLED'}")
        if WHITELISTED_DOMAINS:
            logger.info(f"Domain Whitelist: {len(WHITELISTED_DOMAINS)} domain(s)")
        if BLOCKED_DOMAINS:
            logger.info(f"Domain Blacklist: {len(BLOCKED_DOMAINS)} domain(s)")
        logger.info("=" * 60)
        logger.info("Press Ctrl+C to stop the proxy.")
        
        server.serve_forever()
        
    except OSError as e:
        logger.critical(f"Failed to bind to {PROXY_HOST}:{PROXY_PORT}. Is the port already in use? Error: {e}")
    except KeyboardInterrupt:
        logger.info("Shutdown signal received. Closing server...")
        server.shutdown()
        logger.info("Proxy stopped gracefully.")
    except Exception as e:
        logger.critical(f"A fatal error occurred: {e}", exc_info=True)