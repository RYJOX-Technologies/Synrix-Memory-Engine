# SYNRIX Windows Build

Build, packages, and SDKs for SYNRIX on Windows (single binary, signed license keys).

---

## What’s here

- **`synrix-windows-build/`** – Engine C source, CMake build, Python packages, stress test, and docs.
  - **Build:** `synrix-windows-build/build/windows/` – see `BUILD.md`, `MSYS2_BUILD_INSTRUCTIONS.md`, `LICENSE_SIGNED_KEY.md`.
  - **Packages:** `synrix-windows-build/packages/` – tier packages (100k, 1m, 10m, 50m, unlimited); tier set by `SYNRIX_LICENSE_KEY`.
  - **Quick summary for Joseph:** `synrix-windows-build/SUMMARY_FOR_JOSEPH.md`.
- **`synrix-sdks/`** – Agent Memory SDK, Robotics SDK, RAG SDK (Python; use with built `libsynrix.dll` + OpenSSL DLLs).

## Getting started

1. **Build the engine (MSYS2 MinGW 64-bit):**  
   See `synrix-windows-build/build/windows/MSYS2_BUILD_INSTRUCTIONS.md` and `LICENSE_SIGNED_KEY.md`.
2. **Run tests:**  
   See `synrix-windows-build/TEST_NOW.md` and `PRE_PROD_CHECKLIST.md`.
3. **Push to GitHub:**  
   See `GITHUB_SETUP.md` (use `.gitignore`, never commit the private key, then push).

## License / keys

- Tier (100k / 1m / 10m / 50m / unlimited) is set by a **signed license key** (`SYNRIX_LICENSE_KEY`). Only the key holder can issue valid keys. See `synrix-windows-build/build/windows/LICENSE_SIGNED_KEY.md`.
