#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Simple HTTP server for testing RetroBrowser with local files
Serves demo files on http://localhost:9999/
"""

import os
import sys
from http.server import HTTPServer, SimpleHTTPRequestHandler

class MyHTTPRequestHandler(SimpleHTTPRequestHandler):
    def do_GET(self):
        print(f"[GET] {self.path}")
        super().do_GET()

if __name__ == '__main__':
    # Change to demo directory
    demo_dir = os.path.join(os.path.dirname(__file__), '..', 'demo')
    os.chdir(demo_dir)
    print(f"Serving from: {os.getcwd()}")
    print(f"URL: http://localhost:9999/minimal_color_test.html")
    
    server = HTTPServer(('0.0.0.0', 9999), MyHTTPRequestHandler)
    print("Server started on port 9999")
    print("Press Ctrl+C to stop")
    
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nServer stopped")
