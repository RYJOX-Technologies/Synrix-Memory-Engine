# SYNRIX Troubleshooting Guide

## DLL Not Found Error

If you see this error:
```
Critical Error: SYNRIX C library (libsynrix.dll) not found.
```

### Quick Fix

1. **Make sure you're in the right directory:**
   ```powershell
   cd C:\Users\user\Downloads\synrix_unlimited (4)\synrix_unlimited
   ```

2. **Verify the DLL exists:**
   ```powershell
   dir synrix\libsynrix.dll
   ```
   You should see the DLL file listed.

3. **Run the diagnostic script:**
   ```powershell
   python test_dll_load.py
   ```
   This will tell you exactly what's wrong.

### Common Issues

#### Issue 1: Running from Wrong Directory
**Symptom:** Error says DLL not found even though it exists.

**Solution:** Make sure you're in the `synrix_unlimited` directory (the one containing `synrix/` folder):
```powershell
cd "C:\Users\user\Downloads\synrix_unlimited (4)\synrix_unlimited"
python -c "from synrix import RawSynrixBackend; print('OK')"
```

#### Issue 2: Package Not in Python Path
**Symptom:** `ImportError: No module named 'synrix'`

**Solution:** Add the current directory to Python path:
```powershell
$env:PYTHONPATH = "C:\Users\user\Downloads\synrix_unlimited (4)\synrix_unlimited"
python -c "from synrix import RawSynrixBackend; print('OK')"
```

Or install the package:
```powershell
python setup.py develop
```

#### Issue 3: Missing Runtime DLLs
**Symptom:** DLL loads but crashes with "missing dependency" error.

**Solution:** Make sure all MinGW runtime DLLs are in `synrix/` directory:
- `libgcc_s_seh-1.dll`
- `libstdc++-6.dll`
- `libwinpthread-1.dll`
- `libsynrix.dll`

Check with:
```powershell
dir synrix\*.dll
```

#### Issue 4: Spaces in Path
**Symptom:** Works sometimes, fails other times.

**Solution:** Paths with spaces (like `synrix_unlimited (4)`) can sometimes cause issues. Try:
1. Rename the folder to remove spaces: `synrix_unlimited_4`
2. Or use quotes when setting paths:
   ```powershell
   $env:SYNRIX_LIB_PATH = "C:\Users\user\Downloads\synrix_unlimited (4)\synrix_unlimited\synrix\libsynrix.dll"
   ```

### Manual DLL Path (Last Resort)

If nothing else works, set the environment variable:

**PowerShell:**
```powershell
$env:SYNRIX_LIB_PATH = "C:\Users\user\Downloads\synrix_unlimited (4)\synrix_unlimited\synrix\libsynrix.dll"
```

**Command Prompt:**
```cmd
set SYNRIX_LIB_PATH=C:\Users\user\Downloads\synrix_unlimited (4)\synrix_unlimited\synrix\libsynrix.dll
```

Then run your Python script.

### Still Not Working?

Run the diagnostic script and share the output:
```powershell
python test_dll_load.py
```

This will show exactly what Python sees and where it's looking for the DLL.
