@echo off
REM ============================================================================
REM fix_vc6_compat.bat - Fix VC++6.0 Compatibility Issues
REM ============================================================================
REM PURPOSE: Replace C++11/modern C++ features with VC++6.0 compatible code
REM ISSUES FIXED:
REM   1. std::string.clear() -> assignment to empty string (VC++6.0 has no clear())
REM   2. DEBUG_LOGF -> DEBUG_LOG (removed variadic macro)
REM ============================================================================

echo Fixing VC++6.0 compatibility issues...
echo.

REM Note: Windows doesn't have sed by default, so we'll document the changes needed
echo MANUAL FIXES REQUIRED:
echo.
echo 1. In parser.cpp - Replace all .clear() calls:
echo    - textBuffer.clear()        -^> textBuffer = ""
echo    - tagName.clear()           -^> tagName = ""
echo    - attrKey.clear()           -^> attrKey = ""
echo    - attrValue.clear()         -^> attrValue = ""
echo    - attributes.clear()        -^> attributes = std::map^<std::string, std::string^>()
echo    - containerContent.clear()  -^> containerContent = ""
echo    - containerAttrs.clear()    -^> containerAttrs = std::map^<std::string, std::string^>()
echo    - skipTagName.clear()       -^> skipTagName = ""
echo.
echo 2. In network.cpp - Replace all DEBUG_LOGF with DEBUG_LOG:
echo    - Remove all DEBUG_LOGF calls or convert to simple DEBUG_LOG
echo.
echo 3. Alternative: Use erase() instead of clear():
echo    - str.clear() -^> str.erase(str.begin(), str.end())
echo.

pause
