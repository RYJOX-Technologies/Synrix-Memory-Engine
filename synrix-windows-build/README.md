# SYNRIX Windows Build Package

Complete package for building and using SYNRIX on Windows.

## Contents

- `build/windows/` - Engine source files, CMakeLists.txt, build scripts
- `python-sdk/` - Python SDK with Windows DLL support
- `lattice/cursor_ai_memory.lattice` - **Your actual memory/knowledge graph** (use this!)
- `synrix_context_export.json` - Exported context from Jetson (backup/fallback)
- `python-sdk/import_context.py` - Script to import context into Windows SYNRIX

## Quick Start

1. **Build the engine:**
   ```powershell
   cd build\windows
   .\build.ps1
   ```

2. **Copy DLL to Python SDK:**
   ```powershell
   # After build, DLL will be in build\windows\build\Release\libsynrix.dll
   copy build\windows\build\Release\libsynrix.dll python-sdk\
   ```

3. **Test Python SDK:**
   ```powershell
   cd python-sdk
   python -c "from synrix.raw_backend import RawSynrixBackend; print('OK')"
   ```

4. **Use your lattice file (your memories):**
   ```powershell
   # Copy lattice file to your home directory
   copy lattice\cursor_ai_memory.lattice $env:USERPROFILE\.cursor_ai_memory.lattice
   
   # Test it works
   cd python-sdk
   python -c "from synrix.raw_backend import RawSynrixBackend; m = RawSynrixBackend('~/.cursor_ai_memory.lattice'); print('✅ Memories loaded!')"
   ```

5. **Import context (optional, if lattice file missing):**
   ```powershell
   cd python-sdk
   python import_context.py
   ```

## Structure

```
synrix-windows-build/
├── build/
│   └── windows/          # Engine build files
│       ├── src/          # C source files
│       ├── include/      # Header files
│       ├── CMakeLists.txt
│       ├── build.ps1
│       └── *.md          # Documentation
├── python-sdk/           # Python SDK
│   ├── synrix/           # Python package
│   ├── examples/         # Example scripts
│   └── import_context.py
├── lattice/              # Your memory/knowledge graph
│   └── cursor_ai_memory.lattice
└── README.md
```

## Requirements

- CMake 3.15+
- Visual Studio 2019/2022 OR MinGW-w64
- Python 3.7+

See `build/windows/BUILD.md` for detailed instructions.
