# WHERE AM I? - Package Structure Guide

## [WARNING] IMPORTANT: The Correct Structure

When you extract `synrix_unlimited.zip`, you should see this:

```
synrix_unlimited/                    <- YOU ARE HERE (root folder)
├── installer.bat                    <- Double-click this to install
├── setup.py
├── START_HERE.md                    <- Read this first
├── synrix/                          <- The actual Python package
│   ├── __init__.py
│   ├── ai_memory.py
│   ├── libsynrix.dll               <- DLL files are here
│   └── ... (other files)
└── ... (other files)
```

## [OK] CORRECT: Where You Should Be

**After extracting the zip, you should be at:**
```
C:\Users\user\Downloads\synrix_unlimited
```

**NOT:**
```
C:\Users\user\Downloads\synrix_unlimited\synrix_unlimited\python-sdk  <- WRONG
```

## [WARNING] Common Mistakes

### Mistake 1: Double-Extracted
If you see `synrix_unlimited\synrix_unlimited\`, you extracted twice!

**Fix:** Go back to the first `synrix_unlimited` folder (the outer one).

### Mistake 2: Looking for `python-sdk`
**There is NO `python-sdk` folder!** The package structure is flat.

**The package is:**
- `synrix_unlimited/` (root)
  - `synrix/` (the Python package - this is what you import)

## [OK] What to Do

### Option 1: Use the Installer (Easiest!)
1. Navigate to: `C:\Users\user\Downloads\synrix_unlimited`
2. Double-click `installer.bat`
3. Done!

### Option 2: Manual Install
1. Open PowerShell or Command Prompt
2. Navigate to the root folder:
   ```powershell
   cd C:\Users\user\Downloads\synrix_unlimited
   ```
3. Install:
   ```powershell
   pip install -e .
   ```

### Option 3: Quick Test (No Install)
1. Navigate to: `C:\Users\user\Downloads\synrix_unlimited`
2. In your Python script, add:
   ```python
   import sys
   sys.path.insert(0, r'C:\Users\user\Downloads\synrix_unlimited')
   
   from synrix.ai_memory import get_ai_memory
   memory = get_ai_memory()
   ```

## [INFO] How to Check You're in the Right Place

**Run this in PowerShell:**
```powershell
cd C:\Users\user\Downloads\synrix_unlimited
dir
```

**You should see:**
- `installer.bat` [OK]
- `setup.py` [OK]
- `synrix/` folder [OK]
- `START_HERE.md` [OK]

**If you DON'T see these files, you're in the wrong folder!**

## Folder Quick Navigation

**If you're lost, find the folder that contains:**
- `installer.bat` <- This file
- `setup.py` <- This file
- `synrix/` folder <- This folder

**That's your root folder!** Everything else is inside it.

## Questions?

1. **Find `installer.bat`** in File Explorer
2. **Right-click it** -> "Open in Terminal" or "Open PowerShell here"
3. **You're now in the right place!**

---

**Remember:** The root folder is where `installer.bat` and `setup.py` are. That's where you need to be!
