// Test file to verify if errors are real compilation errors or just IntelliSense issues
// This file simulates the includes and usage patterns from ui.cpp

#include "src/browser/core/stdafx.h"
#include "src/browser/ui/ui.h"
#include "src/browser/renderer/renderer.h"
#include "src/browser/parser/parser.h"

// Test 1: typedef ParsedPageData
typedef Parser::ParseResult ParsedPageData;

// Test 2: Can we use Renderer::HtmlRenderer?
void TestRenderer() {
    Renderer::HtmlRenderer* pRenderer = new Renderer::HtmlRenderer();
    delete pRenderer;
}

// Test 3: Can we use ParsedPageData members?
void TestParsedPageData() {
    ParsedPageData* pData = new ParsedPageData();
    
    // Test accessing status
    if (pData->status == Parser::PARSE_SUCCESS) {
        // Test accessing blocks
        for (size_t i = 0; i < pData->blocks.size(); i++) {
            if (pData->blocks[i].type == Parser::BLOCK_H1) {
                const char* content = pData->blocks[i].content.c_str();
            }
        }
    }
    
    delete pData;
}

// Test 4: Can we use SetWindowSubclass and DefSubclassProc?
LRESULT CALLBACK TestSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, 
                                  UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void TestSubclass(HWND hWnd) {
    SetWindowSubclass(hWnd, TestSubclassProc, 0, 0);
    RemoveWindowSubclass(hWnd, TestSubclassProc, 0);
}

// Test 5: Test MAKEINTRESOURCE for IDC_HAND
void TestCursor() {
    HCURSOR hCursor = LoadCursor(NULL, MAKEINTRESOURCE(32649));
}

int main() {
    // If this compiles, all the "errors" are IntelliSense false positives
    return 0;
}
