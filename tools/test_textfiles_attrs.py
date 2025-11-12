#!/usr/bin/env python3
import requests
from bs4 import BeautifulSoup

r = requests.get('http://127.0.0.1:8080/http://www.textfiles.com', timeout=10)
soup = BeautifulSoup(r.text, 'html.parser')

print("=" * 70)
print("textfiles.com HTML ATTRIBUTE ANALYSIS")
print("=" * 70)

# Body tag
body = soup.find('body')
if body:
    print("\n<BODY> attributes:")
    for attr, value in body.attrs.items():
        print(f"  {attr}={value}")

# FONT tags
fonts = soup.find_all('font')
print(f"\n<FONT> tags found: {len(fonts)}")
for i, font in enumerate(fonts[:5]):
    print(f"  Font #{i+1}: {font.attrs}")

# TD with bgcolor
tds = soup.find_all('td', bgcolor=True)
print(f"\n<TD> tags with bgcolor: {len(tds)}")
for i, td in enumerate(tds[:5]):
    print(f"  TD #{i+1} bgcolor={td.get('bgcolor')}")

# TABLE tags
tables = soup.find_all('table')
print(f"\n<TABLE> tags found: {len(tables)}")

# Check raw HTML snippet
print("\n" + "=" * 70)
print("RAW HTML SNIPPET (first 1500 chars after <BODY>)")
print("=" * 70)
body_idx = r.text.upper().find('<BODY')
if body_idx != -1:
    snippet = r.text[body_idx:body_idx+1500]
    print(snippet)
