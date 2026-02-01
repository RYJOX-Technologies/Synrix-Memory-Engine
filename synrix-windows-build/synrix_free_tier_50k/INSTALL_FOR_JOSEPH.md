# Installation - Dead Simple (For Joseph)

## The Easiest Way (30 seconds)

### Step 1: Extract the Package

Unzip `synrix_free_tier_50k.zip` to any folder (e.g., `C:\synrix_free_tier_50k`)

### Step 2: Use It (One Line)

```python
import sys
sys.path.insert(0, r'C:\synrix_free_tier_50k')  # Change path to where you extracted it

from synrix.ai_memory import get_ai_memory
memory = get_ai_memory()
memory.add("TEST", "It works!")
print("✅ Done!")
```

**That's it!** No pip, no setup, no configuration.

## Even Easier: Install as Package (Recommended)

If you want to use it like a normal Python package:

```bash
cd C:\synrix_free_tier_50k
pip install -e .
```

Then use it anywhere:

```python
from synrix.ai_memory import get_ai_memory
memory = get_ai_memory()
```

## Test It Works

```python
from synrix.ai_memory import get_ai_memory

memory = get_ai_memory()
memory.add("TEST:install", "SYNRIX is working!")
results = memory.query("TEST:")
print(f"✅ Found {len(results)} nodes - Installation successful!")
```

## Requirements

- **Python 3.8+** (that's it!)
- **Windows 10+** (or Linux/macOS)
- **No other dependencies** - Everything is included

## If Something Goes Wrong

1. **"DLL not found"** → Make sure all 4 DLLs are in the `synrix/` folder
2. **"Module not found"** → Check the path in `sys.path.insert()` is correct
3. **Python version** → Need Python 3.8+ (check with `python --version`)

## That's It!

No scripts, no environment variables, no configuration files. Just extract and use.

See `QUICK_START.md` for usage examples.
