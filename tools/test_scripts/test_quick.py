import sys
sys.path.insert(0, 'src/proxy')
from proxy import ProxyRequestHandler

print("=" * 70)
print("TEST: HTML Structure Tags Preservation")
print("=" * 70)

html = '<html><head><title>Test</title></head><body><h1>Hi</h1></body></html>'
# Create minimal handler instance
class FakeSocket:
    def sendall(self, data): pass
    def close(self): pass

class FakeServer:
    pass

handler = ProxyRequestHandler(FakeSocket(), ('127.0.0.1', 0), FakeServer())
result = handler._sanitize_html(html, 'http://test.com')

print('\nINPUT:', html)
print('\nOUTPUT:', result)
print('\nSTRUCTURE CHECK:')

tags = ['<html', '<head', '<body', '<title']
all_ok = True
for t in tags:
    if t in result:
        print(f'✅ {t}> PRESERVED')
    else:
        print(f'❌ {t}> MISSING')
        all_ok = False

print('\n' + '=' * 70)
if all_ok:
    print('✅ SUCCESS - All structure tags preserved!')
else:
    print('❌ FAILED - Some structure tags missing')
