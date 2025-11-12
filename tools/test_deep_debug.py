#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Deep debugging test for textfiles.com rendering issue
Tests each step of the data flow: Proxy -> Parser -> Renderer
"""

import requests
from bs4 import BeautifulSoup
import sys
import os

# Add src/proxy to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'src', 'proxy'))
from config import ALLOWED_HTML_TAGS, ALLOWED_HTML_ATTRIBUTES

print("=" * 80)
print("DEEP DEBUG TEST - TEXTFILES.COM RENDERING")
print("=" * 80)

# Step 1: Fetch raw HTML
print("\n[STEP 1] Fetching raw HTML from http://www.textfiles.com")
print("-" * 80)
try:
    response = requests.get("http://www.textfiles.com", timeout=10)
    raw_html = response.text
    print(f"✓ Successfully fetched {len(raw_html)} bytes")
    print(f"  Status code: {response.status_code}")
except Exception as e:
    print(f"✗ FAILED to fetch: {e}")
    sys.exit(1)

# Step 2: Parse with BeautifulSoup
print("\n[STEP 2] Parsing HTML with BeautifulSoup")
print("-" * 80)
soup = BeautifulSoup(raw_html, 'html.parser')

# Step 3: Find BODY tag
print("\n[STEP 3] Analyzing <BODY> tag")
print("-" * 80)
body = soup.find('body')
if body:
    print(f"✓ Found <body> tag")
    print(f"  Tag name: {body.name}")
    print(f"  Attributes: {dict(body.attrs)}")
    
    # Check specific attributes
    for attr in ['bgcolor', 'text', 'link', 'alink', 'vlink']:
        value = body.get(attr)
        if value:
            print(f"    {attr}: '{value}'")
        else:
            print(f"    {attr}: NOT FOUND")
else:
    print("✗ NO <body> tag found!")
    sys.exit(1)

# Step 4: Sanitize HTML (simulate proxy)
print("\n[STEP 4] Sanitizing HTML (simulating proxy)")
print("-" * 80)

# Create sanitizer
def sanitize_html(html_content):
    soup = BeautifulSoup(html_content, 'html.parser')
    
    # Remove all tags not in whitelist
    for tag in soup.find_all(True):
        if tag.name not in ALLOWED_HTML_TAGS:
            tag.unwrap()
        else:
            # Remove attributes not in whitelist
            allowed_attrs = ALLOWED_HTML_ATTRIBUTES.get(tag.name, set()) | ALLOWED_HTML_ATTRIBUTES.get('*', set())
            attrs_to_remove = [attr for attr in tag.attrs if attr not in allowed_attrs]
            for attr in attrs_to_remove:
                del tag[attr]
    
    return str(soup)

sanitized_html = sanitize_html(raw_html)
print(f"✓ Sanitized HTML: {len(sanitized_html)} bytes")

# Parse sanitized HTML
sanitized_soup = BeautifulSoup(sanitized_html, 'html.parser')
sanitized_body = sanitized_soup.find('body')

print("\n[STEP 5] Analyzing sanitized <BODY> tag")
print("-" * 80)
if sanitized_body:
    print(f"✓ <body> tag still exists after sanitization")
    print(f"  Attributes after sanitization: {dict(sanitized_body.attrs)}")
    
    for attr in ['bgcolor', 'text', 'link', 'alink', 'vlink']:
        value = sanitized_body.get(attr)
        if value:
            print(f"    {attr}: '{value}'")
        else:
            print(f"    {attr}: REMOVED BY SANITIZATION!")
else:
    print("✗ <body> tag removed by sanitization!")

# Step 6: Show first 500 chars of sanitized HTML
print("\n[STEP 6] First 500 characters of sanitized HTML")
print("-" * 80)
print(sanitized_html[:500])

# Step 7: Count key elements
print("\n[STEP 7] Element counts in sanitized HTML")
print("-" * 80)
print(f"  <a> tags: {len(sanitized_soup.find_all('a'))}")
print(f"  <font> tags: {len(sanitized_soup.find_all('font'))}")
print(f"  <table> tags: {len(sanitized_soup.find_all('table'))}")
print(f"  <tr> tags: {len(sanitized_soup.find_all('tr'))}")
print(f"  <td> tags: {len(sanitized_soup.find_all('td'))}")
print(f"  <img> tags: {len(sanitized_soup.find_all('img'))}")
print(f"  <p> tags: {len(sanitized_soup.find_all('p'))}")
print(f"  <div> tags: {len(sanitized_soup.find_all('div'))}")
print(f"  <center> tags: {len(sanitized_soup.find_all('center'))}")

# Step 8: Create minimal test case
print("\n[STEP 8] Creating minimal test HTML file")
print("-" * 80)

# Extract only BODY attributes for minimal test
body_attrs = dict(sanitized_body.attrs) if sanitized_body else {}
attrs_str = ' '.join([f'{k}="{v}"' for k, v in body_attrs.items()])

minimal_html = f"""<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 3.2 Final//EN">
<html>
<head>
    <title>Minimal Test - BODY Colors</title>
