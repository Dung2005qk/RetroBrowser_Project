#!/usr/bin/env python3
"""
Win98 API Compatibility Checker
Scans C++ source files for Windows API usage and checks compatibility with Windows 98 SE
"""

import os
import re
import sys
from pathlib import Path
from dataclasses import dataclass
from typing import List, Dict, Set
from win98_api_database import (
    WIN98_COMPATIBLE_APIS,
    INCOMPATIBLE_APIS,
    SUSPICIOUS_PATTERNS,
    API_METADATA
)


@dataclass
class APIIssue:
    """Represents a compatibility issue found in source code"""
    file: str
    line: int
    api_name: str
    severity: str  # "ERROR", "WARNING", "INFO"
    message: str
    suggestion: str


class Win98APIChecker:
    """Main checker class for Win98 API compatibility"""
    
    def __init__(self):
        # Build flat set of all compatible APIs
        self.compatible_apis = set()
        for dll_apis in WIN98_COMPATIBLE_APIS.values():
            self.compatible_apis.update(dll_apis)
        
        # Build flat set of all incompatible APIs with their OS version
        self.incompatible_apis = {}
        for os_version, data in INCOMPATIBLE_APIS.items():
            for api in data["apis"]:
                self.incompatible_apis[api] = {
                    "os_version": os_version,
                    "alternative": data["alternatives"].get(api, "No known alternative")
                }
        
        # Compile suspicious patterns
        self.suspicious_patterns = [re.compile(pattern) for pattern in SUSPICIOUS_PATTERNS]
        
        # Pattern to match function calls (simplified)
        # Matches: FunctionName( or ::FunctionName(
        self.function_call_pattern = re.compile(r'\b([A-Z][A-Za-z0-9_]*)\s*\(')
        
        self.issues = []
        self.api_usage = {}  # Track all APIs used
    
    def is_in_comment_or_string(self, line: str, pos: int) -> bool:
        """Check if position is inside a comment or string literal"""
        # Check for line comment
        comment_pos = line.find('//')
        if comment_pos != -1 and comment_pos < pos:
            return True
        
        # Check for string literals (simplified - doesn't handle escaped quotes)
        in_string = False
        for i, char in enumerate(line[:pos]):
            if char == '"' and (i == 0 or line[i-1] != '\\'):
                in_string = not in_string
        
        return in_string
    
    def scan_file(self, filepath: str) -> List[APIIssue]:
        """Scan a single file for API compatibility issues"""
        file_issues = []
        
        try:
            with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
        except Exception as e:
            print(f"Warning: Could not read {filepath}: {e}")
            return file_issues
        
        in_block_comment = False
        
        for line_num, line in enumerate(lines, 1):
            # Handle block comments
            if '/*' in line:
                in_block_comment = True
            if '*/' in line:
                in_block_comment = False
                continue
            if in_block_comment:
                continue
            
            # Skip preprocessor directives (except #include which we check separately)
            if line.strip().startswith('#') and not line.strip().startswith('#include'):
                continue
            
            # Check for suspicious patterns
            for pattern in self.suspicious_patterns:
                match = pattern.search(line)
                if match and not self.is_in_comment_or_string(line, match.start()):
                    issue = APIIssue(
                        file=filepath,
                        line=line_num,
                        api_name=match.group(0),
                        severity="WARNING",
                        message=f"Suspicious pattern detected: {match.group(0)}",
                        suggestion="Verify this is compatible with Win98"
                    )
                    file_issues.append(issue)
            
            # Extract function calls
            for match in self.function_call_pattern.finditer(line):
                if self.is_in_comment_or_string(line, match.start()):
                    continue
                
                api_name = match.group(1)
                
                # Track API usage
                if api_name not in self.api_usage:
                    self.api_usage[api_name] = []
                self.api_usage[api_name].append((filepath, line_num))
                
                # Check if it's an incompatible API
                if api_name in self.incompatible_apis:
                    info = self.incompatible_apis[api_name]
                    issue = APIIssue(
                        file=filepath,
                        line=line_num,
                        api_name=api_name,
                        severity="ERROR",
                        message=f"Incompatible API: {api_name} requires {info['os_version']}",
                        suggestion=info['alternative']
                    )
                    file_issues.append(issue)
                
                # Check if it's a known compatible API
                elif api_name in self.compatible_apis:
                    # Known good - no issue
                    pass
                
                # Unknown API - might need manual verification
                elif api_name[0].isupper() and len(api_name) > 3:
                    # Only flag if it looks like a Win32 API (starts with capital, reasonable length)
                    # and contains typical Win32 patterns
                    if any(keyword in api_name for keyword in ['Create', 'Get', 'Set', 'Open', 'Close', 'Read', 'Write', 'Send', 'Recv']):
                        issue = APIIssue(
                            file=filepath,
                            line=line_num,
                            api_name=api_name,
                            severity="INFO",
                            message=f"Unknown API: {api_name} not in Win98 whitelist",
                            suggestion="Verify in MSDN Win98 documentation"
                        )
                        file_issues.append(issue)
        
        return file_issues
    
    def scan_directory(self, dirpath: str, extensions: List[str] = ['.cpp', '.h']) -> Dict[str, List[APIIssue]]:
        """Scan all files in directory with given extensions"""
        all_issues = {}
        
        for root, dirs, files in os.walk(dirpath):
            for file in files:
                if any(file.endswith(ext) for ext in extensions):
                    filepath = os.path.join(root, file)
                    issues = self.scan_file(filepath)
                    if issues:
                        all_issues[filepath] = issues
                    self.issues.extend(issues)
        
        return all_issues
    
    def get_statistics(self) -> Dict:
        """Calculate statistics about API usage and issues"""
        total_apis = len(self.api_usage)
        compatible_count = sum(1 for api in self.api_usage if api in self.compatible_apis)
        
        error_count = sum(1 for issue in self.issues if issue.severity == "ERROR")
        warning_count = sum(1 for issue in self.issues if issue.severity == "WARNING")
        info_count = sum(1 for issue in self.issues if issue.severity == "INFO")
        
        return {
            "total_apis": total_apis,
            "compatible_apis": compatible_count,
            "compatible_percentage": (compatible_count / total_apis * 100) if total_apis > 0 else 0,
            "total_issues": len(self.issues),
            "errors": error_count,
            "warnings": warning_count,
            "info": info_count,
        }

    def generate_text_report(self, output_file: str = None) -> str:
        """Generate plain text report for CI/CD"""
        stats = self.get_statistics()
        
        lines = []
        lines.append("=" * 80)
        lines.append("Win98 API Compatibility Report")
        lines.append("=" * 80)
        lines.append("")
        
        # Statistics
        lines.append("STATISTICS:")
        lines.append(f"  Total APIs detected: {stats['total_apis']}")
        lines.append(f"  Compatible APIs: {stats['compatible_apis']} ({stats['compatible_percentage']:.1f}%)")
        lines.append(f"  Total issues: {stats['total_issues']}")
        lines.append(f"    - Errors: {stats['errors']}")
        lines.append(f"    - Warnings: {stats['warnings']}")
        lines.append(f"    - Info: {stats['info']}")
        lines.append("")
        
        # Group issues by severity
        errors = [i for i in self.issues if i.severity == "ERROR"]
        warnings = [i for i in self.issues if i.severity == "WARNING"]
        infos = [i for i in self.issues if i.severity == "INFO"]
        
        if errors:
            lines.append("ERRORS (Incompatible APIs):")
            lines.append("-" * 80)
            for issue in errors:
                lines.append(f"  {issue.file}:{issue.line}")
                lines.append(f"    API: {issue.api_name}")
                lines.append(f"    Issue: {issue.message}")
                lines.append(f"    Suggestion: {issue.suggestion}")
                lines.append("")
        
        if warnings:
            lines.append("WARNINGS (Suspicious Patterns):")
            lines.append("-" * 80)
            for issue in warnings:
                lines.append(f"  {issue.file}:{issue.line}")
                lines.append(f"    Pattern: {issue.api_name}")
                lines.append(f"    Issue: {issue.message}")
                lines.append("")
        
        if infos:
            lines.append("INFO (Unknown APIs - Manual Verification Needed):")
            lines.append("-" * 80)
            for issue in infos:
                lines.append(f"  {issue.file}:{issue.line}")
                lines.append(f"    API: {issue.api_name}")
                lines.append(f"    Suggestion: {issue.suggestion}")
                lines.append("")
        
        report = "\n".join(lines)
        
        if output_file:
            with open(output_file, 'w', encoding='utf-8') as f:
                f.write(report)
        
        return report

    def generate_html_report(self, output_file: str) -> str:
        """Generate HTML report with color-coded severity levels"""
        stats = self.get_statistics()
        
        html = f"""<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>Win98 API Compatibility Report</title>
    <style>
        body {{
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            margin: 20px;
            background-color: #f5f5f5;
        }}
        .container {{
            max-width: 1200px;
            margin: 0 auto;
            background-color: white;
            padding: 30px;
            box-shadow: 0 0 10px rgba(0,0,0,0.1);
        }}
        h1 {{
            color: #333;
            border-bottom: 3px solid #0066cc;
            padding-bottom: 10px;
        }}
        h2 {{
            color: #555;
            margin-top: 30px;
        }}
        .stats {{
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 20px;
            margin: 20px 0;
        }}
        .stat-box {{
            padding: 20px;
            border-radius: 5px;
            text-align: center;
        }}
        .stat-box.good {{
            background-color: #d4edda;
            border: 1px solid #c3e6cb;
        }}
        .stat-box.warning {{
            background-color: #fff3cd;
            border: 1px solid #ffeaa7;
        }}
        .stat-box.error {{
            background-color: #f8d7da;
            border: 1px solid #f5c6cb;
        }}
        .stat-number {{
            font-size: 36px;
            font-weight: bold;
            margin: 10px 0;
        }}
        .stat-label {{
            font-size: 14px;
            color: #666;
        }}
        .issue {{
            margin: 15px 0;
            padding: 15px;
            border-left: 4px solid;
            background-color: #f9f9f9;
        }}
        .issue.error {{
            border-left-color: #dc3545;
            background-color: #fff5f5;
        }}
        .issue.warning {{
            border-left-color: #ffc107;
            background-color: #fffef5;
        }}
        .issue.info {{
            border-left-color: #17a2b8;
            background-color: #f5fcff;
        }}
        .issue-header {{
            font-weight: bold;
            margin-bottom: 5px;
        }}
        .issue-location {{
            color: #666;
            font-size: 14px;
            font-family: monospace;
        }}
        .issue-message {{
            margin: 10px 0;
        }}
        .issue-suggestion {{
            background-color: #e7f3ff;
            padding: 10px;
            border-radius: 3px;
            margin-top: 10px;
            font-size: 14px;
        }}
        .severity-badge {{
            display: inline-block;
            padding: 3px 8px;
            border-radius: 3px;
            font-size: 12px;
            font-weight: bold;
            color: white;
        }}
        .severity-badge.error {{
            background-color: #dc3545;
        }}
        .severity-badge.warning {{
            background-color: #ffc107;
            color: #333;
        }}
        .severity-badge.info {{
            background-color: #17a2b8;
        }}
    </style>
</head>
<body>
    <div class="container">
        <h1>Win98 API Compatibility Report</h1>
        
        <div class="stats">
            <div class="stat-box good">
                <div class="stat-label">Total APIs Detected</div>
                <div class="stat-number">{stats['total_apis']}</div>
            </div>
            <div class="stat-box good">
                <div class="stat-label">Compatible APIs</div>
                <div class="stat-number">{stats['compatible_apis']}</div>
                <div class="stat-label">{stats['compatible_percentage']:.1f}%</div>
            </div>
            <div class="stat-box {'error' if stats['errors'] > 0 else 'good'}">
                <div class="stat-label">Errors</div>
                <div class="stat-number">{stats['errors']}</div>
            </div>
            <div class="stat-box {'warning' if stats['warnings'] > 0 else 'good'}">
                <div class="stat-label">Warnings</div>
                <div class="stat-number">{stats['warnings']}</div>
            </div>
            <div class="stat-box">
                <div class="stat-label">Info</div>
                <div class="stat-number">{stats['info']}</div>
            </div>
        </div>
"""
        
        # Group issues by severity
        errors = [i for i in self.issues if i.severity == "ERROR"]
        warnings = [i for i in self.issues if i.severity == "WARNING"]
        infos = [i for i in self.issues if i.severity == "INFO"]
        
        if errors:
            html += """
        <h2>❌ Errors (Incompatible APIs)</h2>
        <p>These APIs are not available on Windows 98 and must be replaced.</p>
"""
            for issue in errors:
                html += f"""
        <div class="issue error">
            <div class="issue-header">
                <span class="severity-badge error">ERROR</span>
                {issue.api_name}
            </div>
            <div class="issue-location">{issue.file}:{issue.line}</div>
            <div class="issue-message">{issue.message}</div>
            <div class="issue-suggestion"><strong>Suggestion:</strong> {issue.suggestion}</div>
        </div>
"""
        
        if warnings:
            html += """
        <h2>⚠️ Warnings (Suspicious Patterns)</h2>
        <p>These patterns may indicate compatibility issues. Manual verification recommended.</p>
"""
            for issue in warnings:
                html += f"""
        <div class="issue warning">
            <div class="issue-header">
                <span class="severity-badge warning">WARNING</span>
                {issue.api_name}
            </div>
            <div class="issue-location">{issue.file}:{issue.line}</div>
            <div class="issue-message">{issue.message}</div>
            <div class="issue-suggestion"><strong>Suggestion:</strong> {issue.suggestion}</div>
        </div>
"""
        
        if infos:
            html += """
        <h2>ℹ️ Info (Unknown APIs)</h2>
        <p>These APIs are not in the Win98 whitelist. Verify compatibility in MSDN documentation.</p>
"""
            for issue in infos:
                html += f"""
        <div class="issue info">
            <div class="issue-header">
                <span class="severity-badge info">INFO</span>
                {issue.api_name}
            </div>
            <div class="issue-location">{issue.file}:{issue.line}</div>
            <div class="issue-suggestion"><strong>Suggestion:</strong> {issue.suggestion}</div>
        </div>
"""
        
        html += """
    </div>
</body>
</html>
"""
        
        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(html)
        
        return html


