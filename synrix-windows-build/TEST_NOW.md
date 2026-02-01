# What to Do Now to Test

Follow these steps in order so you can test the single-DLL + signed license setup.

---

## 1. Install OpenSSL in MSYS2 (required for license verification)

Open **MSYS2 MinGW 64-bit** and run:

```bash
pacman -S mingw-w64-x86_64-openssl
```

If you skip this, the engine still builds but license keys are ignored (everyone gets the default cap).

---

## 2. Rebuild the engine

In the same **MSYS2 MinGW 64-bit** terminal:

```bash
cd /c/Users/Livew/Desktop/synrix-windows-build/synrix-windows-build/build/windows
bash build_msys2.sh
```

When CMake runs, check the output for a line like **"OpenSSL found"** or **"SYNRIX_LICENSE_USE_OPENSSL"** so you know license verification is enabled.

The new DLL will be at:

`build/windows/build_msys2/bin/libsynrix.dll`

---

## 3. Copy the new DLL everywhere it’s used

From **PowerShell** (run from `synrix-windows-build`):

```powershell
cd C:\Users\Livew\Desktop\synrix-windows-build\synrix-windows-build

$dll = "build\windows\build_msys2\bin\libsynrix.dll"
$packages = "packages\free_tier_100k\synrix", "packages\free_tier_1m\synrix", "packages\free_tier_10m\synrix", "packages\free_tier_50m\synrix", "packages\unlimited\synrix"
foreach ($p in $packages) { Copy-Item $dll $p -Force }
Copy-Item $dll "..\synrix-sdks\agent-memory-sdk\synrix" -Force
```

(If `synrix-sdks` lives inside `synrix-windows-build`, use `synrix-sdks\agent-memory-sdk\synrix` instead of `..\synrix-sdks\...`.)

---

## 4. Get an unlimited license key

From **PowerShell** or **cmd** (from `synrix-windows-build`):

```powershell
cd C:\Users\Livew\Desktop\synrix-windows-build\synrix-windows-build\build\windows
python tools/synrix_license_keygen.py --tier unlimited --private synrix_license_private.key
```

Copy the **single line** of output (the base64 key). You’ll set it in the next step.

---

## 5. Run the stress test (no key → should cap; with key → should pass)

**Important:** `libsynrix.dll` links to OpenSSL. From PowerShell, add MSYS2’s MinGW bin to PATH so the OpenSSL DLLs are found (otherwise the process can exit silently during init):

```powershell
$env:Path = "C:\msys64\mingw64\bin;" + $env:Path
```

Then run the tests below from the same PowerShell session.

**Test A – No license key (should hit default cap ~25k):**

```powershell
cd C:\Users\Livew\Desktop\synrix-windows-build\synrix-windows-build
# Make sure SYNRIX_LICENSE_KEY is not set (or remove it)
Remove-Item Env:SYNRIX_LICENSE_KEY -ErrorAction SilentlyContinue
python stress_test_binary.py --package "packages\unlimited" --dll libsynrix.dll --nodes 26000 --no-evaluation
```

You should see the run stop at the default node limit (e.g. 25k).

**Test B – With unlimited key (should go past 26k):**

```powershell
$env:SYNRIX_LICENSE_KEY = "PASTE_THE_UNLIMITED_KEY_HERE"
python stress_test_binary.py --package "packages\unlimited" --dll libsynrix.dll --nodes 26000 --no-evaluation
```

You should see all 26,000 nodes inserted and the test complete.

---

## 6. Optional: keygen/format test

From `synrix-windows-build`:

```powershell
python build\windows\tools\test_license_flow.py
```

This checks that keygen and key format work; it does **not** load the DLL.

---

## Quick reference

| Step | Where        | Command / action |
|------|--------------|-------------------|
| 1    | MSYS2        | `pacman -S mingw-w64-x86_64-openssl` |
| 2    | MSYS2        | `cd build/windows` then `bash build_msys2.sh` |
| 3    | PowerShell   | Copy `build\windows\build_msys2\bin\libsynrix.dll` to each `packages\*\synrix\` and to `synrix-sdks\agent-memory-sdk\synrix` |
| 4    | PowerShell   | `python build\windows\tools\synrix_license_keygen.py --tier unlimited --private build\windows\synrix_license_private.key` |
| 5a   | PowerShell   | Unset `SYNRIX_LICENSE_KEY`, run stress test 26k nodes → expect cap |
| 5b   | PowerShell   | Set `SYNRIX_LICENSE_KEY`, run stress test 26k nodes → expect success |

Once Test B passes, the single-DLL + signed license path is working end-to-end.

---

## If the stress test exits silently (no "Loaded DLL", no error)

`libsynrix.dll` depends on OpenSSL at runtime. If those DLLs are not found, the process can exit during init with no message.

**1. Copy OpenSSL DLLs next to libsynrix.dll**

From **PowerShell** (run from `synrix-windows-build`):

```powershell
$dest = "C:\Users\Livew\Desktop\synrix-windows-build\synrix-windows-build\packages\unlimited\synrix"
Copy-Item "C:\msys64\mingw64\bin\libcrypto*.dll" $dest -Force
Copy-Item "C:\msys64\mingw64\bin\libssl*.dll" $dest -Force
```

(If your MSYS2 is elsewhere, use that path; the DLL names may be `libcrypto-3-x64.dll` / `libssl-3-x64.dll` or `libcrypto-1_1-x64.dll` / `libssl-1_1-x64.dll`.)

**2. Run with debug so C stderr is visible**

```powershell
$env:SYNRIX_DEBUG = "1"
$env:SYNRIX_LICENSE_KEY = "YOUR_UNLIMITED_KEY"
python -u stress_test_binary.py --package "packages\unlimited" --dll libsynrix.dll --nodes 26000 --no-evaluation
```

If the C code prints to stderr before crashing, you’ll see it. `-u` makes Python output unbuffered.
