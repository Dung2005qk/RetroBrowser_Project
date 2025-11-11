"""
Test HTML Structure Tags Preservation After Sanitization
=========================================================
Verify that <html>, <head>, <body>, <title> tags are preserved after proxy sanitization.
"""

import sys
import os

# Change to proxy directory so config.py can be imported
proxy_dir = os.path.join(os.path.dirname(__file__), '..', '..', 'src', 'proxy')
os.chdir(proxy_dir)
sys.path.insert(0, proxy_dir)

from proxy import RetroBrowserProxy
from io import StringIO


def test_structure_preservation():
    """Test that essential HTML structure tags are preserved"""
    
    print("=" * 70)
    print("TEST: HTML Structure Tags Preservation")
    print("=" * 70)
    
    # Sample HTML with structure
    test_html = """
    <html>
    <head>
        <title>Test Page</title>
    </head>
    <body>
        <h1>Welcome</h1>
        <p>This is a test.</p>
    </body>
    </html>
    """
    
    proxy = RetroBrowserProxy()
    sanitized = proxy._sanitize_html(test_html)
    
    print("\n[INPUT HTML]")
    print("-" * 70)
    print(test_html.strip())
    
    print("\n[OUTPUT HTML]")
    print("-" * 70)
    print(sanitized.strip())
    
    # Check for essential tags
    essential_tags = ['<html', '<head', '<body', '<title']
    missing_tags = []
    
    print("\n[STRUCTURE TAG CHECK]")
    print("-" * 70)
    for tag in essential_tags:
        if tag in sanitized:
            print(f"✅ {tag}> PRESERVED")
        else:
            print(f"❌ {tag}> MISSING")
            missing_tags.append(tag)
    
    print("\n[RESULT]")
    print("=" * 70)
    if missing_tags:
        print(f"❌ FAILED - Missing tags: {', '.join(missing_tags)}")
        return False
    else:
        print("✅ SUCCESS - All structure tags preserved!")
        return True


if __name__ == '__main__':
    success = test_structure_preservation()
    sys.exit(0 if success else 1)
