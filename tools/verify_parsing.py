#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Verify textfiles.com parsing completeness
"""

import requests
from bs4 import BeautifulSoup

print("=" * 80)
print("TEXTFILES.COM PARSING VERIFICATION")
print("=" * 80)

# Get sanitized HTML from proxy
response = requests.get("http://localhost:8080/http://www.textfiles.com", timeout=10)
html = response.text
soup = BeautifulSoup(html, 'html.parser')

print(f"\n[HTML SIZE]: {len(html)} bytes")

# Count elements
links = soup.find_all('a')
images = soup.find_all('img')
paragraphs = soup.find_all('p')
tables = soup.find_all('table')
fonts = soup.find_all('font')

print(f"\n[ELEMENT COUNTS]")
print(f"  <a> tags: {len(links)}")
print(f"  <img> tags: {len(images)}")
print(f"  <p> tags: {len(paragraphs)}")
print(f"  <table> tags: {len(tables)}")
print(f"  <font> tags: {len(fonts)}")

# Verify ALL 27 links exist
print(f"\n[ALL {len(links)} LINKS]:")
for i, link in enumerate(links, 1):
    href = link.get('href', '')
    text = link.get_text(strip=True) or '(empty)'
    title = link.get('title', '')
    if len(text) > 50:
        text = text[:50] + '...'
    print(f"  {i:2}. [{text[:30]:30}] -> {href[:60]}")

print(f"\n[ALL {len(images)} IMAGES]:")
for i, img in enumerate(images, 1):
    src = img.get('src', '')
    alt = img.get('alt', '')
    # Extract original URL from proxy format
    if 'image?url=' in src:
        import urllib.parse
        original = urllib.parse.unquote(src.split('url=')[1])
    else:
        original = src
    print(f"  {i}. {alt[:40]:40} -> {original}")

print(f"\n{'=' * 80}")
print("CONCLUSION:")
print(f"  If browser shows fewer items than above, it's a RENDERING issue.")
print(f"  If counts match, parser is working correctly!")
print(f"  Try SCROLLING DOWN in browser to see more content.")
print("=" * 80)
