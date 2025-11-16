#!/usr/bin/env python3
"""
Win98 Binary Validation Script

Validates that a compiled binary is compatible with Windows 98 by checking:
- PE header subsystem version (must be 4.10)
- Machine type (must be x86)
- DLL dependencies (must be Win98-compatible)
- Binary size (should be reasonable for Win98)
- No .NET metadata

Usage:
    python validate_win98_binary.py <path_to_exe>
    python validate_win98_binary.py deploy/RetroBrowser_Win98.exe
"""

import sys
import os
import struct
import subprocess
from pathlib import Path
from typing import List, Tuple, Optional
from dataclasses import dataclass


@dataclass
class ValidationResult:
    """Result of a validation check"""
    check_name: str
    passed: bool
    message: str
    severity: str = "ERROR"  # ERROR, WARNING, INFO


class Win98BinaryValidator:
    """Validator for Win98 binary compatibility"""
    
    # Win98-compatible DLLs
    WIN98_COMPATIBLE_DLLS = {
        'kernel32.dll',
        'user32.dll',
        'gdi32.dll',
        'advapi32.dll',
        'ws2_32.dll',
        'wsock32.dll',
        'comctl32.dll',
        'comdlg32.dll',
        'shell32.dll',
        'ole32.dll',
        'oleaut32.dll',
        'winmm.dll',
        'version.dll',
        'msvcrt.dll',
    }
    
    # Maximum reasonable size for Win98 (2MB)
    MAX_BINARY_SIZE = 2 * 1024 * 1024
    
    def __init__(self, binary_path: str):
        self.binary_path = Path(binary_path)
        self.results: List[ValidationResult] = []
        
    def validate_all(self) -> bool:
        """Run all validation checks"""
        print(f"Validating binary: {self.binary_path}")
        print("=" * 70)
        
        if not self.binary_path.exists():
            print(f"ERROR: Binary not found: {self.binary_path}")
            return False
        
        # Run all checks
        self.check_pe_header()
        self.check_dependencies()
        self.check_size_and_resources()
        
        # Print results
        self._print_results()
        
        # Return overall status
        return self._get_overall_status()
    
    def check_pe_header(self) -> None:
        """Check PE header for Win98 compatibility"""
        try:
            with open(self.binary_path, 'rb') as f:
                # Read DOS header
                dos_header = f.read(64)
                if dos_header[:2] != b'MZ':
                    self.results.append(ValidationResult(
                        "PE Header",
                        False,
                        "Not a valid PE executable (missing MZ signature)",
                        "ERROR"
                    ))
                    return
                
                # Get PE header offset
                pe_offset = struct.unpack('<I', dos_header[60:64])[0]
                f.seek(pe_offset)
                
                # Read PE signature
                pe_sig = f.read(4)
                if pe_sig != b'PE\x00\x00':
                    self.results.append(ValidationResult(
                        "PE Header",
                        False,
                        "Not a valid PE executable (missing PE signature)",
                        "ERROR"
                    ))
                    return
                
                # Read COFF header
                coff_header = f.read(20)
                machine = struct.unpack('<H', coff_header[0:2])[0]
                
                # Check machine type (0x014c = IMAGE_FILE_MACHINE_I386)
                if machine == 0x014c:
                    self.results.append(ValidationResult(
                        "Machine Type",
                        True,
                        f"Machine type is x86 (0x{machine:04X}) ✓",
                        "INFO"
                    ))
                else:
                    self.results.append(ValidationResult(
                        "Machine Type",
                        False,
                        f"Machine type is not x86 (0x{machine:04X})",
                        "ERROR"
                    ))
                
                # Read optional header
                optional_header_size = struct.unpack('<H', coff_header[16:18])[0]
                optional_header = f.read(optional_header_size)
                
                if len(optional_header) < 68:
                    self.results.append(ValidationResult(
                        "PE Header",
                        False,
                        "Optional header too small",
                        "ERROR"
                    ))
                    return
                
                # Check for .NET metadata (CLR header)
                magic = struct.unpack('<H', optional_header[0:2])[0]
                if magic == 0x20B:  # PE32+
                    self.results.append(ValidationResult(
                        ".NET Check",
                        False,
                        "Binary is PE32+ (64-bit), not compatible with Win98",
                        "ERROR"
                    ))
                    return
                elif magic == 0x10B:  # PE32
                    # Check for CLR runtime header
                    if optional_header_size >= 208:
                        clr_rva = struct.unpack('<I', optional_header[208:212])[0] if len(optional_header) >= 212 else 0
                        if clr_rva != 0:
                            self.results.append(ValidationResult(
                                ".NET Check",
                                False,
                                "Binary contains .NET metadata (CLR header present)",
                                "ERROR"
                            ))
                        else:
                            self.results.append(ValidationResult(
                                ".NET Check",
                                True,
                                "No .NET metadata detected ✓",
                                "INFO"
                            ))
                    else:
                        self.results.append(ValidationResult(
                            ".NET Check",
                            True,
                            "No .NET metadata detected ✓",
                            "INFO"
                        ))
                
                # Get subsystem version
                subsystem = struct.unpack('<H', optional_header[68:70])[0]
                major_subsystem_version = struct.unpack('<H', optional_header[48:50])[0]
                minor_subsystem_version = struct.unpack('<H', optional_header[50:52])[0]
                
                subsystem_names = {
                    1: "NATIVE",
                    2: "WINDOWS_GUI",
                    3: "WINDOWS_CUI",
                    5: "OS2_CUI",
                    7: "POSIX_CUI"
                }
                subsystem_name = subsystem_names.get(subsystem, f"UNKNOWN({subsystem})")
                
                # Check subsystem version (4.10 = Win98)
                if major_subsystem_version == 4 and minor_subsystem_version == 10:
                    self.results.append(ValidationResult(
                        "Subsystem Version",
                        True,
                        f"Subsystem version is {major_subsystem_version}.{minor_subsystem_version:02d} (Win98) ✓",
                        "INFO"
                    ))
                elif major_subsystem_version == 4 and minor_subsystem_version == 0:
                    self.results.append(ValidationResult(
                        "Subsystem Version",
                        True,
                        f"Subsystem version is {major_subsystem_version}.{minor_subsystem_version:02d} (NT 4.0, compatible with Win98) ✓",
                        "INFO"
                    ))
                elif major_subsystem_version < 4:
                    self.results.append(ValidationResult(
                        "Subsystem Version",
                        True,
                        f"Subsystem version is {major_subsystem_version}.{minor_subsystem_version:02d} (older than Win98, should work) ✓",
                        "INFO"
                    ))
                else:
                    self.results.append(ValidationResult(
                        "Subsystem Version",
                        False,
                        f"Subsystem version is {major_subsystem_version}.{minor_subsystem_version:02d} (requires Windows {major_subsystem_version}.{minor_subsystem_version}+)",
                        "ERROR"
                    ))
                
                # Check subsystem type
                if subsystem == 2:  # WINDOWS_GUI
                    self.results.append(ValidationResult(
                        "Subsystem Type",
                        True,
                        f"Subsystem type is {subsystem_name} ✓",
                        "INFO"
                    ))
                else:
                    self.results.append(ValidationResult(
                        "Subsystem Type",
                        False,
                        f"Subsystem type is {subsystem_name} (expected WINDOWS_GUI)",
                        "WARNING"
                    ))
                    
        except Exception as e:
            self.results.append(ValidationResult(
                "PE Header",
                False,
                f"Error reading PE header: {str(e)}",
                "ERROR"
            ))
    
    def check_dependencies(self) -> None:
        """Check DLL dependencies using dumpbin"""
        try:
            # Try to use dumpbin (Visual Studio tool)
            result = subprocess.run(
                ['dumpbin', '/dependents', str(self.binary_path)],
                capture_output=True,
                text=True,
                timeout=10
            )
            
            if result.returncode == 0:
                self._parse_dumpbin_output(result.stdout)
            else:
                # Fallback to manual PE parsing
                self._parse_dependencies_manual()
                
        except FileNotFoundError:
            # dumpbin not available, use manual parsing
            self._parse_dependencies_manual()
        except Exception as e:
            self.results.append(ValidationResult(
                "Dependencies",
                False,
                f"Error checking dependencies: {str(e)}",
                "ERROR"
            ))
    
    def _parse_dumpbin_output(self, output: str) -> None:
        """Parse dumpbin output to extract dependencies"""
        dependencies = []
        in_dependencies_section = False
        
        for line in output.split('\n'):
            line = line.strip()
            if 'Image has the following dependencies' in line:
                in_dependencies_section = True
                continue
            if in_dependencies_section:
                if line.endswith('.dll') or line.endswith('.DLL'):
                    dependencies.append(line.lower())
                elif line == '' or 'Summary' in line:
                    break
        
        self._check_dependency_list(dependencies)
    
    def _parse_dependencies_manual(self) -> None:
        """Manually parse PE import table for dependencies"""
        try:
            with open(self.binary_path, 'rb') as f:
                # Read DOS header
                dos_header = f.read(64)
                pe_offset = struct.unpack('<I', dos_header[60:64])[0]
                
                # Read PE headers
                f.seek(pe_offset)
                f.read(4)  # PE signature
                coff_header = f.read(20)
                optional_header_size = struct.unpack('<H', coff_header[16:18])[0]
                optional_header = f.read(optional_header_size)
                
                # Get import table RVA
                if len(optional_header) < 120:
                    self.results.append(ValidationResult(
                        "Dependencies",
                        False,
                        "Cannot read import table",
                        "WARNING"
                    ))
                    return
                
                import_table_rva = struct.unpack('<I', optional_header[104:108])[0]
                
                if import_table_rva == 0:
                    self.results.append(ValidationResult(
                        "Dependencies",
                        True,
                        "No import table found (statically linked)",
                        "INFO"
                    ))
                    return
                
                # This is complex, so we'll just note that manual parsing is limited
                self.results.append(ValidationResult(
                    "Dependencies",
                    True,
                    "Manual dependency parsing not fully implemented (dumpbin not available)",
                    "WARNING"
                ))
                
        except Exception as e:
            self.results.append(ValidationResult(
                "Dependencies",
                False,
                f"Error parsing dependencies manually: {str(e)}",
                "WARNING"
            ))
    
    def _check_dependency_list(self, dependencies: List[str]) -> None:
        """Check if dependencies are Win98-compatible"""
        if not dependencies:
            self.results.append(ValidationResult(
                "Dependencies",
                True,
                "No DLL dependencies found (statically linked) ✓",
                "INFO"
            ))
            return
        
        incompatible = []
        compatible = []
        
        for dll in dependencies:
            dll_lower = dll.lower()
            if dll_lower in self.WIN98_COMPATIBLE_DLLS:
                compatible.append(dll)
            else:
                # Check for known incompatible patterns
                if any(pattern in dll_lower for pattern in ['msvcr1', 'msvcp1', 'vcruntime', 'ucrtbase']):
                    incompatible.append(dll)
                else:
                    # Unknown DLL, flag as warning
                    self.results.append(ValidationResult(
                        "Dependencies",
                        True,
                        f"Unknown DLL dependency: {dll} (verify Win98 compatibility)",
                        "WARNING"
                    ))
        
        if compatible:
            self.results.append(ValidationResult(
                "Dependencies",
                True,
                f"Win98-compatible DLLs: {', '.join(compatible)} ✓",
                "INFO"
            ))
        
        if incompatible:
            self.results.append(ValidationResult(
                "Dependencies",
                False,
                f"Incompatible DLLs detected: {', '.join(incompatible)}",
                "ERROR"
            ))
        
        if not incompatible and compatible:
            self.results.append(ValidationResult(
                "Dependencies",
                True,
                "All dependencies are Win98-compatible ✓",
                "INFO"
            ))
    
    def check_size_and_resources(self) -> None:
        """Check binary size and resources"""
        try:
            # Check file size
            file_size = self.binary_path.stat().st_size
            size_mb = file_size / (1024 * 1024)
            
            if file_size <= self.MAX_BINARY_SIZE:
                self.results.append(ValidationResult(
                    "Binary Size",
                    True,
                    f"Binary size is {size_mb:.2f} MB (within {self.MAX_BINARY_SIZE / (1024*1024):.0f} MB limit) ✓",
                    "INFO"
                ))
            else:
                self.results.append(ValidationResult(
                    "Binary Size",
                    False,
                    f"Binary size is {size_mb:.2f} MB (exceeds {self.MAX_BINARY_SIZE / (1024*1024):.0f} MB recommended limit)",
                    "WARNING"
                ))
            
            # Check for PDB file
            pdb_path = self.binary_path.with_suffix('.pdb')
            if pdb_path.exists():
                pdb_size = pdb_path.stat().st_size / (1024 * 1024)
                self.results.append(ValidationResult(
                    "Debug Symbols",
                    True,
                    f"Debug symbols found at {pdb_path.name} ({pdb_size:.2f} MB) ✓",
                    "INFO"
                ))
            else:
                self.results.append(ValidationResult(
                    "Debug Symbols",
                    True,
                    "No separate PDB file (symbols may be embedded or stripped)",
                    "INFO"
                ))
                
        except Exception as e:
            self.results.append(ValidationResult(
                "Size Check",
                False,
                f"Error checking size: {str(e)}",
                "ERROR"
            ))
    
    def _print_results(self) -> None:
        """Print validation results"""
        print("\nValidation Results:")
        print("-" * 70)
        
        errors = []
        warnings = []
        info = []
        
        for result in self.results:
            if result.severity == "ERROR":
                errors.append(result)
            elif result.severity == "WARNING":
                warnings.append(result)
            else:
                info.append(result)
        
        # Print errors first
        if errors:
            print("\n❌ ERRORS:")
            for result in errors:
                status = "✓" if result.passed else "✗"
                print(f"  {status} {result.check_name}: {result.message}")
        
        # Print warnings
        if warnings:
            print("\n⚠️  WARNINGS:")
            for result in warnings:
                status = "✓" if result.passed else "✗"
                print(f"  {status} {result.check_name}: {result.message}")
        
        # Print info
        if info:
            print("\n✓ INFO:")
            for result in info:
                print(f"  • {result.check_name}: {result.message}")
        
        print("\n" + "=" * 70)
    
    def _get_overall_status(self) -> bool:
        """Get overall validation status"""
        has_errors = any(not r.passed and r.severity == "ERROR" for r in self.results)
        has_warnings = any(not r.passed and r.severity == "WARNING" for r in self.results)
        
        if has_errors:
            print("❌ VALIDATION FAILED: Binary may not run on Windows 98")
            return False
        elif has_warnings:
            print("⚠️  VALIDATION PASSED WITH WARNINGS: Binary may have issues on Windows 98")
            return True
        else:
            print("✅ VALIDATION PASSED: Binary should be compatible with Windows 98")
            return True


def main():
    """Main entry point"""
    if len(sys.argv) < 2:
        print("Usage: python validate_win98_binary.py <path_to_exe>")
        print("Example: python validate_win98_binary.py deploy/RetroBrowser_Win98.exe")
        sys.exit(1)
    
    binary_path = sys.argv[1]
    validator = Win98BinaryValidator(binary_path)
    success = validator.validate_all()
    
    sys.exit(0 if success else 1)


if __name__ == '__main__':
    main()
