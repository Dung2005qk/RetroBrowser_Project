// Quick parser test - verify LI/A nesting
#include <iostream>
#include <string>
#include <vector>
#include "../src/browser/parser/parser.h"

int main() {
    // Test HTML with nested <li><a>
    std::string html = R"(
<html>
<h1>Test Header</h1>
<p>Test paragraph</p>
<ul>
<li><a href="http://example.com">Link text here</a></li>
<li><a href="http://example2.com">Second link</a></li>
</ul>
</html>
)";

    Parser::HtmlParser parser;
    std::vector<char> htmlVec(html.begin(), html.end());
    Parser::ParseResult result = parser.Parse(htmlVec);

    std::cout << "=== PARSE RESULT ===" << std::endl;
    std::cout << "Total blocks: " << result.blocks.size() << std::endl;
    std::cout << std::endl;

    for (size_t i = 0; i < result.blocks.size(); i++) {
        const auto& block = result.blocks[i];
        std::cout << "Block #" << i << ": ";
        
        // Print type
        switch(block.type) {
            case Parser::BLOCK_H1: std::cout << "H1"; break;
            case Parser::BLOCK_P: std::cout << "P"; break;
            case Parser::BLOCK_LI: std::cout << "LI"; break;
            case Parser::BLOCK_A: std::cout << "A"; break;
            case Parser::BLOCK_TEXT: std::cout << "TEXT"; break;
            case Parser::BLOCK_UL: std::cout << "UL"; break;
            default: std::cout << "OTHER(" << block.type << ")"; break;
        }
        
        // Print content
        std::cout << " | Content: \"" << block.content << "\"";
        
        // Print href if exists
        auto it = block.attributes.find("href");
        if (it != block.attributes.end()) {
            std::cout << " | href: \"" << it->second << "\"";
        }
        
        std::cout << std::endl;
    }

    std::cout << std::endl << "=== WARNINGS ===" << std::endl;
    for (const auto& warning : result.warnings) {
        std::cout << "  " << warning << std::endl;
    }

    // Verify expectations
    bool hasLIwithContent = false;
    bool hasLIwithHref = false;
    
    for (const auto& block : result.blocks) {
        if (block.type == Parser::BLOCK_LI) {
            if (!block.content.empty()) hasLIwithContent = true;
            if (block.attributes.find("href") != block.attributes.end()) hasLIwithHref = true;
        }
    }

    std::cout << std::endl << "=== VALIDATION ===" << std::endl;
    std::cout << "LI blocks have content: " << (hasLIwithContent ? "YES ✓" : "NO ✗") << std::endl;
    std::cout << "LI blocks have href: " << (hasLIwithHref ? "YES ✓" : "NO ✗") << std::endl;

    return (hasLIwithContent && hasLIwithHref) ? 0 : 1;
}
