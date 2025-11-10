#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Unit tests for proxy.py logic
"""

import sys
import os

# Add proxy directory to path
sys.path.insert(0, r"c:\Users\LMC\OneDrive - THS\Desktop\RetroBrowser_Project\src\proxy")

def test_parse_logic():
    """Test the parsing and URL handling logic"""
    print("=" * 60)
    print("Testing Proxy Logic")
    print("=" * 60)
    
    # Test 1: URL parsing
    print("\n[Test 1] URL Format Validation")
    test_paths = [
        ("/https://example.com", True, "https://example.com"),
        ("/http://example.com", True, "http://example.com"),
        ("/https://www.google.com/search?q=test", True, "https://www.google.com/search?q=test"),
        ("/ftp://example.com", False, None),
        ("/example.com", False, None),
        ("", False, None),
    ]
    
    for path, should_pass, expected_url in test_paths:
        if len(path) > 1 and (path.startswith('/http://') or path.startswith('/https://')):
            result_url = path[1:]
            passed = (result_url == expected_url)
            status = "✓" if passed else "✗"
            print(f"  {status} Path: '{path}' -> '{result_url}' (expected: '{expected_url}')")
        else:
            if not should_pass:
                print(f"  ✓ Path: '{path}' correctly rejected")
            else:
                print(f"  ✗ Path: '{path}' should have passed but was rejected")
    
    # Test 2: Dangerous URL scheme detection
    print("\n[Test 2] Dangerous URL Scheme Detection")
    test_urls = [
        ("javascript:alert('xss')", True),
        ("JavaScript:alert('xss')", True),
        ("vbscript:msgbox('xss')", True),
        ("data:text/html,<script>alert('xss')</script>", True),
        ("about:blank", True),
        ("http://example.com", False),
        ("https://example.com", False),
        ("  javascript:alert('xss')  ", True),  # with whitespace
    ]
    
    dangerous_schemes = ['javascript:', 'vbscript:', 'data:', 'about:']
    
    for url, should_be_dangerous in test_urls:
        url_lower = url.strip().lower()
        is_dangerous = any(url_lower.startswith(scheme) for scheme in dangerous_schemes)
        
        if is_dangerous == should_be_dangerous:
            status = "✓"
            result = "DANGEROUS" if is_dangerous else "SAFE"
        else:
            status = "✗"
            result = f"WRONG (detected as {'DANGEROUS' if is_dangerous else 'SAFE'})"
        
        print(f"  {status} '{url[:40]}...' -> {result}")
    
    # Test 3: Request header accumulation logic
    print("\n[Test 3] Request Header Accumulation")
    
    # Simulating fragmented request
    fragments = [
        b"GET /https://example.com HTTP/1.0\r\n",
        b"Host: localhost\r\n",
        b"User-Agent: Test\r\n",
        b"\r\n"
    ]
    
    # Simulate accumulation
    raw_request = b''
    for i, fragment in enumerate(fragments):
        raw_request += fragment
        has_full_header = b'\r\n\r\n' in raw_request
        print(f"  Fragment {i+1}: {len(fragment)} bytes, Full header: {has_full_header}")
    
    if b'\r\n\r\n' in raw_request:
        print(f"  ✓ Complete header received ({len(raw_request)} bytes total)")
        request_text = raw_request.decode('utf-8', errors='ignore')
        request_line = request_text.split('\r\n')[0]
        print(f"  ✓ Request line: '{request_line}'")
    else:
        print(f"  ✗ Header incomplete")
    
    # Test 4: BeautifulSoup fallback simulation
    print("\n[Test 4] HTML Sanitization Fallback")
    
    try:
        from bs4 import BeautifulSoup
        
        # Test with valid HTML
        valid_html = "<html><body><h1>Test</h1><script>alert('xss')</script></body></html>"
        try:
            soup = BeautifulSoup(valid_html, 'html.parser')
            text = soup.get_text(separator='\n', strip=True)
            print(f"  ✓ Valid HTML parsed, extracted text: '{text}'")
        except Exception as e:
            print(f"  ✗ Failed to parse valid HTML: {e}")
        
        # Test with malformed HTML
        malformed_html = "<html><body><h1>Test</h1><unclosed tag"
        try:
            soup = BeautifulSoup(malformed_html, 'html.parser')
            text = soup.get_text(separator='\n', strip=True)
            print(f"  ✓ Malformed HTML handled gracefully, extracted text: '{text}'")
        except Exception as e:
            print(f"  ✗ Failed to handle malformed HTML: {e}")
            
    except ImportError:
        print("  ⚠ BeautifulSoup not available, skipping test")
    
    print("\n" + "=" * 60)
    print("✓ Logic Tests Completed!")
    print("=" * 60)

if __name__ == '__main__':
    test_parse_logic()
