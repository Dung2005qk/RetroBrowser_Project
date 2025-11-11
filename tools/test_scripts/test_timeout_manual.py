#!/usr/bin/env python3
"""
Manual test for timeout behavior with different timeout values
"""

import socket
import time

PROXY_HOST = '127.0.0.1'
PROXY_PORT = 8080

def test_with_timeout(timeout_seconds):
    """Test request with specific timeout"""
    print(f"\n{'='*60}")
    print(f"Testing with timeout: {timeout_seconds}s")
    print('='*60)
    
    try:
        start_time = time.time()
        
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(timeout_seconds)
        sock.connect((PROXY_HOST, PROXY_PORT))
        
        # Request to complex site
        request = "GET https://lite.cnn.com HTTP/1.1\r\n"
        request += "Host: lite.cnn.com\r\n"
        request += "Connection: close\r\n"
        request += "\r\n"
        
        sock.sendall(request.encode('utf-8'))
        print(f"Request sent at {time.time() - start_time:.3f}s")
        
        response = b""
        chunk_count = 0
        while True:
            try:
                chunk = sock.recv(4096)
                if not chunk:
                    break
                chunk_count += 1
                response += chunk
                if len(response) > 1000000:  # 1MB limit
                    break
            except socket.timeout:
                print(f"Socket timeout during recv() at {time.time() - start_time:.3f}s")
                break
        
        sock.close()
        elapsed = time.time() - start_time
        
        print(f"Elapsed time: {elapsed:.3f}s")
        print(f"Chunks received: {chunk_count}")
        print(f"Response size: {len(response)} bytes")
        
        if response:
            # Parse status line
            response_text = response.decode('utf-8', errors='ignore')
            status_line = response_text.split('\n')[0].strip()
            print(f"Status: {status_line}")
            return True
        else:
            print("Empty response (timeout too short)")
            return False
            
    except socket.timeout:
        elapsed = time.time() - start_time
        print(f"Connection timeout at {elapsed:.3f}s")
        return False
    except Exception as e:
        print(f"Error: {e}")
        return False

if __name__ == '__main__':
    print("TIMEOUT BEHAVIOR TEST")
    print("Testing different timeout values with complex page (lite.cnn.com)")
    
    timeouts = [0.1, 0.5, 1.0, 2.0, 5.0]
    
    for t in timeouts:
        result = test_with_timeout(t)
        print(f"Result: {'SUCCESS' if result else 'FAILED/TIMEOUT'}")
    
    print("\n" + "="*60)
    print("CONCLUSION:")
    print("- Very short timeouts (0.1s) may not be enough for complex pages")
    print("- Proxy needs time to: fetch upstream + sanitize + send back")
    print("- This is EXPECTED behavior, not a bug")
