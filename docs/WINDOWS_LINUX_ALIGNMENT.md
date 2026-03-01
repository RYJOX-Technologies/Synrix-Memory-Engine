# Windows / Linux alignment – same behavior

Make SDK, engine, and license behave the same on both platforms so one key format and one test approach work everywhere.

**Linux build and release:** See **docs/LINUX_COHESION_CHECKLIST.md** for the Linux build and release tarball.

---

## 1. One engine binary per platform

| Platform | Binary | Build |
|----------|--------|--------|
| **Windows** | `libsynrix.dll` | `build/windows` (CMake / MSYS2) |
| **Linux** | `libsynrix.so` | `build/linux/build.sh` |

License is applied inside the engine at init on both platforms. The SDK uses the native lib only (no server required).

---

## 2. Same key source order (both platforms)

First non-empty wins:

| Priority | Source | Windows | Linux |
|----------|--------|---------|--------|
| 1 | Argument | Key passed to init/parse | Same |
| 2 | Environment | `SYNRIX_LICENSE_KEY` | Same |
| 3 | Per-user file | `%AppData%\Synrix\license.json` | `~/.synrix/license.json` |
| 4 | Next to binary | `license_key` in same dir as DLL/exe | `license_key` in same dir as .so |

License format and tier behavior are consistent across platforms. See release documentation for usage.

---

## 3. Same SDK usage (no server)

- Use only the SDK + native lib (`.dll` / `.so`). No HTTP, no server.
- **Load:** `RawSynrixBackend(lattice_path, max_nodes=..., evaluation_mode=False)`.
- **Discovery:** Windows: `SYNRIX_LIB_PATH` or DLL next to exe/SDK; Linux: `LD_LIBRARY_PATH` or `SYNRIX_LIB_PATH` or repo build output when running from repo.

---

## 4. Same test: SDK + lib only (no server)

- **Script:** `scripts/test_license_sdk_lib.py` (or equivalent).
- Uses only `RawSynrixBackend` + `libsynrix.dll` / `libsynrix.so`. Creates a temp lattice, adds nodes until limit or `--max-adds N` is reached.
- **Windows:** Set `PATH` or `SYNRIX_LIB_PATH` to the dir containing `libsynrix.dll`, then run the script.
- **Linux:** Run with `LD_LIBRARY_PATH` or `SYNRIX_LIB_PATH` set to the dir containing `libsynrix.so`.

---

## 5. Checklist summary

| Item | Windows | Linux |
|------|---------|--------|
| Single engine binary with lattice + license | ✅ | ✅ |
| License applied at init | ✅ | Same behavior |
| Key source order: arg → env → file → next-to-binary | ✅ | Same |
| License file path | `%AppData%\Synrix\license.json` | `~/.synrix/license.json` |
| Test: SDK + lib only, no server | Run script with DLL on PATH | Run with `LD_LIBRARY_PATH` / `SYNRIX_LIB_PATH` |

---

## 6. Bringing Linux changes into this repo (optional)

If your Linux build produces `build/linux/out/libsynrix.so` with the same key order and tier behavior as Windows, you can copy the build script and test script into this repo so both platforms are tested and documented in one place.
