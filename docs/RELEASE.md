# Release process (Windows + Linux)

Release status: both platforms ready. Windows zip and Linux tarball built and tested.

---

## 1. Pre-release

- Build Windows engine (CMake / MSYS2) → `libsynrix.dll` + runtimes.
- Build Linux engine (`build/linux/build.sh`) → `build/linux/out/libsynrix.so`.
- Run `scripts/test_license_sdk_lib.py` with `SYNRIX_LIB_PATH` (or `PATH` / `LD_LIBRARY_PATH`) set to the folder containing the engine. Optionally run `build/windows/tools/test_release_zip.py` after creating the Windows zip.

## 2. Windows release

- Create `synrix-windows.zip`: DLL, required runtime DLLs, optional README.
- Script: `build/windows/create_release_zip.ps1` (or equivalent).
- Attach `synrix-windows.zip` to the GitHub release.

## 3. Linux release

- Create tarball: `synrix-linux-x86_64.tar.gz` (or `synrix-linux-arm64.tar.gz`) containing `libsynrix.so` and bundled runtime libs.
- Build: `build/linux/build.sh`.
- Attach tarball to the same GitHub release.
- SHA256 for each asset is shown on the release page.

## 4. Quick-reference table

| Step | Windows | Linux |
|------|---------|--------|
| Build | CMake (MSYS2) | `build/linux/build.sh` |
| Output | `libsynrix.dll` + runtimes | `build/linux/out/libsynrix.so` |
| Package | `synrix-windows.zip` | `synrix-linux-*.tar.gz` |
| Test | `test_release_zip.py` / `test_license_sdk_lib.py` | `test_license_sdk_lib.py` with `LD_LIBRARY_PATH` |

## 5. Docs to keep aligned

- **docs/LINUX_COHESION_CHECKLIST.md** – Linux build and SDK checklist.
- **docs/WINDOWS_LINUX_ALIGNMENT.md** – Same behavior (key order, SDK, tier, tests).
