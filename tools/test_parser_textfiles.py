#!/usr/bin/env python3
"""
Test parser with actual textfiles.com HTML to see if content is being dropped
"""

import requests

# Fetch HTML from proxy
print("Fetching HTML from proxy...")
r = requests.get('http://127.0.0.1:8080/http://www.textfiles.com')
html = r.text

print(f"Received {len(html)} bytes from proxy")
print(f"\nFirst 500 chars:")
print("=" * 80)
print(html[:500])
print("=" * 80)

# Check if key content exists
checks = [
    ('<body bgcolor="#000000"', 'BODY tag with bgcolor'),
    ('text="00FF00"', 'BODY text attribute'),
    ('Where are the files?', 'Question 1'),
    ('Who are you?', 'Question 2'),
    ('Why does this matter?', 'Question 3'),
    ('What was it like?', 'Question 4'),
    ('How can I help?', 'Question 5'),
    ('<table', 'TABLE tags'),
    ('<img', 'IMG tags'),
    ('TEXTFILES.COM Sites', 'Footer banner'),
]

print("\n" + "=" * 80)
print("CONTENT VERIFICATION:")
print("=" * 80)
for needle, description in checks:
    found = needle.lower() in html.lower()
    status = "✓ FOUND" if found else "✗ MISSING"
    print(f"{status:12} | {description:30} | '{needle[:40]}'")

# Count tags
import re
print("\n" + "=" * 80)
print("TAG COUNTS:")
print("=" * 80)
for tag in ['body', 'table', 'tr', 'td', 'img', 'a', 'font', 'center']:
    count = len(re.findall(f'<{tag}[ >]', html, re.IGNORECASE))
    print(f"{tag.upper():8} | {count:3} occurrences")
