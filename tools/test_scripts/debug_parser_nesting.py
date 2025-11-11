"""
Debug Parser Output - Check what parser actually produces
"""
import sys
sys.path.insert(0, 'src/proxy')

# Simulate HTML from proxy
html = """<html><head></head><body>
<title>http://info.cern.ch</title>

<h1>http://info.cern.ch - home of the first website</h1>
<p>From here you can:</p>
<ul>
<li><a href="http://info.cern.ch/hypertext/WWW/TheProject.html">Browse the first website</a></li>
<li><a href="http://info.cern.ch/hypertext/WWW/TheProject.html">Learn about the first website</a></li>
</ul>
</body></html>"""

print("=" * 70)
print("PARSER DEBUG - What C++ Parser Should Receive")
print("=" * 70)

print("\n[INPUT HTML]")
print("-" * 70)
print(html)

print("\n[EXPECTED BLOCKS BY PARSER]")
print("-" * 70)
print("BLOCK_TITLE: 'http://info.cern.ch'")
print("BLOCK_H1: 'http://info.cern.ch - home of the first website'")
print("BLOCK_P: 'From here you can:'")
print("BLOCK_UL: (container)")
print("BLOCK_LI: (empty, contains BLOCK_A)")
print("  BLOCK_A: 'Browse the first website' [href=...]")
print("BLOCK_LI: (empty, contains BLOCK_A)")
print("  BLOCK_A: 'Learn about the first website' [href=...]")

print("\n[KEY ISSUE]")
print("-" * 70)
print("⚠️  Parser may not handle NESTED tags correctly!")
print("   <li><a href='...'>TEXT</a></li>")
print("   Parser sees: <li> (no text) then <a>TEXT</a>")
print("   Result: LI block has EMPTY content!")
print("           A block has the actual text.")
print("\nProblem: Renderer expects LI.content to have text")
print("         But text is in nested <a> tag!")

print("\n[SOLUTION NEEDED]")
print("-" * 70)
print("Option 1: Parser extracts ALL text inside <li>, ignoring nested tags")
print("Option 2: Renderer draws nested blocks (complex)")
print("Option 3: Proxy removes <a> from inside <li> (flatten structure)")
print("\nRecommend: Option 3 (proxy flattens) - simplest fix")
