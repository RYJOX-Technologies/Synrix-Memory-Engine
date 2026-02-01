# For Joseph - Quick Start

## What You're Getting

This package contains **everything** you need:

1. **Python SDK** (`synrix/` folder) - Ready to use, pre-built binaries included
2. **Engine Source Code** (`engine/` folder) - If you want to build from source

## Two Options

### Option 1: Use Pre-Built (Easiest - 30 seconds)

The package already includes pre-built binaries. Just use it:

```python
import sys
sys.path.insert(0, r'C:\synrix_free_tier_50k')  # Your path

from synrix.ai_memory import get_ai_memory
memory = get_ai_memory()
memory.add("TEST", "Works!")
```

**That's it!** No build needed.

### Option 2: Build from Source

If you want to build the engine yourself (Windows or Linux):

1. **Windows (MSYS2)**:
   ```bash
   cd engine/build/windows
   bash build_msys2.sh
   cp build_msys2/bin/libsynrix_free_tier.dll ../../../synrix/
   ```

2. **Linux**:
   ```bash
   cd engine/build/linux
   bash build.sh
   cp build/libsynrix_free_tier.so ../../../synrix/
   ```

See `DEVELOPER_BUILD_GUIDE.md` for detailed instructions.

## What You Need

### For Using Pre-Built Binary
- **Python 3.8+** (that's it!)

### For Building from Source
- **Windows**: MSYS2 + MinGW-w64 OR Visual Studio 2022 + CMake
- **Linux**: `build-essential` + `cmake`
- **CMake 3.15+**

## Package Contents

```
synrix_free_tier_50k/
├── synrix/                    # Python SDK (ready to use)
│   ├── *.dll or *.so          # Pre-built binaries
│   └── *.py                   # Python code
├── engine/                     # Engine source code
│   ├── src/                    # C source files
│   ├── include/                # Header files
│   └── build/
│       ├── windows/            # Windows build scripts
│       └── linux/              # Linux build scripts
└── Documentation files
```

## Quick Test

```python
from synrix.ai_memory import get_ai_memory

memory = get_ai_memory()
memory.add("TEST:install", "SYNRIX works!")
print(f"✅ Found {len(memory.query('TEST:'))} nodes")
```

## Documentation

- **`START_HERE.md`** - First file to read
- **`DEVELOPER_PACKAGE_README.md`** - Package overview
- **`DEVELOPER_BUILD_GUIDE.md`** - Build instructions
- **`QUICK_START.md`** - Usage examples
- **`AI_AGENT_GUIDE.md`** - Comprehensive guide

## That's It!

Extract → Use (or Build) → Done.

No scripts, no environment variables, no configuration files.
