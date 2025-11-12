#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Analyze textfiles.com HTML structure to find missing content
"""

import requests
from bs4 import BeautifulSoup
import sys
import os

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'src', 'proxy'))
from config import ALLOWED_HTML_TAGS, ALLOWED_HTML_ATTRIBUTES

print("=" * 80)
print("ANALYZING TEXTFILES.COM STRUCTURE")
print("=" * 80)

# Fetch via proxy
response = requests.get("http://localhost:8080/http://www.textfiles.com", timeout=10)
html = response.text

soup = BeautifulSoup(html, 'html.parser')

# Count all elements
print("\n[ELEMENT COUNTS]")
print(f"  <a> tags: {len(soup.find_all('a'))}")
print(f"  <img> tags: {len(soup.find_all('img'))}")
print(f"  <font> tags: {len(soup.find_all('font'))}")
print(f"  <table> tags: {len(soup.find_all('table'))}")
print(f"  <tr> tags: {len(soup.find_all('tr'))}")
print(f"  <td> tags: {len(soup.find_all('td'))}")
print(f"  <center> tags: {len(soup.find_all('center'))}")
print(f"  <p> tags: {len(soup.find_all('p'))}")

# Analyze images
print("\n[IMAGE ANALYSIS]")
images = soup.find_all('img')
for i, img in enumerate(images):
    src = img.get('src', '')
    alt = img.get('alt', '')
    print(f"  Image {i+1}: src='{src}' alt='{alt}'")

# Analyze links
print("\n[LINK ANALYSIS]")
links = soup.find_all('a')
print(f"Total links: {len(links)}")
for i, link in enumerate(links[:10]):  # First 10
    href = link.get('href', '')
    text = link.get_text(strip=True)
    print(f"  Link {i+1}: href='{href}' text='{text[:30]}'")

# Analyze table structure
print("\n[TABLE STRUCTURE]")
tables = soup.find_all('table')
for ti, table in enumerate(tables):
    rows = table.find_all('tr')
    print(f"\nTable {ti+1}: {len(rows)} rows")
    for ri, row in enumerate(rows[:3]):  # First 3 rows
        cells = row.find_all(['td', 'th'])
        print(f"  Row {ri+1}: {len(cells)} cells")
        for ci, cell in enumerate(cells):
            content = cell.get_text(strip=True)[:40]
            # Check what's inside
            imgs = cell.find_all('img')
            links = cell.find_all('a')
            fonts = cell.find_all('font')
            print(f"    Cell {ci+1}: {len(imgs)} imgs, {len(links)} links, {len(fonts)} fonts - '{content}'")

# Check CENTER tags
print("\n[CENTER TAGS]")
centers = soup.find_all('center')
print(f"Total <center> tags: {len(centers)}")
for i, center in enumerate(centers):
    # What's inside?
    tables = center.find_all('table')
    imgs = center.find_all('img')
    links = center.find_all('a')
    fonts = center.find_all('font')
    text = center.get_text(strip=True)[:60]
    print(f"  Center {i+1}: {len(tables)} tables, {len(imgs)} imgs, {len(links)} links, {len(fonts)} fonts")
    print(f"    Text: '{text}'")

# Find the big logo image
print("\n[LOGO IMAGE]")
logo_img = soup.find('img', alt='textfiles.com')
if logo_img:
    print(f"  Found logo: src='{logo_img.get('src')}' width={logo_img.get('width')} height={logo_img.get('height')}")
else:
    print("  Logo NOT FOUND!")

print("\n" + "=" * 80)
