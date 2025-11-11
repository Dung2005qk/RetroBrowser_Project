#!/usr/bin/env python3
"""
Test image proxy endpoint with different URL formats
"""

import socket
from urllib.parse import quote

PROXY_HOST = '127.0.0.1'
PROXY_PORT = 8080

def test_image_endpoint(request_format, description):
    """Test image endpoint with specific request format"""
    print(f"\n{'='*70}")
    print(f"Testing: {description}")
    print(f"Request: {request_format}")
    print('='*70)
    
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(10)
        sock.connect((PROXY_HOST, PROXY_PORT))
        
        request = f"{request_format}\r\n"
        request += f"Host: {PROXY_HOST}\r\n"
        request += "Connection: close\r\n"
        request += "\r\n"
        
        print(f"Full request:\n{request}")
        
        sock.sendall(request.encode('utf-8'))
        
        # Receive response
        response = b""
        while True:
            chunk = sock.recv(4096)
            if not chunk:
                break
            response += chunk
            if len(response) > 100000:  # 100KB limit
                break
        
        sock.close()
        
        # Parse status
        response_text = response.decode('utf-8', errors='ignore')
        status_line = response_text.split('\n')[0].strip()
        
        print(f"Response status: {status_line}")
        
        if '200' in status_line:
            print("[OK] Image endpoint working")
            return True
        elif '404' in status_line:
            print("[INFO] Image not found (404) - endpoint works but image missing")
            return True
        elif '400' in status_line:
            print("[ERROR] Bad request (400) - endpoint not recognized")
            return False
        else:
            print(f"[WARNING] Unexpected status: {status_line}")
            return False
            
    except Exception as e:
        print(f"[ERROR] {e}")
        return False

if __name__ == '__main__':
    print("IMAGE PROXY ENDPOINT TEST")
    print(f"Testing proxy at {PROXY_HOST}:{PROXY_PORT}\n")
    
    # Check if proxy running
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(2)
        result = sock.connect_ex((PROXY_HOST, PROXY_PORT))
        sock.close()
        if result != 0:
            print("[ERROR] Proxy not running!")
            exit(1)
    except:
        print("[ERROR] Cannot connect to proxy")
        exit(1)
    
    print("[OK] Proxy is running\n")
    
    # Test different formats
    test_image_url = quote('http://www.textfiles.com/images/textfile.gif')
    
    tests = [
        (
            f"GET /image?url={test_image_url} HTTP/1.1",
            "Custom format (with leading slash)"
        ),
        (
            f"GET http://{PROXY_HOST}:{PROXY_PORT}/image?url={test_image_url} HTTP/1.1",
            "Standard proxy format (full URL to proxy)"
        ),
        (
            f"GET http://www.textfiles.com/image?url={test_image_url} HTTP/1.1",
            "Standard proxy format (full URL with image endpoint)"
        ),
    ]
    
    results = []
    for request_fmt, desc in tests:
        result = test_image_endpoint(request_fmt, desc)
        results.append((desc, result))
    
    print("\n" + "="*70)
    print("SUMMARY")
    print("="*70)
    
    for desc, result in results:
        status = "[OK]" if result else "[FAIL]"
        print(f"{status} {desc}")
    
    if all(r for _, r in results):
        print("\n[SUCCESS] All image endpoint formats working!")
    else:
        print(f"\n[WARNING] {sum(1 for _, r in results if not r)}/{len(results)} formats failed")
