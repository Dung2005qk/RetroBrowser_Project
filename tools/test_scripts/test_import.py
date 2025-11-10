#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Quick syntax and import check for proxy.py
"""

import sys
import importlib.util

def test_import():
    """Test if proxy.py can be imported without errors"""
    print("=" * 60)
    print("Testing proxy.py - Import and Syntax Check")
    print("=" * 60)
    
    proxy_path = r"c:\Users\LMC\OneDrive - THS\Desktop\RetroBrowser_Project\src\proxy\proxy.py"
    
    try:
        print("\n[1] Loading module specification...")
        spec = importlib.util.spec_from_file_location("proxy", proxy_path)
        if spec is None:
            print("✗ Failed to load module spec")
            return False
        print("✓ Module spec loaded")
        
        print("\n[2] Creating module from spec...")
        proxy_module = importlib.util.module_from_spec(spec)
        if proxy_module is None:
            print("✗ Failed to create module")
            return False
        print("✓ Module created")
        
        print("\n[3] Adding module to sys.modules...")
        sys.modules["proxy"] = proxy_module
        print("✓ Module added to sys.modules")
        
        print("\n[4] Executing module (importing)...")
        # This will actually import and run the module-level code
        # but NOT the if __name__ == '__main__' block
        spec.loader.exec_module(proxy_module)
        print("✓ Module executed successfully")
        
        print("\n[5] Checking module contents...")
        required_items = [
            'ProxyRequestHandler',
            'CACHE',
            'CACHE_LOCK',
            'CLIENT_SEMAPHORE'
        ]
        
        for item in required_items:
            if hasattr(proxy_module, item):
                print(f"  ✓ {item} found")
            else:
                print(f"  ✗ {item} NOT found")
                return False
        
        print("\n[6] Checking ProxyRequestHandler methods...")
        handler_class = proxy_module.ProxyRequestHandler
        required_methods = [
            'handle',
            '_parse_client_request',
            '_validate_request',
            '_fetch_upstream',
            '_sanitize_html',
            '_send_http_response',
            '_send_error_response'
        ]
        
        for method in required_methods:
            if hasattr(handler_class, method):
                print(f"  ✓ {method} found")
            else:
                print(f"  ✗ {method} NOT found")
                return False
        
        print("\n" + "=" * 60)
        print("✓ ALL CHECKS PASSED!")
        print("=" * 60)
        return True
        
    except SyntaxError as e:
        print(f"\n✗ SYNTAX ERROR: {e}")
        print(f"  Line {e.lineno}: {e.text}")
        return False
    except ImportError as e:
        print(f"\n✗ IMPORT ERROR: {e}")
        return False
    except Exception as e:
        print(f"\n✗ UNEXPECTED ERROR: {e}")
        import traceback
        traceback.print_exc()
        return False

if __name__ == '__main__':
    success = test_import()
    sys.exit(0 if success else 1)
