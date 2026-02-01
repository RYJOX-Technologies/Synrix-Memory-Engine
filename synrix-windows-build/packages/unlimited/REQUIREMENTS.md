# SYNRIX System Requirements

## Required Dependencies

SYNRIX requires the following system components to run on Windows:

### 1. Visual C++ 2013 Runtime (x64)

**Required DLLs:**
- `msvcr120.dll`
- `msvcp120.dll`

**Installation:**
- **Automatic:** Run `install_vc2013.bat` (included in package)
- **Manual:** Download from https://www.microsoft.com/en-us/download/details.aspx?id=40784

**Why needed:**
`libsynrix.dll` was compiled with Visual C++ 2013 and requires these runtime libraries.

**Error if missing:**
```
Could not find module 'libsynrix.dll' (or one of its dependencies)
```

---

### 2. zlib1.dll

**Required DLL:**
- `zlib1.dll` (compression library)

**Installation:**
- **Automatic:** Run `download_zlib.ps1` (included in package)
- **Manual:** Download from https://github.com/brechtsanders/winlibs_mingw/releases
  - Extract `zlib1.dll` from the `bin/` directory
  - Copy to `synrix_unlimited/synrix/zlib1.dll`

**Why needed:**
`libsynrix.dll` uses zlib for compression/decompression operations.

**Error if missing:**
```
The specified module could not be found
```

---

### 3. MinGW Runtime DLLs (Included in Package)

These are **already included** in the package:
- `libgcc_s_seh-1.dll`
- `libstdc++-6.dll`
- `libwinpthread-1.dll`

**Location:** `synrix_unlimited/synrix/`

**Why needed:**
`libsynrix.dll` was compiled with MinGW-w64 and requires these runtime libraries.

---

## Quick Setup

### Option 1: One-Click Installer (Recommended!)

**Just double-click `installer_v2.bat` - it handles everything automatically!**

The installer will:
- [OK] Check Python version
- [OK] Install Visual C++ 2013 Runtime (if needed)
- [OK] Download zlib1.dll (if needed)
- [OK] Install SYNRIX package
- [OK] Test installation

**That's it!** No manual steps required.

### Option 2: Manual Installation

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

### Option 2: Manual

1. Download and install Visual C++ 2013 Redistributable (x64) from Microsoft
2. Download zlib1.dll from MinGW-w64 releases and copy to `synrix/` directory
3. Run `pip install -e .`

---

## Verification

After installation, verify all dependencies:

```bash
python check_dll_dependencies.py
```

This will check:
- [OK] All DLLs are present
- [OK] DLL architecture matches Python (64-bit)
- [OK] DLL loads successfully

---

## Troubleshooting

### "Could not find module 'libsynrix.dll'"

**Cause:** Missing Visual C++ 2013 Runtime

**Fix:** Run `install_vc2013.bat`

---

### "The specified module could not be found"

**Cause:** Missing zlib1.dll or MinGW runtime DLLs

**Fix:** 
1. Run `download_zlib.ps1` for zlib1.dll
2. Verify MinGW DLLs are in `synrix/` directory

---

### "function 'lattice_init' not found"

**Cause:** DLL loaded but functions not accessible (native loader issue)

**Fix:** Already handled automatically - `raw_backend.py` falls back to manual loading

---

## Technical Details

### DLL Dependencies

`libsynrix.dll` has the following dependencies:

1. **Visual C++ 2013 Runtime** (system-wide)
   - `msvcr120.dll` -> `C:\Windows\System32\`
   - `msvcp120.dll` -> `C:\Windows\System32\`

2. **zlib1.dll** (package-local)
   - Must be in same directory as `libsynrix.dll`
   - Location: `synrix_unlimited/synrix/zlib1.dll`

3. **MinGW Runtime** (package-local)
   - `libgcc_s_seh-1.dll` -> `synrix_unlimited/synrix/`
   - `libstdc++-6.dll` -> `synrix_unlimited/synrix/`
   - `libwinpthread-1.dll` -> `synrix_unlimited/synrix/`

### Compilation Details

- **Toolchain:** MinGW-w64
- **C++ Runtime:** Visual C++ 2013
- **Compression:** zlib
- **Architecture:** x64 (64-bit)

---

## Summary

**Minimum requirements:**
1. [OK] Python 3.8+ (64-bit)
2. [OK] Visual C++ 2013 Runtime (x64) - install via `install_vc2013.bat`
3. [OK] zlib1.dll - download via `download_zlib.ps1`
4. [OK] MinGW runtime DLLs - already included in package

**All dependencies can be installed automatically using the included scripts!**
