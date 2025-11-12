#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Test script to verify CSS styling works correctly in proxy
"""

import requests
from bs4 import BeautifulSoup

PROXY_URL = "http://127.0.0.1:8080"

def test_css_preservation():
    """Test that proxy preserves style attributes"""
    print("=" * 60)
    print("TEST 1: CSS Style Attribute Preservation")
    print("=" * 60)
    
    # Test local CSS test file through proxy
    test_url = f"{PROXY_URL}/http://localhost:8000/demo/css_test.html"
    
    try:
        response = requests.get(test_url, timeout=10)
        soup = BeautifulSoup(response.text, 'html.parser')
        
        # Check if style attributes are preserved
        elements_with_style = soup.find_all(style=True)
        
        print(f"✓ Found {len(elements_with_style)} elements with style attribute")
        
        if len(elements_with_style) > 0:
            print("✓ PASS: Proxy preserves style attributes")
            
            # Show some examples
            print("\nSample styled elements:")
            for i, elem in enumerate(elements_with_style[:5]):
                print(f"  {i+1}. <{elem.name} style=\"{elem['style']}\">")
            
            return True
        else:
            print("✗ FAIL: No style attributes found!")
            return False
            
    except Exception as e:
        print(f"✗ ERROR: {e}")
        return False


def test_textfiles_com():
    """Test textfiles.com styling"""
    print("\n" + "=" * 60)
    print("TEST 2: textfiles.com CSS Analysis")
    print("=" * 60)
    
    test_url = f"{PROXY_URL}/http://www.textfiles.com"
    
    try:
        response = requests.get(test_url, timeout=10)
        soup = BeautifulSoup(response.text, 'html.parser')
        
        # Check for inline styles
        styled_elements = soup.find_all(style=True)
        
        print(f"\n✓ Total elements with inline styles: {len(styled_elements)}")
        
        # Look for specific colors
        colors_found = []
        for elem in styled_elements:
            style = elem.get('style', '')
            if 'color' in style.lower() or 'background' in style.lower():
                colors_found.append(f"<{elem.name}>: {style}")
        
        if colors_found:
            print(f"✓ Found {len(colors_found)} elements with color styling:")
            for color_style in colors_found[:10]:  # Show first 10
                print(f"  - {color_style}")
        else:
            print("⚠ No color styles found in inline attributes")
            print("  (May use <style> blocks or external CSS)")
        
        # Check page structure
        headers = soup.find_all(['h1', 'h2', 'h3'])
        links = soup.find_all('a')
        
        print(f"\n✓ Page structure:")
        print(f"  - Headers: {len(headers)}")
        print(f"  - Links: {len(links)}")
        
        # Show first header
        if headers:
            first_header = headers[0]
            print(f"\n  First header: <{first_header.name}>{first_header.get_text()[:50]}...</{first_header.name}>")
            if first_header.get('style'):
                print(f"  Style: {first_header['style']}")
        
        return True
        
    except Exception as e:
        print(f"✗ ERROR: {e}")
        return False


def check_proxy_running():
    """Check if proxy is running"""
    print("=" * 60)
    print("Checking Proxy Status")
    print("=" * 60)
    
    try:
        # Try to connect to proxy with a simple request
        response = requests.get(f"{PROXY_URL}/http://httpbin.org/html", timeout=5)
        print("✓ Proxy is running on http://127.0.0.1:8080")
        return True
    except:
        print("✗ Proxy is NOT running!")
        print("  Please start proxy: python src/proxy/proxy.py")
        return False


if __name__ == "__main__":
    print("\n" + "=" * 60)
    print("RetroBrowser CSS Styling Test Suite")
    print("=" * 60)
    print()
    
    # Check proxy first
    if not check_proxy_running():
        exit(1)
    
    print()
    
    # Run tests
    test1_passed = test_css_preservation()
    test2_passed = test_textfiles_com()
    
    # Summary
    print("\n" + "=" * 60)
    print("TEST SUMMARY")
    print("=" * 60)
    print(f"Test 1 (CSS Preservation): {'PASS ✓' if test1_passed else 'FAIL ✗'}")
    print(f"Test 2 (textfiles.com): {'PASS ✓' if test2_passed else 'FAIL ✗'}")
    print("=" * 60)
