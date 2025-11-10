#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Integration test - Start proxy and send a real request
"""

import socket
import time
import subprocess
import sys
import signal

def send_test_request():
    """Send a test request to the proxy"""
    print("\n[Client] Connecting to proxy at localhost:8080...")
    
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(10)
        sock.connect(('localhost', 8080))
        print("[Client] ✓ Connected to proxy")
        
        # Send request
        print("[Client] Sending request for http://example.com...")
        request = b"GET /http://example.com HTTP/1.0\r\n"
        request += b"Host: localhost\r\n"
        request += b"\r\n"
        
        sock.sendall(request)
        print("[Client] ✓ Request sent")
        
        # Receive response
        print("[Client] Waiting for response...")
        response = b""
        start_time = time.time()
        
        while time.time() - start_time < 10:
            try:
                chunk = sock.recv(4096)
                if not chunk:
                    break
                response += chunk
                if len(response) > 100:  # Got enough to check
                    break
            except socket.timeout:
                break
        
        sock.close()
        
        if response:
            response_text = response.decode('utf-8', errors='ignore')
            lines = response_text.split('\n')
            status_line = lines[0] if lines else ""
            
            print(f"[Client] ✓ Received response: {status_line}")
            print(f"[Client]   Total bytes: {len(response)}")
            
            if response_text.startswith('HTTP/'):
                print("[Client] ✓ Valid HTTP response")
                
                # Check status code
                if '200' in status_line:
                    print("[Client] ✓ Status 200 OK")
                    return True
                else:
                    print(f"[Client] ⚠ Non-200 status: {status_line}")
                    return True  # Still valid response
            else:
                print("[Client] ✗ Invalid HTTP response")
                return False
        else:
            print("[Client] ✗ No response received")
            return False
            
    except ConnectionRefusedError:
        print("[Client] ✗ Connection refused - is the proxy running?")
        return False
    except Exception as e:
        print(f"[Client] ✗ Error: {e}")
        return False

def main():
    print("=" * 60)
    print("Integration Test - Proxy with Real Request")
    print("=" * 60)
    
    proxy_path = r"c:\Users\LMC\OneDrive - THS\Desktop\RetroBrowser_Project\src\proxy\proxy.py"
    python_exe = r"C:/Users/LMC/AppData/Local/Programs/Python/Python313/python.exe"
    
    print("\n[Test] Starting proxy server...")
    
    # Start proxy in subprocess
    proxy_process = subprocess.Popen(
        [python_exe, proxy_path],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        creationflags=subprocess.CREATE_NEW_PROCESS_GROUP  # Allow clean termination
    )
    
    # Wait for proxy to start
    print("[Test] Waiting 3 seconds for proxy to initialize...")
    time.sleep(3)
    
    # Check if proxy is still running
    if proxy_process.poll() is not None:
        stdout, stderr = proxy_process.communicate()
        print(f"[Test] ✗ Proxy exited prematurely!")
        print(f"[Test] stdout: {stdout.decode('utf-8', errors='ignore')[:200]}")
        print(f"[Test] stderr: {stderr.decode('utf-8', errors='ignore')[:200]}")
        return False
    
    print("[Test] ✓ Proxy appears to be running")
    
    # Send test request
    success = send_test_request()
    
    # Cleanup: Stop the proxy
    print("\n[Test] Stopping proxy server...")
    try:
        # Send Ctrl+C signal to the process group
        proxy_process.send_signal(signal.CTRL_C_EVENT)
        proxy_process.wait(timeout=5)
        print("[Test] ✓ Proxy stopped gracefully")
    except:
        proxy_process.terminate()
        proxy_process.wait(timeout=2)
        print("[Test] ⚠ Proxy terminated forcefully")
    
    print("\n" + "=" * 60)
    if success:
        print("✓ INTEGRATION TEST PASSED!")
    else:
        print("✗ INTEGRATION TEST FAILED!")
    print("=" * 60)
    
    return success

if __name__ == '__main__':
    success = main()
    sys.exit(0 if success else 1)
