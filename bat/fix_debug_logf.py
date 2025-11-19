#!/usr/bin/env python3
"""
Fix all DEBUG_LOGF calls in source files for VC++6.0 compatibility
"""

import re
import os

files_to_fix = [
    "src/browser/core/main.cpp",
    "src/browser/network/network.cpp"
]

for filepath in files_to_fix:
    if not os.path.exists(filepath):
        print(f"Warning: {filepath} not found")
        continue
    
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    original_content = content
    
    # Replace DEBUG_LOGF with comment
    # Match DEBUG_LOGF(...); including multi-line calls
    content = re.sub(
        r'DEBUG_LOGF\s*\([^;]+\);',
        '// DEBUG_LOGF removed for VC++6.0 compat',
        content,
        flags=re.MULTILINE | re.DOTALL
    )
    
    if content != original_content:
        with open(filepath, 'w', encoding='utf-8', newline='') as f:
            f.write(content)
        print(f"✓ Fixed {filepath}")
    else:
        print(f"- No changes needed in {filepath}")

print("\nDone!")
