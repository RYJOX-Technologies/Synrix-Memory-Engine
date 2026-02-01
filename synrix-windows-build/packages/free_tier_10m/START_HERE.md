# START HERE - SYNRIX Unlimited

**New to SYNRIX?** Read `SIMPLE_GUIDE.md` first - it explains everything like you're 5 years old!

**[WARNING] Confused about where to navigate?** See `WHERE_AM_I.md` - shows the correct folder structure!

## Installation (One-Click!)

### Option A: One-Click Installer (Recommended - Default!)

**Just double-click `installer.bat` - it handles everything automatically!**

**Note:** `installer.bat` now uses the improved installer_v2 system by default.

1. **Double-click `installer_v2.bat`**
2. Wait for installation to complete (it will automatically):
   - Check Python version
   - Install Visual C++ 2013 Runtime (if needed)
   - Download zlib1.dll (if needed)
   - Install SYNRIX package
   - Test installation
3. **Done!** Use SYNRIX from any Python script:

**That's it!** The installer handles all dependencies automatically.

### Option B: Manual Installation

If you prefer manual control:

1. **Install Visual C++ 2013 Runtime:**
   ```bash
   install_vc2013.bat
   ```

2. **Download zlib1.dll:**
   ```powershell
   powershell -ExecutionPolicy Bypass -File download_zlib.ps1
   ```

3. **Install SYNRIX:**
   ```bash
   pip install -e .
   ```

```python
from synrix.ai_memory import get_ai_memory
memory = get_ai_memory()
memory.add("TEST", "Works!")
```

### Option B: Quick Test (No Install)

1. Extract this package anywhere (e.g., `C:\synrix_unlimited`)

2. In your Python script, add one line:

```python
import sys
sys.path.insert(0, r'C:\synrix_unlimited')  # Change to your path

from synrix.ai_memory import get_ai_memory
memory = get_ai_memory()
memory.add("TEST", "Works!")
```

**Done!** That's it.

### Option C: Install as Package (Manual)

```bash
cd C:\synrix_unlimited
pip install -e .
```

**Note:** The dot (`.`) at the end is required - it means "install the current directory"

Then use it anywhere:

```python
from synrix.ai_memory import get_ai_memory
memory = get_ai_memory()
```

## Quick Test

```python
from synrix.ai_memory import get_ai_memory

memory = get_ai_memory()
memory.add("TEST:install", "SYNRIX works!")
print(f"Found {len(memory.query('TEST:'))} nodes")
```

## What You Get

- [OK] **10,000,000 node limit** - Free tier evaluation version

- [OK] **Unlimited nodes** - No limits, use as much as you need
- [OK] **Fast**: O(1) lookups, O(k) queries
- [OK] **Persistent**: Survives restarts
- [OK] **No setup**: Just extract and use
- [OK] **No dependencies**: Everything included

## Next Steps

- `USAGE_GUIDE.md` - **Complete usage guide with examples**
- `README.md` - Full documentation
- `FOR_JOSEPH.md` - Quick reference

## Requirements

- Python 3.8+ (64-bit)
- Visual C++ 2013 Runtime (x64) - install via `install_vc2013.bat`
- zlib1.dll - download via `download_zlib.ps1`

**See `REQUIREMENTS.md` for detailed dependency information.**

## Troubleshooting

**Having issues?** 

1. **Start here:** `IF_YOU_HAVE_ISSUES.md` - Step-by-step troubleshooting guide
2. **Quick diagnostic:** Run `python cursor_diagnostic.py` (or `test_dll_load.py`)
3. **Cursor-specific:** See `CURSOR_FIX.md` if using Cursor IDE
4. **General help:** See `TROUBLESHOOTING.md` for more solutions

---

**That's it!** No scripts, no config, no hassle. Just extract and use.
