#!/usr/bin/env python3
"""
Store comprehensive knowledge about SYNRIX installer system in the lattice.
This ensures we remember all the fixes, dependencies, and best practices.
"""

import sys
from pathlib import Path

# Add synrix to path
sys.path.insert(0, str(Path(__file__).parent / "synrix_unlimited"))

from synrix.ai_memory import get_ai_memory

memory = get_ai_memory()

print("Storing SYNRIX installer knowledge in lattice...")
print()

# 1. Installer Architecture
memory.add("INSTALLER:architecture", """
SYNRIX uses a two-tier installer system:
- installer.bat: Main entry point (default, user-facing)
- installer_v2.py: Improved installer with auto-dependency handling

installer.bat checks for installer_v2.py and uses it if available,
falling back to basic installation if not found.
""")

# 2. Dependencies
memory.add("INSTALLER:dependencies", """
SYNRIX requires three types of dependencies on Windows:

1. Visual C++ 2013 Runtime (x64):
   - DLLs: msvcr120.dll, msvcp120.dll
   - Location: C:\\Windows\\System32\\
   - Installer: install_vc2013.bat or auto-installed by installer_v2.py
   - Download: https://www.microsoft.com/en-us/download/details.aspx?id=40784

2. zlib1.dll:
   - Location: synrix_unlimited/synrix/zlib1.dll (package-local)
   - Installer: download_zlib.ps1 or auto-downloaded by installer_v2.py
   - Source: MinGW-w64 releases (GitHub)

3. MinGW Runtime DLLs (included in package):
   - libgcc_s_seh-1.dll
   - libstdc++-6.dll
   - libwinpthread-1.dll
   - Location: synrix_unlimited/synrix/
""")

# 3. Installer Features
memory.add("INSTALLER:features", """
installer_v2.py provides:
- Automatic Python version and architecture checking (64-bit required)
- Automatic VC++ 2013 Runtime installation (downloads and installs silently)
- Automatic zlib1.dll download (extracts from MinGW package)
- Automatic SYNRIX package installation (pip install -e .)
- Automatic installation testing
- Better error messages with actionable fixes
- One-click experience (double-click installer_v2.bat)
""")

# 4. Common Issues and Fixes
memory.add("INSTALLER:issues_fixes", """
Common installation issues and their fixes:

1. "Could not find module 'libsynrix.dll'":
   - Cause: Missing VC++ 2013 Runtime
   - Fix: Run install_vc2013.bat or installer_v2.bat

2. "The specified module could not be found":
   - Cause: Missing zlib1.dll
   - Fix: Run download_zlib.ps1 or installer_v2.bat

3. "is not a valid editable requirement":
   - Cause: Trailing backslash in path passed to pip
   - Fix: installer_v2.py removes trailing backslash automatically

4. "function 'lattice_init' not found":
   - Cause: DLL loaded but functions not accessible (native loader issue)
   - Fix: raw_backend.py automatically falls back to ctypes.CDLL()

5. DLL loading fails in Cursor IDE:
   - Cause: Path resolution issues in non-standard Python environments
   - Fix: installer_v2.py uses multiple path resolution methods
""")

# 5. Error Message Improvements
memory.add("INSTALLER:error_messages", """
raw_backend.py now provides enhanced error messages:
- Detects missing VC++ 2013 Runtime via registry check
- Detects missing zlib1.dll via file existence check
- Provides one-click fixes in error messages
- Links to installer_v2.bat for automatic resolution
- Shows exact paths and troubleshooting steps
""")

# 6. Package Structure
memory.add("INSTALLER:package_structure", """
synrix_unlimited package structure:
- installer.bat (main entry point, uses installer_v2.py if available)
- installer_v2.bat (double-click wrapper for installer_v2.py)
- installer_v2.py (improved installer with auto-dependency handling)
- install_vc2013.bat (VC++ 2013 Runtime installer)
- download_zlib.ps1 (zlib1.dll downloader)
- setup.py (package setup)
- synrix/ (Python package directory)
  - libsynrix.dll (main DLL)
  - libgcc_s_seh-1.dll (MinGW runtime)
  - libstdc++-6.dll (MinGW runtime)
  - libwinpthread-1.dll (MinGW runtime)
  - zlib1.dll (if downloaded)
  - *.py (Python modules)
- Documentation files (START_HERE.md, REQUIREMENTS.md, etc.)
- Diagnostic tools (check_dll_dependencies.py, etc.)
""")

