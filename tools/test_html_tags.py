#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Count blocks from parser output
"""

import requests

print("Testing parser output...")

# Fetch textfiles.com
response = requests.get("http://localhost:8080/http://www.textfiles.com", timeout=10)
html = response.text

# Count tags in HTML
print(f"\nHTML contains:")
print(f"  <img> tags: {html.count('<img')}")
print(f"  <a> tags: {html.count('<a ')}")
print(f"  <table> tags: {html.count('<table')}")
print(f"  <td> tags: {html.count('<td')}")

# Show first IMG tag
import re
img_match = re.search(r'<img[^>]+>', html)
if img_match:
    print(f"\nFirst <img> tag:\n  {img_match.group(0)[:120]}")

# Show first A tag with href
a_match = re.search(r'<a [^>]*href[^>]+>.*?</a>', html, re.DOTALL)
if a_match:
    full_match = a_match.group(0)
    if len(full_match) > 150:
        full_match = full_match[:150] + '...'
    print(f"\nFirst <a> tag:\n  {full_match}")

print("\n" + "="*80)
print("If browser shows 0 images/few links, parser is dropping table content!")
print("="*80)
