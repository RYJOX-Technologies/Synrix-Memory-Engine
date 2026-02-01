# Installation Guide

## Option 1: Direct Import (Recommended for Testing)

Simply add the package to your Python path:

```python
import sys
sys.path.insert(0, '/path/to/synrix_free_tier_50k')

from synrix.ai_memory import get_ai_memory
memory = get_ai_memory()
```

## Option 2: Install as Package

```bash
cd synrix_free_tier_50k
pip install -e .
```

Then use normally:

```python
from synrix.ai_memory import get_ai_memory
memory = get_ai_memory()
```

## Option 3: Copy to Project

Copy the `synrix/` directory to your project:

```bash
cp -r synrix_free_tier_50k/synrix /path/to/your/project/
```

Then import:

```python
from synrix.ai_memory import get_ai_memory
memory = get_ai_memory()
```

## Verification

Test the installation:

```python
from synrix.ai_memory import get_ai_memory

memory = get_ai_memory()
memory.add("TEST:installation", "SYNRIX is working!")
results = memory.query("TEST:")
print(f"✅ Installation successful! Found {len(results)} test nodes")
```

## Requirements

- **Python**: 3.8+ (required for `os.add_dll_directory()` on Windows)
- **Platform**: Windows 10+, Linux, macOS
- **No external dependencies**: Self-contained

## Troubleshooting

### Windows: DLL Not Found

If you see "DLL not found" errors:
1. Ensure all DLLs are in the `synrix/` directory
2. Check Python version (3.8+ required)
3. Try reinstalling the package

### Linux/macOS: Shared Library Not Found

If you see "shared library not found":
1. Ensure `.so` files are in the `synrix/` directory
2. Check file permissions (`chmod +x *.so`)
3. Verify Python can find the package