# 7. Best Practices
memory.add("INSTALLER:best_practices", """
Best practices for SYNRIX installer:

1. Always use installer_v2.py as the default installer
2. installer.bat should check for installer_v2.py and use it
3. Provide fallback to basic installation if installer_v2.py missing
4. Include all dependency installers in package (install_vc2013.bat, download_zlib.ps1)
5. Include comprehensive diagnostic tools
6. Error messages should detect missing dependencies and provide fixes
7. Test on clean Windows systems before distribution
8. Remove trailing backslashes from paths before passing to pip
9. Check Python architecture (64-bit required)
10. Verify all DLLs are present before attempting installation
""")

# 8. Testing Checklist
memory.add("INSTALLER:testing_checklist", """
Testing checklist for SYNRIX installer:

1. Package structure:
   - All required files present
   - installer_v2.py syntax valid
   - installer.bat calls installer_v2.py correctly

2. Dependency detection:
   - VC++ 2013 Runtime check works
   - zlib1.dll check works
   - Python version/architecture check works

3. Installation:
   - VC++ 2013 Runtime auto-installs (if needed)
   - zlib1.dll auto-downloads (if needed)
   - SYNRIX package installs correctly
   - Installation test passes

4. Error handling:
   - Error messages detect missing dependencies
   - Error messages provide one-click fixes
   - Fallback mechanisms work

5. Documentation:
   - START_HERE.md mentions installer_v2
   - REQUIREMENTS.md mentions installer_v2
   - IF_YOU_HAVE_ISSUES.md mentions installer_v2
""")

# 9. Platform-Specific Notes
memory.add("INSTALLER:platform_notes", """
Platform-specific installation notes:

Windows:
- Requires 64-bit Python (architecture check in installer_v2.py)
- VC++ 2013 Runtime must be installed system-wide
- zlib1.dll must be in synrix/ directory (package-local)
- MinGW runtime DLLs must be in synrix/ directory
- Use os.add_dll_directory() for secure DLL resolution (Windows 10+)

Linux/macOS:
- No VC++ Runtime needed
- zlib typically available via system package manager
- MinGW runtime not needed (different toolchain)
- Standard CDLL loading works
""")

# 10. Future Improvements
memory.add("INSTALLER:future_improvements", """
Future improvements for SYNRIX installer:

Priority 3: pip package
- pip install synrix should work
- Bundle dependencies in wheel
- Handle platform differences automatically
- Requires PyPI account and CI/CD setup

Other improvements:
- GUI installer option
- Silent installation mode
- Uninstaller script
- Update mechanism
- Version checking
""")

# 11. Build System
memory.add("BUILD:system", """
SYNRIX uses a simple build system: change node count, press build, get package.

Main script: build_package.py
- Builds DLL with specified node limit
- Creates package from template (synrix_unlimited/)
- Packages everything into ZIP ready to distribute

Usage:
  python build_package.py --limit 50000 --name free_tier_50k
  python build_package.py --limit 100000 --name free_tier_100k
  python build_package.py --unlimited --name unlimited

Or build all at once:
  build_all_versions.bat
""")

memory.add("BUILD:process", """
Build process:
1. Builds DLL with CMake (specified node limit or unlimited)
2. Copies template from synrix_unlimited/
3. Replaces DLL with newly built one
4. Updates metadata (package name, node limit info)
5. Creates ZIP file ready to distribute

Output: packages/{name}/ and packages/{name}.zip
""")

memory.add("BUILD:cmake_config", """
CMake configuration for different versions:

Free Tier (with limit):
  -DSYNRIX_FREE_TIER_50K=ON
  -DSYNRIX_FREE_TIER_LIMIT={limit}
  -DSYNRIX_EVALUATION_MODE_ALWAYS_ENABLED=ON
  Output: libsynrix_free_tier.dll

Unlimited:
  (no free tier flags)
  Output: libsynrix.dll
""")

memory.add("BUILD:template", """
Package template: synrix_unlimited/
- Contains all installer files (installer_v2.py, etc.)
- Contains all documentation
- Contains all diagnostic tools
- Contains MinGW runtime DLLs
- This is the base for all packages

Each package is created by:
1. Copying template
2. Replacing DLL with version-specific one
3. Updating metadata
""")

print()
print("[OK] All installer and build knowledge stored in lattice!")
print()
print("Stored topics:")
print("  - INSTALLER:architecture")
print("  - INSTALLER:dependencies")
print("  - INSTALLER:features")
print("  - INSTALLER:issues_fixes")
print("  - INSTALLER:error_messages")
print("  - INSTALLER:package_structure")
print("  - INSTALLER:best_practices")
print("  - INSTALLER:testing_checklist")
print("  - INSTALLER:platform_notes")
print("  - INSTALLER:future_improvements")
print("  - BUILD:system")
print("  - BUILD:process")
print("  - BUILD:cmake_config")
print("  - BUILD:template")
print()
print("Query with: memory.query('INSTALLER:') or memory.query('BUILD:')")
