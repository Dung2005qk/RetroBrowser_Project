#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Quick Test for Proxy URL Format Support
Tests both standard proxy format and custom format with leading slash
"""

import socket
import sys

PROXY_HOST = '127.0.0.1'
PROXY_PORT = 8080

def test_format(request_line, description):
    """Test a specific request format"""
    print(f"\n{'='*70}")
    print(f"Testing: {description}")
    print(f"Request: {request_line}")
    print('='*70)
    
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(10)
        sock.connect((PROXY_HOST, PROXY_PORT))
        
        # Build full request
        request = request_line + "\r\n"
        request += f"Host: example.com\r\n"
        request += "User-Agent: Test-Client/1.0\r\n"
        request += "Connection: close\r\n"
        request += "\r\n"
        
        print(f"Full request:\n{request}")
        
        # Send request
        sock.sendall(request.encode('utf-8'))
        
        # Receive response
        response = b""
        while True:
            chunk = sock.recv(4096)
            if not chunk:
                break
            response += chunk
            if len(response) > 100000:  # Limit response size
                break
        
        sock.close()
        
        # Parse response
        response_text = response.decode('utf-8', errors='ignore')
        lines = response_text.split('\n')
        status_line = lines[0].strip() if lines else "No response"
        
        print(f"\nResponse status: {status_line}")
        
        if '200' in status_line:
            print("✓ SUCCESS - Got 200 OK")
            # Show first few lines of body
            if '\r\n\r\n' in response_text:
                body = response_text.split('\r\n\r\n', 1)[1]
                body_preview = body[:200].replace('\n', ' ')
                print(f"Body preview: {body_preview}...")
            return True
        elif '400' in status_line:
            print("✗ FAILED - Got 400 Bad Request")
            return False
        else:
            print(f"? UNEXPECTED - Got {status_line}")
            return False
            
    except Exception as e:
        print(f"✗ ERROR: {str(e)}")
        return False

def main():
    print("="*70)
    print("PROXY FORMAT COMPATIBILITY TEST")
    print("="*70)
    print(f"Testing proxy at {PROXY_HOST}:{PROXY_PORT}")
    
    # Check if proxy is running
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(2)
        result = sock.connect_ex((PROXY_HOST, PROXY_PORT))
        sock.close()
        if result != 0:
            print("\n✗ ERROR: Proxy is not running!")
            print("Please start proxy: python src/proxy/proxy.py")
            sys.exit(1)
    except Exception as e:
        print(f"\n✗ ERROR: Cannot connect to proxy: {e}")
        sys.exit(1)
    
    print("✓ Proxy is running\n")
    
    # Test both formats
    results = []
    
    # Format 1: Standard proxy format (no leading slash)
    results.append(test_format(
        "GET http://example.com HTTP/1.1",
        "Standard Proxy Format (no leading slash)"
    ))
    
    # Format 2: Custom format with leading slash
    results.append(test_format(
        "GET /http://example.com HTTP/1.1",
        "Custom Format (with leading slash)"
    ))
    
    # Format 3: HTTPS with standard format
    results.append(test_format(
        "GET https://example.com HTTP/1.1",
        "Standard Proxy Format with HTTPS"
    ))
    
    # Format 4: HTTPS with slash
    results.append(test_format(
        "GET /https://example.com HTTP/1.1",
        "Custom Format with HTTPS and leading slash"
    ))
    
    # Summary
    print("\n" + "="*70)
    print("TEST SUMMARY")
    print("="*70)
    passed = sum(results)
    total = len(results)
    print(f"Passed: {passed}/{total}")
    
    if passed == total:
        print("\n✓ ALL TESTS PASSED - Both formats are supported!")
        sys.exit(0)
    else:
        print(f"\n✗ {total - passed} TEST(S) FAILED")
        sys.exit(1)

if __name__ == '__main__':
    main()
