#!/usr/bin/env python3
"""
Diagnostic Test - Check why browser shows "Done" but blank screen

This test will:
1. Request a simple page through proxy
2. Analyze the HTML response
3. Check if content is suitable for rendering
4. Identify potential rendering issues
"""

import socket
import sys

PROXY_HOST = '127.0.0.1'
PROXY_PORT = 8080

def test_url(url, description):
    """Test a URL and analyze response for rendering issues"""
    print(f"\n{'='*70}")
    print(f"Testing: {description}")
    print(f"URL: {url}")
    print('='*70)
    
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(10)
        sock.connect((PROXY_HOST, PROXY_PORT))
        
        # Build request
        request = f"GET {url} HTTP/1.1\r\n"
        request += f"Host: {url.split('/')[2]}\r\n"
        request += "User-Agent: Mozilla/4.0 (compatible; MSIE 5.0; Windows 98)\r\n"
        request += "Connection: close\r\n"
        request += "\r\n"
        
        sock.sendall(request.encode('utf-8'))
        
        # Receive response
        response = b""
        while True:
            chunk = sock.recv(4096)
            if not chunk:
                break
            response += chunk
            if len(response) > 5000000:  # 5MB limit
                break
        
        sock.close()
        
        # Parse response
        response_text = response.decode('utf-8', errors='ignore')
        
        # Split headers and body
        if '\r\n\r\n' in response_text:
            header, body = response_text.split('\r\n\r\n', 1)
        else:
            header, body = response_text.split('\n\n', 1)
        
        status_line = header.split('\n')[0]
        print(f"Status: {status_line}")
        
        if '200' not in status_line:
            print(f"[WARNING] Non-200 response")
            return False
        
        # Analyze body content
        body_lower = body.lower()
        
        print(f"\nContent Analysis:")
        print(f"  Total size: {len(body)} bytes")
        
        # Check for actual content
        has_html = '<html' in body_lower or '<!doctype' in body_lower
        has_body = '<body' in body_lower
        has_text = any(tag in body_lower for tag in ['<p', '<h1', '<h2', '<div', '<span'])
        
        print(f"  Has HTML tags: {has_html}")
        print(f"  Has <body>: {has_body}")
        print(f"  Has text content tags: {has_text}")
        
        # Count content
        p_count = body_lower.count('<p')
        h_count = sum(body_lower.count(f'<h{i}') for i in range(1, 7))
        div_count = body_lower.count('<div')
        a_count = body_lower.count('<a ')
        
        print(f"\n  Paragraph tags: {p_count}")
        print(f"  Heading tags: {h_count}")
        print(f"  Div tags: {div_count}")
        print(f"  Link tags: {a_count}")
        
        # Check for problematic content
        has_script = '<script' in body_lower
        has_style = '<style' in body_lower
        has_css_link = 'stylesheet' in body_lower
        
        print(f"\n  Has <script>: {has_script} (should be removed by proxy)")
        print(f"  Has <style>: {has_style} (should be removed by proxy)")
        print(f"  Has CSS links: {has_css_link}")
        
        # Check for images
        img_count = body_lower.count('<img')
        has_image_proxy = '/image?url=' in body.lower()
        
        print(f"\n  Image tags: {img_count}")
        print(f"  Has image proxy URLs: {has_image_proxy}")
        
        # Extract text preview (first 500 chars of visible text)
        # Simple approach: remove all tags
        import re
        text_only = re.sub(r'<[^>]+>', '', body)
        text_only = ' '.join(text_only.split())  # Normalize whitespace
        preview = text_only[:500]
        
        print(f"\nText Preview (first 500 chars):")
        print(f"  {preview}")
        
        # Potential issues
        print(f"\nPotential Issues:")
        issues = []
        
        if len(body) < 100:
            issues.append("Body too short (< 100 bytes) - may be empty page")
        
        if not has_text and not has_html:
            issues.append("No HTML content detected - may be plain text or error page")
        
        if p_count == 0 and h_count == 0 and div_count == 0:
            issues.append("No paragraphs, headings, or divs - content may not render")
        
        if has_script:
            issues.append("WARNING: Scripts not fully removed by proxy")
        
        if len(text_only.strip()) < 50:
            issues.append("Very little visible text (< 50 chars) - page may appear blank")
        
        if issues:
            for issue in issues:
                print(f"  [!] {issue}")
        else:
            print(f"  [OK] No obvious issues detected")
        
        return True
        
    except Exception as e:
        print(f"[ERROR] {e}")
        return False

if __name__ == '__main__':
    print("="*70)
    print("RENDERING DIAGNOSTIC TEST")
    print("="*70)
    print(f"Proxy: {PROXY_HOST}:{PROXY_PORT}")
    
    # Check if proxy is running
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(2)
        result = sock.connect_ex((PROXY_HOST, PROXY_PORT))
        sock.close()
        if result != 0:
            print("\n[ERROR] Proxy is not running!")
            print("Please start: python src/proxy/proxy.py")
            sys.exit(1)
    except:
        print("\n[ERROR] Cannot connect to proxy")
        sys.exit(1)
    
    print("[OK] Proxy is running\n")
    
    # Test URLs in order of complexity
    tests = [
        ('http://example.com', 'Simple test page (example.com)'),
        ('http://www.textfiles.com/', 'Text-heavy site (from screenshot)'),
        ('https://example.com', 'HTTPS version'),
    ]
    
    for url, desc in tests:
        success = test_url(url, desc)
        if not success:
            print(f"\n[FAILED] {desc}")
    
    print("\n" + "="*70)
    print("DIAGNOSTIC COMPLETE")
    print("="*70)
    print("\nIf pages have content but browser shows blank:")
    print("  1. Check renderer initialization")
    print("  2. Check CalculateLayout is called")
    print("  3. Check m_displayList is populated")
    print("  4. Add debug logging to renderer.cpp")
