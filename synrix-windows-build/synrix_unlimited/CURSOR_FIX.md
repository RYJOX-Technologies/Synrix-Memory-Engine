# Cursor IDE DLL Loading Fix

If you're using SYNRIX in Cursor and getting DLL errors, try these solutions:

## Quick Fix 1: Set Environment Variable

**In Cursor's terminal or settings:**

```powershell
$env:SYNRIX_LIB_PATH = "C:\path\to\synrix_unlimited\synrix\libsynrix.dll"
```

Replace `C:\path\to\synrix_unlimited` with your actual path.

## Quick Fix 2: Install Package Properly

**Option A: Install in development mode**
```powershell
cd C:\path\to\synrix_unlimited
pip install -e .
```

**Option B: Add to Python path in your script**
```python
import sys
import os
sys.path.insert(0, r'C:\path\to\synrix_unlimited')

from synrix.raw_backend import RawSynrixBackend
```

## Quick Fix 3: Use Absolute Path in Code

If Cursor's Python environment is different, explicitly set the path:

```python
import os
os.environ['SYNRIX_LIB_PATH'] = r'C:\path\to\synrix_unlimited\synrix\libsynrix.dll'

from synrix.raw_backend import RawSynrixBackend
backend = RawSynrixBackend('test.lattice')
```

## Diagnostic: Run This in Cursor

Create a test file in Cursor and run it:

```python
import sys
import os
from pathlib import Path

print("=== Cursor Environment Diagnostic ===")
print(f"Python executable: {sys.executable}")
print(f"Current directory: {os.getcwd()}")
print(f"Python path:")
for i, p in enumerate(sys.path):
    print(f"  [{i}] {p}")

# Try to find synrix
try:
    import synrix
    print(f"\n[OK] synrix imported successfully")
    print(f"  Location: {synrix.__file__}")
except ImportError as e:
    print(f"\n[ERROR] Failed to import synrix: {e}")

# Check for DLL
synrix_path = Path(__file__).parent / "synrix"
if synrix_path.exists():
    dll_path = synrix_path / "libsynrix.dll"
    print(f"\nDLL check:")
    print(f"  synrix/ directory: {synrix_path}")
    print(f"  DLL path: {dll_path}")
    print(f"  DLL exists: {dll_path.exists()}")
    
    if dll_path.exists():
        print(f"  DLL size: {dll_path.stat().st_size / 1024:.1f} KB")
```

## Common Cursor Issues

### Issue 1: Cursor Uses Different Python
Cursor might be using a different Python interpreter than your terminal.

**Solution:** Check Cursor's Python interpreter setting and ensure it matches where you installed the package.

### Issue 2: Package Not in sys.path
Cursor's Python might not see the package directory.

**Solution:** Add the directory to `PYTHONPATH` in Cursor's settings or use `sys.path.insert()` in your code.

### Issue 3: DLL Dependencies Missing
The DLL might be found but its dependencies (MinGW runtime) are missing.

**Solution:** Ensure all DLLs are in the same directory:
- `libsynrix.dll`
- `libgcc_s_seh-1.dll`
- `libstdc++-6.dll`
- `libwinpthread-1.dll`

## Still Not Working?

1. Run the diagnostic script above
2. Share the output
3. Check `TROUBLESHOOTING.md` for more solutions