def main():
    """Main entry point"""
    if len(sys.argv) < 2:
        print("Usage: python check_win98_compatibility.py <source_directory>")
        print("Example: python check_win98_compatibility.py ../src/browser")
        sys.exit(1)
    
    source_dir = sys.argv[1]
    
    if not os.path.exists(source_dir):
        print(f"Error: Directory '{source_dir}' does not exist")
        sys.exit(1)
    
    print(f"Scanning {source_dir} for Win98 API compatibility...")
    print()
    
    checker = Win98APIChecker()
    issues = checker.scan_directory(source_dir)
    
    # Generate reports
    text_report = checker.generate_text_report("docs/win98_compatibility_report.txt")
    print(text_report)
    
    html_report_path = "docs/win98_compatibility_report.html"
    checker.generate_html_report(html_report_path)
    print(f"\nHTML report generated: {html_report_path}")
    
    # Exit with error code if there are errors
    stats = checker.get_statistics()
    if stats['errors'] > 0:
        print(f"\n❌ Found {stats['errors']} compatibility errors!")
        sys.exit(1)
    elif stats['warnings'] > 0:
        print(f"\n⚠️ Found {stats['warnings']} warnings. Manual review recommended.")
        sys.exit(0)
    else:
        print("\n✅ No compatibility issues found!")
        sys.exit(0)


if __name__ == "__main__":
    main()
