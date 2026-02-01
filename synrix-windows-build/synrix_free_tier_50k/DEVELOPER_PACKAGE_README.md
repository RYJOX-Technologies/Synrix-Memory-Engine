# SYNRIX Developer Package - For Joseph

This is a **complete developer package** that includes both the Python SDK and the engine source code for building on Windows and Linux.

## What's Included

1. **Python SDK** (`synrix/` directory)
   - Ready-to-use Python package
   - Windows-native DLL loader
   - AI memory interface
   - All documentation

2. **Engine Source Code** (`engine/` directory)
   - Complete C source files
   - Header files
   - CMake build configuration
   - Build scripts for Windows and Linux

## Two Ways to Use This Package

### Option 1: Use Pre-Built Binary (Easiest)

If you just want to use SYNRIX without building:

1. Extract the package
2. The `synrix/` directory already contains pre-built binaries
3. Use it immediately (see `QUICK_START.md`)

**No build required!**

### Option 2: Build from Source (For Development)

If you want to build the engine yourself:

1. Extract the package
2. Follow `DEVELOPER_BUILD_GUIDE.md`
3. Build for Windows or Linux
4. Copy the built binary to `synrix/` directory

## Quick Start (No Build)

```python
import sys
sys.path.insert(0, r'C:\synrix_free_tier_50k')  # Your path

from synrix.ai_memory import get_ai_memory
memory = get_ai_memory()
memory.add("TEST", "Works!")
```

## Building from Source

### Windows (MSYS2)

```bash
cd engine/build/windows
bash build_msys2.sh
cp build_msys2/bin/libsynrix_free_tier.dll ../../../synrix/
```

### Linux

```bash
cd engine/build/linux
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DSYNRIX_FREE_TIER_50K=ON
make -j$(nproc)
cp libsynrix_free_tier.so ../../../synrix/
```

See `DEVELOPER_BUILD_GUIDE.md` for detailed instructions.

## Package Structure

```
synrix_free_tier_50k/
├── synrix/                    # Python SDK (ready to use)
│   ├── __init__.py
│   ├── _native.py
│   ├── raw_backend.py
│   ├── ai_memory.py
│   └── *.dll or *.so          # Pre-built binaries
├── engine/                     # Engine source code
│   ├── src/                    # C source files
│   ├── include/                # Header files
│   └── build/
│       ├── windows/             # Windows build
│       └── linux/               # Linux build
├── README.md                    # Main documentation
├── QUICK_START.md               # 5-minute guide
├── DEVELOPER_BUILD_GUIDE.md    # Build instructions
├── AI_AGENT_GUIDE.md            # Usage examples
└── INSTALL.md                   # Installation options
```

## Requirements

### For Using Pre-Built Binary

- **Python 3.8+** (that's it!)

### For Building from Source

- **Windows**: MSYS2 + MinGW-w64 OR Visual Studio 2022 + CMake
- **Linux**: `build-essential` + `cmake`
- **CMake 3.15+**

## Features

- ✅ **50,000 node limit** (free tier)
- ✅ **O(1) lookups** (~131.5 ns)
- ✅ **O(k) prefix queries** (scales with results)
- ✅ **Persistent storage** (survives restarts)
- ✅ **Windows-native** (no scripts needed)
- ✅ **Linux support** (full compatibility)
- ✅ **No external dependencies** (self-contained)

## Documentation

- **`START_HERE.md`** - First file to read
- **`QUICK_START.md`** - 5-minute getting started
- **`DEVELOPER_BUILD_GUIDE.md`** - Build from source
- **`AI_AGENT_GUIDE.md`** - Comprehensive examples
- **`INSTALL.md`** - Installation options
- **`README.md`** - Full documentation

## Testing

Test the package:

```python
from synrix.ai_memory import get_ai_memory

memory = get_ai_memory()
memory.add("TEST:install", "SYNRIX works!")
print(f"✅ Found {len(memory.query('TEST:'))} nodes")
```

## Support

- Check `DEVELOPER_BUILD_GUIDE.md` for build issues
- Check `INSTALL.md` for installation issues
- Check `AI_AGENT_GUIDE.md` for usage examples

---

**Ready to go!** Extract, use, or build - your choice.
