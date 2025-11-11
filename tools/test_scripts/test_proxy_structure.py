"""
Quick Test: Verify HTML Structure Tags via Proxy
"""
import requests

print("=" * 70)
print("PROXY TEST: HTML Structure Tags")
print("=" * 70)

proxy_url = "http://127.0.0.1:8080"
test_url = "http://info.cern.ch"

print(f"\n[REQUEST]")
print(f"  Proxy: {proxy_url}")
print(f"  Target: {test_url}")

try:
    # Use proxy
    response = requests.get(test_url, proxies={"http": proxy_url}, timeout=10)
    
    print(f"\n[RESPONSE]")
    print(f"  Status: {response.status_code}")
    print(f"  Size: {len(response.text)} bytes")
    
    # Check for structure tags
    html = response.text
    
    print(f"\n[STRUCTURE CHECK]")
    tags_to_check = ['<html', '<head', '<body', '<title']
    all_ok = True
    
    for tag in tags_to_check:
        if tag in html.lower():
            print(f"  ✅ {tag}> FOUND")
        else:
            print(f"  ❌ {tag}> MISSING")
            all_ok = False
    
    # Show first 200 chars
    print(f"\n[HTML PREVIEW]")
    print("-" * 70)
    print(html[:200])
    print("...")
    print("-" * 70)
    
    if all_ok:
        print("\n✅ SUCCESS - All structure tags preserved!")
        exit(0)
    else:
        print("\n❌ FAILED - Some structure tags missing!")
        exit(1)
        
except Exception as e:
    print(f"\n❌ ERROR: {e}")
    exit(1)
