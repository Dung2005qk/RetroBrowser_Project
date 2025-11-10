#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Simple test script for the proxy server
"""

import socket
import time

def test_proxy():
    """Test basic proxy functionality"""
    print("=" * 60)
    print("Testing Proxy Server")
    print("=" * 60)
    
    # Test 1: Connect to proxy
    print("\n[Test 1] Connecting to proxy at localhost:8080...")
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5)
        sock.connect(('localhost', 8080))
        print("✓ Successfully connected to proxy")
    except Exception as e:
        print(f"✗ Failed to connect: {e}")
        return
    
    # Test 2: Send a simple HTTP request
    print("\n[Test 2] Sending HTTP GET request for http://example.com...")
    try:
        request = b"GET /http://example.com HTTP/1.0\r\n"
        request += b"Host: localhost\r\n"
        request += b"\r\n"
        
        sock.sendall(request)
        print("✓ Request sent successfully")
        
        # Receive response
        print("\n[Test 3] Receiving response...")
        response = b""
        while True:
            try:
                chunk = sock.recv(4096)
                if not chunk:
                    break
                response += chunk
            except socket.timeout:
                break
        
        if response:
            response_text = response.decode('utf-8', errors='ignore')
            lines = response_text.split('\n')
            status_line = lines[0] if lines else "No status line"
            print(f"✓ Received response: {status_line}")
            print(f"  Total bytes: {len(response)}")
            
            # Check if it's a valid HTTP response
            if response_text.startswith('HTTP/'):
                print("✓ Valid HTTP response received")
                
                # Show first 200 chars of body
                if '\r\n\r\n' in response_text:
                    body_start = response_text.find('\r\n\r\n') + 4
                    body_preview = response_text[body_start:body_start+200]
                    print(f"\n  Body preview:\n  {body_preview}...")
            else:
                print("✗ Invalid HTTP response")
        else:
            print("✗ No response received")
            
    except Exception as e:
        print(f"✗ Error during request: {e}")
    finally:
        sock.close()
        print("\n[Test 4] Connection closed")
    
    print("\n" + "=" * 60)
    print("Test completed!")
    print("=" * 60)

if __name__ == '__main__':
    test_proxy()
