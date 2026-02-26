# Windows / Linux alignment – same behavior

Make SDK, engine, and license behave the same on both platforms so one keygen, one key format, and one test approach work everywhere.

**Linux build and release status:** See **docs/LINUX_COHESION_CHECKLIST.md** for the authoritative Linux build (lattice + exact_name_index + license_utils + synrix_license_linux), SDK, tests, and release tarball.

---

## 1. One engine binary per platform (full license + lattice)

| Platform | Binary | Build | License in engine |
|----------|--------|--------|-------------------|
| **Windows** | `libsynrix.dll` | `build/windows` CMake (MSYS2 MinGW) | `license_verify.c` + `persistent_lattice.c` (calls `synrix_license_parse` inside `lattice_init`) |
| **Linux** | `libsynrix.so` | `build/linux/build.sh` | Lattice + `exact_name_index` + `license_utils` + `synrix_license_linux` (25k default, tier by key; same as server path). License in `lattice_init` or SDK apply after init. |

**Important:** On Windows, `lattice_init()` already calls `synrix_license_parse(NULL, &license_limit, &license_unlimited)` and sets `free_tier_limit`. For Linux to act the same, the Linux engine should do the same inside `lattice_init()`. Then the SDK only calls `lattice_init` and never needs to call license from Python on either platform. If your Linux tree does not yet apply license in `lattice_init`, you can either:

- **Preferred:** Add the same `synrix_license_parse(NULL, ...)` block at the start of `lattice_init()` in the Linux C code (same as Windows `persistent_lattice.c`), **or**
- Keep having the SDK apply license after init: in `raw_backend.py`, after `lattice_init`, if the loaded lib has `synrix_license_parse` and `lattice_apply_license`, call them (Linux-only path). Windows stays as-is (license already applied in init).

---

## 2. Same key source order (both platforms)

Use the same priority; first non-empty wins:

| Priority | Source | Windows | Linux |
|----------|--------|---------|--------|
| 1 | Argument to parse | `synrix_license_parse(key, ...)` | Same |
| 2 | Environment | `SYNRIX_LICENSE_KEY` | Same |
| 3 | Per-user file | `%AppData%\Synrix\license.json` | `~/.synrix/license.json` |
| 4 | Next to binary | Directory of `libsynrix.dll` / `synrix.exe` → `license_key` | Directory of `libsynrix.so` → `license_key` |

Same payload format (112-byte payload + 64-byte Ed25519 signature), same tier bytes (0=25k … 4=unlimited). See **docs/LINUX_LICENSE_SPEC.md** for the full spec.

---

## 3. Same SDK usage (no server)

- **Cursor memory / agent:** Use only the SDK + native lib (`.dll` / `.so`). No HTTP, no server.
- **Load:** `RawSynrixBackend(lattice_path, max_nodes=..., evaluation_mode=False)`.
- **License:** Applied inside the engine when `lattice_init` runs (Windows already; Linux should do the same in C, or SDK calls parse+apply on Linux when symbols exist).
- **Discovery:** Windows: `SYNRIX_LIB_PATH` or DLL next to exe/sdk; Linux: `LD_LIBRARY_PATH` or `SYNRIX_LIB_PATH` or `build/linux/out/` when running from repo.

---

## 4. Same test: SDK + lib only (no server)

Run the same style of test on both platforms:

- **Script:** `scripts/test_license_sdk_lib.py` (or equivalent name).
- **What it does:** Uses only `RawSynrixBackend` + `libsynrix.dll` / `libsynrix.so`. Creates a temp lattice, adds nodes (semantic names) until `add_node` returns 0 or `--max-adds N` is reached. No server, no CLI subprocess (optional: also test CLI separately).
- **Windows:** e.g. set `PATH` or `SYNRIX_LIB_PATH` to the dir containing `libsynrix.dll`, then  
  `python scripts/test_license_sdk_lib.py [--max-adds 1000]`
- **Linux:** e.g.  
  `LD_LIBRARY_PATH=build/linux/out python3 scripts/test_license_sdk_lib.py [--max-adds 1000]`

If your license is unlimited, a run with `--max-adds 1000` should add 1000+ nodes and succeed. If tier is 25k, the script should hit the limit and get `add_node` returning 0 (or `FreeTierLimitError`) at 25k+1.

---

## 5. Checklist summary

| Item | Windows | Linux |
|------|---------|--------|
| Single engine binary (.dll / .so) with lattice + license | ✅ CMake build | ✅ build.sh (lattice + exact_name_index + license_utils + synrix_license_linux) |
| License applied in `lattice_init()` | ✅ in `persistent_lattice.c` | Add same in Linux `lattice_init` (or SDK apply after init) |
| Key source order: arg → env → file → next-to-binary | ✅ `license_verify.c` | Same (e.g. `synrix_license_linux.c` or same file) |
| License file path | `%AppData%\Synrix\license.json` | `~/.synrix/license.json` |
| HWID | Registry `MachineGuid` | `/etc/machine-id` (or fallback) |
| SDK applies license on init | In `raw_backend._init_lattice`: if lib has `synrix_license_parse` + `lattice_apply_license`, apply after `lattice_init`; else use `lattice_disable_evaluation_mode` when not evaluation_mode. Same on Windows and Linux. |
| Test: SDK + lib only, no server | Run `test_license_sdk_lib.py` with DLL on PATH | Run with `LD_LIBRARY_PATH` / `SYNRIX_LIB_PATH` |
| Same tier limits (25k/1m/10m/50m/unlimited) | ✅ | ✅ (LINUX_LICENSE_SPEC §6) |

---

## 6. Bringing Linux changes into this repo (optional)

If your Linux dev box has:

- **build/linux/build.sh** – full build including lattice + `exact_name_index.c` + `license_utils` + `synrix_license_linux.c` (output `build/linux/out/libsynrix.so`); license in `lattice_init` or SDK apply after init,
- **python-sdk/synrix/raw_backend.py** – after `lattice_init`, call `synrix_license_parse` + `lattice_apply_license` when symbols exist (LicenseClaims struct; key_override=None → env / ~/.synrix/license.json). Same block in all package copies (unlimited, free_tier_*). Windows DLL can expose same symbols for parity.
- **scripts/test_license_sdk_lib.py** – SDK-only test with `--max-adds`,

you can copy those into this repo so both platforms are tested and documented in one place. The Windows side already has license in `lattice_init`; adding the Linux build script, the optional SDK license-apply block for Linux, and the shared test script will make Windows and Linux act the same from the user’s perspective.
