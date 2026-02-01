# If You're Having Issues - Step-by-Step Guide

**[WARNING] First: Make sure you're in the right folder!** See `WHERE_AM_I.md` if you're confused about the package structure.

Follow these steps in order. Stop when something works!

## Step 0: Try the One-Click Installer First!

**Before troubleshooting, try the improved installer:**

```bash
installer_v2.bat
```

This will automatically:
- [OK] Check for missing dependencies
- [OK] Install Visual C++ 2013 Runtime (if needed)
- [OK] Download zlib1.dll (if needed)
- [OK] Install SYNRIX package
- [OK] Test installation

**If the installer works, you're done!** If it fails, continue to Step 1.

---

## Step 1: Run the Diagnostic (30 seconds)

**In Cursor, open a terminal and run:**

```bash
python test_dll_load.py
```

**Or if that's not available:**

```bash
python fix_dll_path.py
```

This will tell you exactly what's wrong and may fix it automatically.

**What to look for:**
- [OK] If it says "SUCCESS" -> You're done. Everything works.
- [WARNING] If it says "libpath not set" or "DLL not found" -> Go to Step 2
- [ERROR] If it says "FAILED" -> Go to Step 3

---

## Step 2: DLL Won't Load (DLL exists but fails to load)

**Quick Fix Option A: Check Dependencies First**

```bash
python check_dll_dependencies.py
```

This will check if all required DLLs are present and try to load the DLL. It will show the exact error if it fails.

**Quick Fix Option B: Run the Fix Script**

```bash
python fix_dll_path.py
```

This will automatically find the DLL and set the path. Follow the instructions it prints.

**Quick Fix Option B: Set Environment Variable Manually**

**First, find where synrix is installed:**
```python
import synrix
import os
print(os.path.dirname(synrix.__file__))
```

**Then in PowerShell (Cursor's terminal):**
```powershell
$env:SYNRIX_LIB_PATH = "C:\path\to\synrix\libsynrix.dll"
```

**Replace `C:\path\to\synrix` with the path from above (should end with `synrix`).**

**Then test:**
```python
from synrix.ai_memory import get_ai_memory
memory = get_ai_memory()
memory.add("TEST", "Works!")
```

**If that works:** Add this to your Python script permanently (see Step 4).

**If that doesn't work:** Go to Step 4.

---

## Step 3: Package Not Found (Import Error)

**Quick Fix - Add to Python Path:**

**In your Python script, add this at the top:**
```python
import sys
import os

# Add the synrix_unlimited directory to Python path
synrix_path = r"C:\path\to\synrix_unlimited"  # Change this!
sys.path.insert(0, synrix_path)

# Now import synrix
from synrix.raw_backend import RawSynrixBackend
```

**Replace `C:\path\to\synrix_unlimited` with your actual path.**

**If that works:** You're done!

**If that doesn't work:** Go to Step 4.

---

## Step 4: Install the Package Properly

**Option A: Development Install (Recommended)**

1. Open terminal in Cursor
2. Navigate to the synrix_unlimited directory:
   ```powershell
   cd C:\path\to\synrix_unlimited
   ```
3. Install (note the **dot** at the end!):
   ```powershell
   pip install -e .
   ```
   **Important:** The dot (`.`) means "current directory" - don't forget it!
4. Test:
   ```python
   from synrix.raw_backend import RawSynrixBackend
   backend = RawSynrixBackend('test.lattice')
   ```

**Option B: Permanent Environment Variable**

**Windows (PowerShell):**
```powershell
[System.Environment]::SetEnvironmentVariable('SYNRIX_LIB_PATH', 'C:\path\to\synrix_unlimited\synrix\libsynrix.dll', 'User')
```

**Then restart Cursor.**

---

## Step 5: Still Not Working?

**Check these common issues:**

### Issue A: Missing DLL Files
**Check that all these files exist in `synrix/` directory:**
- `libsynrix.dll`
- `libgcc_s_seh-1.dll`
- `libstdc++-6.dll`
- `libwinpthread-1.dll`

**If any are missing:** The package is incomplete. Re-download the zip.

### Issue B: Wrong Python Interpreter
**Cursor might be using a different Python than your terminal.**

1. Check Cursor's Python interpreter:
   - Open Command Palette (Ctrl+Shift+P)
   - Type "Python: Select Interpreter"
   - Note which Python it's using

2. Make sure you installed the package to that Python:
   ```powershell
   # Use the same Python that Cursor uses
   C:\path\to\cursor\python.exe -m pip install -e C:\path\to\synrix_unlimited
   ```

### Issue C: Spaces in Path
**Paths with spaces (like `synrix_unlimited (4)`) can cause issues.**

**Solution:** Rename the folder to remove spaces:
```powershell
Rename-Item "synrix_unlimited (4)" "synrix_unlimited_4"
```

Then update all paths to use the new name.

---

## Step 6: Get Help

**If nothing above works, provide this information:**

1. **Output from diagnostic:**
   ```bash
   python cursor_diagnostic.py > diagnostic_output.txt
   ```
   Share the contents of `diagnostic_output.txt`

2. **Exact error message:**
   - Copy the full error message
   - Include the traceback

3. **Your setup:**
   - Where is synrix_unlimited located? (full path)
   - What Python version? (`python --version`)
   - What's your Cursor Python interpreter? (from Step 5B)

4. **What you've tried:**
   - Which steps from above did you try?
   - What happened with each?

**Then check:**
- `CURSOR_FIX.md` - More Cursor-specific solutions
- `TROUBLESHOOTING.md` - General troubleshooting guide

---

## Quick Reference: Most Common Solutions

**Solution 1 (Fastest - Run this script):**
```bash
python fix_dll_path.py
```

**Solution 1b (Manual - Add to your Python script):**
```python
import os
import synrix
synrix_path = os.path.dirname(synrix.__file__)
os.environ['SYNRIX_LIB_PATH'] = os.path.join(synrix_path, 'libsynrix.dll')
from synrix.ai_memory import get_ai_memory
```

**Solution 2 (Most Reliable):**
```powershell
cd C:\path\to\synrix_unlimited
pip install -e .
```

**Solution 3 (If Cursor uses different Python):**
```powershell
# Find Cursor's Python first, then:
C:\path\to\cursor\python.exe -m pip install -e C:\path\to\synrix_unlimited
```

---

**Remember:** The diagnostic script (`cursor_diagnostic.py`) will tell you exactly what's wrong. Always start there!