</head>
<body {attrs_str}>
    <h1>Test Heading</h1>
    <p>This is a test paragraph. If you see GREEN text on BLACK background, colors work!</p>
    <p>If you see black text on white background, colors are NOT being applied.</p>
    <a href="test.html">Test Link</a>
</body>
</html>"""

minimal_file = os.path.join(os.path.dirname(__file__), '..', 'demo', 'minimal_color_test.html')
with open(minimal_file, 'w', encoding='utf-8') as f:
    f.write(minimal_html)

print(f"✓ Created minimal test file: {minimal_file}")
print(f"  BODY tag: <body {attrs_str}>")
print(f"\n  Load this file in RetroBrowser to test if colors work:")
print(f"  file:///{minimal_file.replace(chr(92), '/')}")

# Step 9: Parser simulation (C++ logic in Python)
print("\n[STEP 9] Simulating C++ parser color extraction")
print("-" * 80)

def parse_legacy_html_color(attr_value):
    """Simulate ParseLegacyHtmlColors C++ function"""
    if not attr_value:
        return None
    
    value = attr_value.strip()
    
    # Add # prefix if missing
    if value and value[0] != '#':
        value = '#' + value
    
    # Parse hex color
    if len(value) == 7:  # #RRGGBB
        try:
            r = int(value[1:3], 16)
            g = int(value[3:5], 16)
            b = int(value[5:7], 16)
            return f"RGB({r}, {g}, {b})"
        except ValueError:
            return None
    
    return None

if sanitized_body:
    print("Parser would extract these colors from <body>:")
    bgcolor_val = sanitized_body.get('bgcolor')
    text_val = sanitized_body.get('text')
    
    print(f"  bgcolor attribute: '{bgcolor_val}'")
    if bgcolor_val:
        parsed = parse_legacy_html_color(bgcolor_val)
        print(f"    → Parsed as: {parsed}")
    
    print(f"  text attribute: '{text_val}'")
    if text_val:
        parsed = parse_legacy_html_color(text_val)
        print(f"    → Parsed as: {parsed}")

# Final summary
print("\n" + "=" * 80)
print("SUMMARY")
print("=" * 80)
print(f"1. Raw HTML fetch: ✓ SUCCESS ({len(raw_html)} bytes)")
print(f"2. Original <body> tag: {'✓ FOUND' if body else '✗ NOT FOUND'}")
print(f"3. Sanitized HTML: ✓ SUCCESS ({len(sanitized_html)} bytes)")
print(f"4. Sanitized <body> tag: {'✓ EXISTS' if sanitized_body else '✗ REMOVED'}")
if sanitized_body:
    print(f"5. bgcolor attribute: {'✓ PRESERVED' if sanitized_body.get('bgcolor') else '✗ LOST'}")
    print(f"6. text attribute: {'✓ PRESERVED' if sanitized_body.get('text') else '✗ LOST'}")
print(f"\n7. Next step: Load minimal_color_test.html in RetroBrowser")
print(f"   - If GREEN on BLACK: ✓ Colors work, problem is with textfiles.com HTML")
print(f"   - If BLACK on WHITE: ✗ Colors broken, C++ parser/renderer issue")
print("=" * 80)
