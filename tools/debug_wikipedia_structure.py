"""
DEBUG SCRIPT: Phân tích chi tiết cấu trúc Wikipedia HTML
để tìm ra tại sao 90% content bị mất sau khi qua proxy sanitization.
"""

import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'src', 'proxy'))

from bs4 import BeautifulSoup
import requests
import re
from config import BLACKLISTED_TAGS, ALLOWED_HTML_TAGS, ALLOWED_HTML_ATTRIBUTES

def analyze_wikipedia_structure():
    """Download và phân tích cấu trúc Wikipedia desktop"""
    
    print("=" * 80)
    print("WIKIPEDIA STRUCTURE ANALYSIS")
    print("=" * 80)
    
    url = "https://en.wikipedia.org/wiki/Computer_network"
    print(f"\n[1] Downloading: {url}")
    response = requests.get(url, headers={'User-Agent': 'Mozilla/5.0'}, timeout=20)
    raw_html = response.text
    print(f"    Raw HTML size: {len(raw_html):,} bytes")
    
    soup = BeautifulSoup(raw_html, 'html.parser')
    
    # Find main content div
    # Apply same pre-processing as proxy
    print(f"\n[2] Pre-processing HTML (remove <link>, <meta> tags)...")
    raw_html = re.sub(r'<link[^>]*>', '', raw_html, flags=re.IGNORECASE)
    raw_html = re.sub(r'<meta[^>]*>', '', raw_html, flags=re.IGNORECASE)
    print(f"    After pre-process: {len(raw_html):,} bytes")
    
    # Re-parse with lxml
    soup = BeautifulSoup(raw_html, 'lxml')
    
    print(f"\n[3] Locating main content area...")
    content_div = soup.find('div', id='mw-content-text')
    if content_div:
        print(f"    ✓ Found #mw-content-text: {len(str(content_div)):,} bytes")
    else:
        print(f"    ✗ ERROR: #mw-content-text not found!")
        return
    
    parser_output = content_div.find('div', class_='mw-parser-output')
    if parser_output:
        print(f"    ✓ Found .mw-parser-output: {len(str(parser_output)):,} bytes")
    else:
        print(f"    ✗ ERROR: .mw-parser-output not found!")
        return
    
    # Analyze paragraph count
    print(f"\n[4] Analyzing paragraph content...")
    all_paragraphs = parser_output.find_all('p')
    print(f"    Total <p> tags: {len(all_paragraphs)}")
    
    substantial_paragraphs = [p for p in all_paragraphs if len(p.get_text(strip=True)) > 50]
    print(f"    Paragraphs with >50 chars: {len(substantial_paragraphs)}")
    
    if substantial_paragraphs:
        print(f"\n    SAMPLE - First paragraph content:")
        first_p = substantial_paragraphs[0]
        text = first_p.get_text(strip=True)[:200]
        print(f"    \"{text}...\"")
    
    # Check what tags wrap paragraphs
    print(f"\n[5] Analyzing paragraph parent tags...")
    parent_tags = {}
    for p in substantial_paragraphs[:20]:  # Check first 20
        parent = p.parent
        if parent:
            tag_name = parent.name
            classes = ' '.join(parent.get('class', []))
            key = f"<{tag_name} class='{classes}'>"
            parent_tags[key] = parent_tags.get(key, 0) + 1
    
    print("    Parent tag distribution:")
    for tag, count in sorted(parent_tags.items(), key=lambda x: -x[1])[:10]:
        print(f"      {count:2d}x {tag}")
    
    # Simulate sanitization
    print(f"\n[6] Simulating proxy sanitization...")
    
    # Step 1: Remove blacklisted tags
    blacklist_removed = 0
    for tag_name in BLACKLISTED_TAGS:
        tags = parser_output.find_all(tag_name)
        blacklist_removed += len(tags)
        for tag in tags:
            tag.decompose()
    print(f"    Removed {blacklist_removed} blacklisted tags")
    
    # Step 2: Tag whitelisting
    unwrapped = 0
    for tag in parser_output.find_all(True):
        if tag.name not in ALLOWED_HTML_TAGS:
            tag.unwrap()
            unwrapped += 1
    print(f"    Unwrapped {unwrapped} non-whitelisted tags")
    
    # Step 3: Attribute stripping
    attrs_removed = 0
    for tag in parser_output.find_all(True):
        allowed_attrs = ALLOWED_HTML_ATTRIBUTES.get(tag.name, set())
        current_attrs = list(tag.attrs.keys())
        for attr in current_attrs:
            if attr not in allowed_attrs:
                del tag[attr]
                attrs_removed += 1
    print(f"    Removed {attrs_removed} non-whitelisted attributes")
    
    # Check paragraphs after sanitization
    print(f"\n[7] Checking paragraphs AFTER sanitization...")
    remaining_paragraphs = parser_output.find_all('p')
    print(f"    Remaining <p> tags: {len(remaining_paragraphs)}")
    
    substantial_after = [p for p in remaining_paragraphs if len(p.get_text(strip=True)) > 50]
    print(f"    Paragraphs with >50 chars: {len(substantial_after)}")
    
    if substantial_after:
        print(f"\n    SAMPLE - First remaining paragraph:")
        text = substantial_after[0].get_text(strip=True)[:200]
        print(f"    \"{text}...\"")
    else:
        print(f"\n    ✗ NO SUBSTANTIAL PARAGRAPHS REMAINING!")
        print(f"\n    Checking what happened to original first paragraph...")
        
        # Re-download and trace
        response2 = requests.get(url, headers={'User-Agent': 'Mozilla/5.0'}, timeout=20)
        raw_html2 = response2.text
        raw_html2 = re.sub(r'<link[^>]*>', '', raw_html2, flags=re.IGNORECASE)
        raw_html2 = re.sub(r'<meta[^>]*>', '', raw_html2, flags=re.IGNORECASE)
        soup2 = BeautifulSoup(raw_html2, 'lxml')
        parser_output2 = soup2.find('div', class_='mw-parser-output')
        first_p_original = None
        for p in parser_output2.find_all('p'):
            if len(p.get_text(strip=True)) > 50:
                first_p_original = p
                break
        
        if first_p_original:
            print(f"\n    Original paragraph HTML:")
            print(f"    {str(first_p_original)[:300]}")
            print(f"\n    Parent chain:")
            parent = first_p_original.parent
            depth = 0
            while parent and depth < 5:
                classes = ' '.join(parent.get('class', []))
                tag_id = parent.get('id', '')
                print(f"      {'  ' * depth}<{parent.name} id='{tag_id}' class='{classes}'>")
                
                # Check if parent is in blacklist
                if parent.name in BLACKLISTED_TAGS:
                    print(f"      {'  ' * depth}  ⚠️  BLACKLISTED TAG - Will decompose with all content!")
                    break
                
                parent = parent.parent
                depth += 1
    
    # Final statistics
    print(f"\n[8] Final statistics...")
    sanitized_html = str(parser_output)
    print(f"    Sanitized HTML size: {len(sanitized_html):,} bytes")
    print(f"    Size reduction: {100 - (len(sanitized_html) / len(str(content_div)) * 100):.1f}%")
    
    print(f"\n" + "=" * 80)
    print("ANALYSIS COMPLETE")
    print("=" * 80)

if __name__ == "__main__":
    analyze_wikipedia_structure()
