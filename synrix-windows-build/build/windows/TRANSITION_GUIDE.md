# Transition Guide: Jetson → Host PC

## Complete Package - Everything You Need

The transfer package includes **EVERYTHING** needed to build and use SYNRIX on Windows:

### Engine (C Code)
- ✅ 34 source files (`.c`) in `build/windows/src/`
- ✅ 26 header files (`.h`) in `build/windows/include/`
- ✅ `CMakeLists.txt` - Build configuration
- ✅ `build.ps1` / `build.bat` - Build scripts
- ✅ Complete documentation (BUILD.md, WINDOWS_COMPATIBILITY.md, PLATFORM_ABSTRACTION.md)

### Python SDK
- ✅ Complete `synrix/` Python package with **Windows DLL support**
- ✅ `raw_backend.py` - Updated to detect `libsynrix.dll` on Windows
- ✅ Example scripts in `examples/`
- ✅ `setup.py` and `pyproject.toml` for installation

### Your Memories (Lattice File)
- ✅ `lattice/cursor_ai_memory.lattice` - **Your actual knowledge graph/memories**
- ✅ Cross-platform compatible (same binary format works on Windows)
- ✅ All your stored patterns, constraints, failures, and task context

### Context Transfer (Backup)
- ✅ `synrix_context_export.json` - Exported context from Jetson (backup/fallback)
- ✅ `python-sdk/import_context.py` - Script to import context into Windows SYNRIX

## Transfer Package

**Location on Jetson:** `/tmp/synrix-windows-build.tar.gz` (~760KB)

**Transfer Command:**
```powershell
scp astro@192.168.1.185:/tmp/synrix-windows-build.tar.gz $env:USERPROFILE\Downloads\
```

## Using Your Lattice File (Your Memories)

The lattice file is **cross-platform compatible** - the same binary format works on both Linux and Windows. Your memories are preserved!

### Option 1: Use Lattice File from Package (Recommended)

The package includes your lattice file. Just copy it to your home directory:

```powershell
# After extracting the package
copy lattice\cursor_ai_memory.lattice $env:USERPROFILE\.cursor_ai_memory.lattice

# Use it
python -c "from synrix.raw_backend import RawSynrixBackend; m = RawSynrixBackend('~/.cursor_ai_memory.lattice'); print('✅ All your memories are here!')"
```

### Option 2: Transfer Updated Lattice Later

If you update the lattice on Jetson and want to sync it to Windows:

**On Jetson:**
```bash
cd build/windows
bash transfer_lattice.sh Livew 192.168.1.XXX  # Update with your Windows IP
```

**On Windows:**
```powershell
# The file will be in your home directory
copy cursor_ai_memory.lattice $env:USERPROFILE\.cursor_ai_memory.lattice
```

### Query Your Memories

```python
from synrix.raw_backend import RawSynrixBackend
import json

# Load your lattice file
memory = RawSynrixBackend("~/.cursor_ai_memory.lattice")

# Query your stored context
context = memory.find_by_prefix("TASK:windows_build_transition", limit=1)
if context:
    data = json.loads(context[0]['data'].decode('utf-8'))
    print("Context restored:", data)

# Query constraints
constraints = memory.find_by_prefix("CONSTRAINT:", limit=20)
print(f"Found {len(constraints)} constraints")

# Query patterns
patterns = memory.find_by_prefix("PATTERN:", limit=20)
print(f"Found {len(patterns)} patterns")
```

## Build Steps (On Host PC)

1. **Extract:**
   ```powershell
   cd $env:USERPROFILE\Downloads
   # Use 7-Zip or: tar -xzf synrix-windows-build.tar.gz
   cd synrix-windows-build
   ```

2. **Build Engine:**
   ```powershell
   cd build\windows
   .\build.ps1
   ```

3. **Copy DLL to Python SDK:**
   ```powershell
   # DLL will be in build\windows\build\Release\libsynrix.dll
   copy build\Release\libsynrix.dll ..\..\python-sdk\
   ```

4. **Use Your Lattice File (Your Memories):**
   ```powershell
   # Copy lattice file to your home directory (Windows path)
   copy ..\..\lattice\cursor_ai_memory.lattice $env:USERPROFILE\.cursor_ai_memory.lattice
   
   # Test it works with your memories
   cd ..\..\python-sdk
   python -c "from synrix.raw_backend import RawSynrixBackend; m = RawSynrixBackend('~/.cursor_ai_memory.lattice'); print('✅ Memories loaded!'); print(f'Nodes: {len(m.find_by_prefix(\"\", limit=1000))}')"
   ```

5. **Test Python SDK:**
   ```powershell
   python -c "from synrix.raw_backend import RawSynrixBackend; print('✅ SDK working!')"
   ```

6. **Import Context (Only if lattice file missing):**
   ```powershell
   python import_context.py
   ```

## Key Decisions Made

1. **Primary target: Linux** (native, no compromises)
2. **Windows: MinGW-w64 first** for fastest bring-up
3. **12 Windows gotchas documented** (mmap parity, alignment, durability, etc.)
4. **Platform abstraction design ready** if Windows becomes first-class
5. **Fixed build.ps1** - removed Unicode characters that caused encoding issues

## Important Files

- `WINDOWS_COMPATIBILITY.md` - All 12 Windows landmines documented
- `PLATFORM_ABSTRACTION.md` - Future platform abstraction design (if needed)
- `BUILD.md` - Detailed build instructions

## What's Ready

✅ **Python SDK already updated** - `raw_backend.py` now detects Windows DLL automatically
✅ **No new control files needed** - Everything is in the package
✅ **Complete SDK included** - All Python code, examples, and tools
✅ **Context import ready** - Script included to restore Jetson context

## Next Steps After Build

1. ✅ DLL automatically detected by Python SDK (no manual path setup needed)
2. ✅ Test with: `python -c "from synrix.raw_backend import RawSynrixBackend; print('OK')"`
3. ✅ Import context: `python import_context.py`
4. ✅ Use in your code: `from synrix.raw_backend import RawSynrixBackend`

## If You Need Help

All context is stored in SYNRIX under `TASK:windows_build_transition`. Query it to restore full context.
