#!/usr/bin/env python3
"""
Simple HTTP File Server for Testing RetroBrowser
Serves files from demo/ directory on port 8081
"""
import http.server
import socketserver
import os

PORT = 8081
# Navigate to parent directory first, then serve from demo/
DIRECTORY = "../demo"

class MyHTTPRequestHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=DIRECTORY, **kwargs)
    
    def end_headers(self):
        # Add CORS headers for cross-origin requests
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Cache-Control', 'no-store, no-cache, must-revalidate')
        super().end_headers()

if __name__ == '__main__':
    os.chdir(os.path.dirname(os.path.abspath(__file__)))
    
    with socketserver.TCPServer(("", PORT), MyHTTPRequestHandler) as httpd:
        print(f"=" * 60)
        print(f"  Simple File Server Running")
        print(f"=" * 60)
        print(f"  Listening on: http://localhost:{PORT}")
        print(f"  Serving from: {os.path.abspath(DIRECTORY)}/")
        print(f"  Press Ctrl+C to stop")
        print(f"=" * 60)
        print()
        print("Test URLs:")
        print(f"  - http://127.0.0.1:8080/http://127.0.0.1:{PORT}/simple_test.html")
        print(f"  - http://127.0.0.1:8080/http://127.0.0.1:{PORT}/test_content.html")
        print()
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nShutting down server...")
